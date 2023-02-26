#include <sys/sysinfo.h>
#include <cassert>
#include <iostream>
#include <string>
#include <stdexcept>
#include <chrono>
#include <algorithm>
#include <limits>

#include "encoder.hh"
#include "conversion.hh"
#include "timestamp.hh"

using namespace std;
using namespace chrono;

Encoder::Encoder(const uint16_t display_width,
                 const uint16_t display_height,
                 const uint16_t frame_rate,
                 const string & output_path)
  : display_width_(display_width), display_height_(display_height),
    frame_rate_(frame_rate), output_fd_()
{
  // open the output file
  if (not output_path.empty()) {
    output_fd_ = FileDescriptor(check_syscall(
        open(output_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644)));
  }

  // populate VP9 configuration with default values
  check_call(vpx_codec_enc_config_default(&vpx_codec_vp9_cx_algo, &cfg_, 0),
             VPX_CODEC_OK, "vpx_codec_enc_config_default");

  // copy the configuration below mostly from WebRTC (libvpx_vp9_encoder.cc)
  cfg_.g_w = display_width_;
  cfg_.g_h = display_height_;
  cfg_.g_timebase.num = 1;
  cfg_.g_timebase.den = frame_rate_; // WebRTC uses a 90 kHz clock
  cfg_.g_pass = VPX_RC_ONE_PASS;
  cfg_.g_lag_in_frames = 0; // disable lagged encoding
  // WebRTC disables error resilient mode unless for SVC
  cfg_.g_error_resilient = VPX_ERROR_RESILIENT_DEFAULT;
  cfg_.g_threads = 4; // encoder threads; should equal to column tiles below
  cfg_.rc_resize_allowed = 0; // WebRTC enables spatial sampling
  cfg_.rc_dropframe_thresh = 0; // WebRTC sets to 30 (% of target data buffer)
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_buf_optimal_sz = 600;
  cfg_.rc_buf_sz = 1000;
  cfg_.rc_min_quantizer = 2;
  cfg_.rc_max_quantizer = 52;
  cfg_.rc_undershoot_pct = 50;
  cfg_.rc_overshoot_pct = 50;

  // prevent libvpx encoder from automatically placing key frames
  cfg_.kf_mode = VPX_KF_DISABLED;
  // WebRTC sets the two values below to 3000 frames (fixed keyframe interval)
  cfg_.kf_max_dist = numeric_limits<unsigned int>::max();
  cfg_.kf_min_dist = 0;

  cfg_.rc_end_usage = VPX_CBR;
  cfg_.rc_target_bitrate = target_bitrate_;

  // use no more than 16 or the number of avaialble CPUs
  const unsigned int cpu_used = min(get_nprocs(), 16);

  // more encoder settings
  check_call(vpx_codec_enc_init(&context_, &vpx_codec_vp9_cx_algo, &cfg_, 0),
             VPX_CODEC_OK, "vpx_codec_enc_init");

  // this value affects motion estimation and *dominates* the encoding speed
  codec_control(&context_, VP8E_SET_CPUUSED, cpu_used);

  // enable encoder to skip static/low content blocks
  codec_control(&context_, VP8E_SET_STATIC_THRESHOLD, 1);

  // clamp the max bitrate of a keyframe to 900% of average per-frame bitrate
  codec_control(&context_, VP8E_SET_MAX_INTRA_BITRATE_PCT, 900);

  // enable encoder to adaptively change QP for each segment within a frame
  codec_control(&context_, VP9E_SET_AQ_MODE, 3);

  // set the number of column tiles in encoding a frame to 2 ** 2 = 4
  codec_control(&context_, VP9E_SET_TILE_COLUMNS, 2);

  // enable row-based multi-threading
  codec_control(&context_, VP9E_SET_ROW_MT, 1);

  // disable frame parallel decoding
  codec_control(&context_, VP9E_SET_FRAME_PARALLEL_DECODING, 0);

  // enable denoiser (but not on ARM since optimization is pending)
  codec_control(&context_, VP9E_SET_NOISE_SENSITIVITY, 1);

  cerr << "Initialized VP9 encoder (CPU used: " << cpu_used << ")" << endl;
}

Encoder::~Encoder()
{
  if (vpx_codec_destroy(&context_) != VPX_CODEC_OK) {
    cerr << "~Encoder(): failed to destroy VPX encoder context" << endl;
  }
}

void Encoder::compress_frame(const RawImage & raw_img)
{
  const auto frame_generation_ts = timestamp_us();

  // encode raw_img into frame 'frame_id_'
  encode_frame(raw_img);

  // packetize frame 'frame_id_' into datagrams
  const size_t frame_size = packetize_encoded_frame();

  // output frame information
  if (output_fd_) {
    const auto frame_encoded_ts = timestamp_us();

    output_fd_->write(to_string(frame_id_) + "," +
                      to_string(target_bitrate_) + "," +
                      to_string(frame_size) + "," +
                      to_string(frame_generation_ts) + "," +
                      to_string(frame_encoded_ts) + "\n");
  }

  // move onto the next frame
  frame_id_++;
}

void Encoder::encode_frame(const RawImage & raw_img)
{
  if (raw_img.display_width() != display_width_ or
      raw_img.display_height() != display_height_) {
    throw runtime_error("Encoder: image dimensions don't match");
  }

  // check if a key frame needs to be encoded
  vpx_enc_frame_flags_t encode_flags = 0; // normal frame
  if (not unacked_.empty()) {
    const auto & first_unacked = unacked_.cbegin()->second;

    // give up if first unacked datagram was initially sent MAX_UNACKED_US ago
    const auto us_since_first_send = timestamp_us() - first_unacked.send_ts;

    if (us_since_first_send > MAX_UNACKED_US) {
      encode_flags = VPX_EFLAG_FORCE_KF; // force next frame to be key frame

      cerr << "* Recovery: gave up retransmissions and forced a key frame "
           << frame_id_ << endl;

      if (verbose_) {
        cerr << "Giving up on lost datagram: frame_id="
             << first_unacked.frame_id << " frag_id=" << first_unacked.frag_id
             << " rtx=" << first_unacked.num_rtx
             << " us_since_first_send=" << us_since_first_send << endl;
      }

      // clean up
      send_buf_.clear();
      unacked_.clear();
    }
  }

  // encode a frame and calculate encoding time
  const auto encode_start = steady_clock::now();
  check_call(vpx_codec_encode(&context_, raw_img.get_vpx_image(), frame_id_, 1,
                              encode_flags, VPX_DL_REALTIME),
             VPX_CODEC_OK, "failed to encode a frame");
  const auto encode_end = steady_clock::now();
  const double encode_time_ms = duration<double, milli>(
                                encode_end - encode_start).count();

  // track stats in the current period
  num_encoded_frames_++;
  total_encode_time_ms_ += encode_time_ms;
  max_encode_time_ms_ = max(max_encode_time_ms_, encode_time_ms);
}

size_t Encoder::packetize_encoded_frame()
{
  // read the encoded frame's "encoder packets" from 'context_'
  const vpx_codec_cx_pkt_t * encoder_pkt;
  vpx_codec_iter_t iter = nullptr;
  unsigned int frames_encoded = 0;
  size_t frame_size = 0;

  while ((encoder_pkt = vpx_codec_get_cx_data(&context_, &iter))) {
    if (encoder_pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
      frames_encoded++;

      // there should be exactly one frame encoded
      if (frames_encoded > 1) {
        throw runtime_error("Multiple frames were encoded at once");
      }

      frame_size = encoder_pkt->data.frame.sz;
      assert(frame_size > 0);

      // read the returned frame type
      auto frame_type = FrameType::NONKEY;
      if (encoder_pkt->data.frame.flags & VPX_FRAME_IS_KEY) {
        frame_type = FrameType::KEY;

        if (verbose_) {
          cerr << "Encoded a key frame: frame_id=" << frame_id_ << endl;
        }
      }

      // total fragments to divide this frame into
      const uint16_t frag_cnt = narrow_cast<uint16_t>(
          frame_size / (Datagram::max_payload + 1) + 1);

      // next address to copy compressed frame data from
      uint8_t * buf_ptr = static_cast<uint8_t *>(encoder_pkt->data.frame.buf);
      const uint8_t * const buf_end = buf_ptr + frame_size;

      for (uint16_t frag_id = 0; frag_id < frag_cnt; frag_id++) {
        // calculate payload size and construct the payload
        const size_t payload_size = (frag_id < frag_cnt - 1) ?
            Datagram::max_payload : buf_end - buf_ptr;

        // enqueue a datagram
        send_buf_.emplace_back(frame_id_, frame_type, frag_id, frag_cnt,
          string_view {reinterpret_cast<const char *>(buf_ptr), payload_size});

        buf_ptr += payload_size;
      }
    }
  }

  return frame_size;
}

void Encoder::add_unacked(const Datagram & datagram)
{
  const auto seq_num = make_pair(datagram.frame_id, datagram.frag_id);
  auto [it, success] = unacked_.emplace(seq_num, datagram);

  if (not success) {
    throw runtime_error("datagram already exists in unacked");
  }

  it->second.last_send_ts = it->second.send_ts;
}

void Encoder::add_unacked(Datagram && datagram)
{
  const auto seq_num = make_pair(datagram.frame_id, datagram.frag_id);
  auto [it, success] = unacked_.emplace(seq_num, move(datagram));

  if (not success) {
    throw runtime_error("datagram already exists in unacked");
  }

  it->second.last_send_ts = it->second.send_ts;
}

void Encoder::handle_ack(const shared_ptr<AckMsg> & ack)
{
  const auto curr_ts = timestamp_us();

  // observed an RTT sample
  add_rtt_sample(curr_ts - ack->send_ts);

  // find the acked datagram in 'unacked_'
  const auto acked_seq_num = make_pair(ack->frame_id, ack->frag_id);
  auto acked_it = unacked_.find(acked_seq_num);

  if (acked_it == unacked_.end()) {
    // do nothing else if ACK is not for an unacked datagram
    return;
  }

  // retransmit all unacked datagrams before the acked one (backward)
  for (auto rit = make_reverse_iterator(acked_it);
       rit != unacked_.rend(); rit++) {
    auto & datagram = rit->second;

    // skip if a datagram has been retransmitted MAX_NUM_RTX times
    if (datagram.num_rtx >= MAX_NUM_RTX) {
      continue;
    }

    // retransmit if it's the first RTX or the last RTX was about one RTT ago
    if (datagram.num_rtx == 0 or
        curr_ts - datagram.last_send_ts > ewma_rtt_us_.value()) {
      datagram.num_rtx++;
      datagram.last_send_ts = curr_ts;

      // retransmissions are more urgent
      send_buf_.emplace_front(datagram);
    }
  }

  // finally, erase the acked datagram from 'unacked_'
  unacked_.erase(acked_it);
}

void Encoder::add_rtt_sample(const unsigned int rtt_us)
{
  // min RTT
  if (not min_rtt_us_ or rtt_us < *min_rtt_us_) {
    min_rtt_us_ = rtt_us;
  }

  // EWMA RTT
  if (not ewma_rtt_us_) {
    ewma_rtt_us_ = rtt_us;
  } else {
    ewma_rtt_us_ = ALPHA * rtt_us + (1 - ALPHA) * (*ewma_rtt_us_);
  }
}

void Encoder::output_periodic_stats()
{
  cerr << "Frames encoded in the last ~1s: " << num_encoded_frames_ << endl;

  if (num_encoded_frames_ > 0) {
    cerr << "  - Avg/Max encoding time (ms): "
         << double_to_string(total_encode_time_ms_ / num_encoded_frames_)
         << "/" << double_to_string(max_encode_time_ms_) << endl;
  }

  if (min_rtt_us_ and ewma_rtt_us_) {
    cerr << "  - Min/EWMA RTT (ms): " << double_to_string(*min_rtt_us_ / 1000.0)
         << "/" << double_to_string(*ewma_rtt_us_ / 1000.0) << endl;
  }

  // reset all but RTT-related stats
  num_encoded_frames_ = 0;
  total_encode_time_ms_ = 0.0;
  max_encode_time_ms_ = 0.0;
}

void Encoder::set_target_bitrate(const unsigned int bitrate_kbps)
{
  target_bitrate_ = bitrate_kbps;

  cfg_.rc_target_bitrate = target_bitrate_;
  check_call(vpx_codec_enc_config_set(&context_, &cfg_),
             VPX_CODEC_OK, "set_target_bitrate");
}
