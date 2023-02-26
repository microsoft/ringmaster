#ifndef DECODER_HH
#define DECODER_HH

extern "C" {
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>
}

#include <map>
#include <vector>
#include <deque>
#include <optional>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "protocol.hh"
#include "sdl.hh"
#include "file_descriptor.hh"

// decoder's view of a video frame
class Frame
{
public:
  Frame(const uint32_t frame_id,
        const FrameType frame_type,
        const uint16_t frag_cnt);

  // if the frame has fragment 'frag_id'
  bool has_frag(const uint16_t frag_id) const;

  // get fragment 'frag_id'
  Datagram & get_frag(const uint16_t frag_id);
  const Datagram & get_frag(const uint16_t frag_id) const;

  // insert a fragment into the frame
  void insert_frag(const Datagram & datagram);
  void insert_frag(Datagram && datagram);

  // if the frame has received all fragments
  bool complete() const { return null_frags_ == 0; }
  std::optional<size_t> frame_size() const;

  // accessors
  uint32_t id() const { return id_; }
  FrameType type() const { return type_; }

  std::vector<std::optional<Datagram>> & frags() { return frags_; }
  const std::vector<std::optional<Datagram>> & frags() const { return frags_; }

  unsigned int null_frags() const { return null_frags_; }

private:
  uint32_t id_;    // frame ID
  FrameType type_; // frame type

  std::vector<std::optional<Datagram>> frags_; // fragments of this frame
  unsigned int null_frags_; // number of uninitialized fragments
  size_t frame_size_ {0}; // frame size so far

  // validate if a datagram belongs to this frame
  void validate_datagram(const Datagram & datagram) const;
};

class Decoder
{
public:
  enum LazyLevel : int {
    DECODE_DISPLAY = 0,   // decode and display
    DECODE_ONLY = 1,      // decode only but not display
    NO_DECODE_DISPLAY = 2 // neither decode nor display
  };

  Decoder(const uint16_t display_width,
          const uint16_t display_height,
          const int lazy_level = 0,
          const std::string & output_path = "");

  // add a received datagram
  void add_datagram(const Datagram & datagram);
  void add_datagram(Datagram && datagram);

  // is next frame complete; might skip to a complete key frame ahead
  bool next_frame_complete();

  // depending on the lazy level, might decode and display the next frame
  void consume_next_frame();

  // output stats every second and reset
  void output_periodic_stats();

  // accessors
  uint32_t next_frame() const { return next_frame_; }

  // mutators
  void set_verbose(const bool verbose) { verbose_ = verbose; }

  // forbid copying and moving
  Decoder(const Decoder & other) = delete;
  const Decoder & operator=(const Decoder & other) = delete;
  Decoder(Decoder && other) = delete;
  Decoder & operator=(Decoder && other) = delete;

private:
  // initialized before worker thread starts and won't be modified again
  uint16_t display_width_;
  uint16_t display_height_;
  LazyLevel lazy_level_;
  std::optional<FileDescriptor> output_fd_; // only one thread should output
  std::chrono::time_point<std::chrono::steady_clock> decoder_epoch_;

  // print debugging info
  bool verbose_ {false};

  // next frame ID to decode
  uint32_t next_frame_ {0};

  // frame ID => class Frame
  std::map<uint32_t, Frame> frame_buf_ {};

  // performance stats
  unsigned int num_decodable_frames_ {0};
  size_t total_decodable_frame_size_ {0}; // bytes
  std::chrono::time_point<std::chrono::steady_clock> last_stats_time_ {};

  // shared between main (Decoder) and worker threads
  std::mutex mtx_ {};
  std::condition_variable cv_ {};
  std::deque<Frame> shared_queue_ {};

  // worker thread for decoding and displaying frames
  std::thread worker_ {};

  // common code between the two versions of add_datagram()
  bool add_datagram_common(const Datagram & datagram);

  // advance next frame ID by 'n'
  void advance_next_frame(const unsigned int n = 1);

  // clean up states (such as frame_buf_) up to frame 'frontier'
  void clean_up_to(const uint32_t frontier);

  // worker thread calls the functions below
  double decode_frame(vpx_codec_ctx_t & context, const Frame & frame);
  void display_decoded_frame(vpx_codec_ctx_t & context, VideoDisplay & display);
  void worker_main();
};

#endif /* DECODER_HH */
