#include <getopt.h>
#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>
#include <utility>
#include <chrono>

#include "conversion.hh"
#include "timerfd.hh"
#include "udp_socket.hh"
#include "poller.hh"
#include "yuv4mpeg.hh"
#include "protocol.hh"
#include "encoder.hh"
#include "timestamp.hh"

using namespace std;
using namespace chrono;

// global variables in an unnamed namespace
namespace {
  constexpr unsigned int BILLION = 1000 * 1000 * 1000;
}

void print_usage(const string & program_name)
{
  cerr <<
  "Usage: " << program_name << " [options] port y4m\n\n"
  "Options:\n"
  "--mtu <MTU>                MTU for deciding UDP payload size\n"
  "-o, --output <file>        file to output performance results to\n"
  "-v, --verbose              enable more logging for debugging"
  << endl;
}

pair<Address, ConfigMsg> recv_config_msg(UDPSocket & udp_sock)
{
  // wait until a valid ConfigMsg is received
  while (true) {
    const auto & [peer_addr, raw_data] = udp_sock.recvfrom();

    const shared_ptr<Msg> msg = Msg::parse_from_string(raw_data.value());
    if (msg == nullptr or msg->type != Msg::Type::CONFIG) {
      continue; // ignore invalid or non-config messages
    }

    const auto config_msg = dynamic_pointer_cast<ConfigMsg>(msg);
    if (config_msg) {
      return {peer_addr, *config_msg};
    }
  }
}

int main(int argc, char * argv[])
{
  // argument parsing
  string output_path;
  bool verbose = false;

  const option cmd_line_opts[] = {
    {"mtu",     required_argument, nullptr, 'M'},
    {"output",  required_argument, nullptr, 'o'},
    {"verbose", no_argument,       nullptr, 'v'},
    { nullptr,  0,                 nullptr,  0 },
  };

  while (true) {
    const int opt = getopt_long(argc, argv, "o:v", cmd_line_opts, nullptr);
    if (opt == -1) {
      break;
    }

    switch (opt) {
      case 'M':
        Datagram::set_mtu(strict_stoi(optarg));
        break;
      case 'o':
        output_path = optarg;
        break;
      case 'v':
        verbose = true;
        break;
      default:
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
  }

  if (optind != argc - 2) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  const auto port = narrow_cast<uint16_t>(strict_stoi(argv[optind]));
  const string y4m_path = argv[optind + 1];

  UDPSocket udp_sock;
  udp_sock.bind({"0", port});
  cerr << "Local address: " << udp_sock.local_address().str() << endl;

  // wait for a receiver to send 'ConfigMsg' and "connect" to it
  cerr << "Waiting for receiver..." << endl;

  const auto & [peer_addr, config_msg] = recv_config_msg(udp_sock);
  cerr << "Peer address: " << peer_addr.str() << endl;
  udp_sock.connect(peer_addr);

  // read configuration from the peer
  const auto width = config_msg.width;
  const auto height = config_msg.height;
  const auto frame_rate = config_msg.frame_rate;
  const auto target_bitrate = config_msg.target_bitrate;

  cerr << "Received config: width=" << to_string(width)
       << " height=" << to_string(height)
       << " FPS=" << to_string(frame_rate)
       << " bitrate=" << to_string(target_bitrate) << endl;

  // set UDP socket to non-blocking now
  udp_sock.set_blocking(false);

  // open the video file
  YUV4MPEG video_input(y4m_path, width, height);

  // allocate a raw image
  RawImage raw_img(width, height);

  // initialize the encoder
  Encoder encoder(width, height, frame_rate, output_path);
  encoder.set_target_bitrate(target_bitrate);
  encoder.set_verbose(verbose);

  Poller poller;

  // create a periodic timer with the same period as the frame interval
  Timerfd fps_timer;
  const timespec frame_interval {0, static_cast<long>(BILLION / frame_rate)};
  fps_timer.set_time(frame_interval, frame_interval);

  // read a raw frame when the periodic timer fires
  poller.register_event(fps_timer, Poller::In,
    [&]()
    {
      // being lenient: read raw frames 'num_exp' times and use the last one
      const auto num_exp = fps_timer.read_expirations();
      if (num_exp > 1) {
        cerr << "Warning: skipping " << num_exp - 1 << " raw frames" << endl;
      }

      for (unsigned int i = 0; i < num_exp; i++) {
        // fetch a raw frame into 'raw_img' from the video input
        if (not video_input.read_frame(raw_img)) {
          throw runtime_error("Reached the end of video input");
        }
      }

      // compress 'raw_img' into frame 'frame_id' and packetize it
      encoder.compress_frame(raw_img);

      // interested in socket being writable if there are datagrams to send
      if (not encoder.send_buf().empty()) {
        poller.activate(udp_sock, Poller::Out);
      }
    }
  );

  // when UDP socket is writable
  poller.register_event(udp_sock, Poller::Out,
    [&]()
    {
      deque<Datagram> & send_buf = encoder.send_buf();

      while (not send_buf.empty()) {
        auto & datagram = send_buf.front();

        // timestamp the sending time before sending
        datagram.send_ts = timestamp_us();

        if (udp_sock.send(datagram.serialize_to_string())) {
          if (verbose) {
            cerr << "Sent datagram: frame_id=" << datagram.frame_id
                 << " frag_id=" << datagram.frag_id
                 << " frag_cnt=" << datagram.frag_cnt
                 << " rtx=" << datagram.num_rtx << endl;
          }

          // move the sent datagram to unacked if not a retransmission
          if (datagram.num_rtx == 0) {
            encoder.add_unacked(move(datagram));
          }

          send_buf.pop_front();
        } else { // EWOULDBLOCK; try again later
          datagram.send_ts = 0; // since it wasn't sent successfully
          break;
        }
      }

      // not interested in socket being writable if no datagrams to send
      if (send_buf.empty()) {
        poller.deactivate(udp_sock, Poller::Out);
      }
    }
  );

  // when UDP socket is readable
  poller.register_event(udp_sock, Poller::In,
    [&]()
    {
      while (true) {
        const auto & raw_data = udp_sock.recv();

        if (not raw_data) { // EWOULDBLOCK; try again when data is available
          break;
        }
        const shared_ptr<Msg> msg = Msg::parse_from_string(*raw_data);

        // ignore invalid or non-ACK messages
        if (msg == nullptr or msg->type != Msg::Type::ACK) {
          return;
        }

        const auto ack = dynamic_pointer_cast<AckMsg>(msg);

        if (verbose) {
          cerr << "Received ACK: frame_id=" << ack->frame_id
               << " frag_id=" << ack->frag_id << endl;
        }

        // RTT estimation, retransmission, etc.
        encoder.handle_ack(ack);

        // send_buf might contain datagrams to be retransmitted now
        if (not encoder.send_buf().empty()) {
          poller.activate(udp_sock, Poller::Out);
        }
      }
    }
  );

  // create a periodic timer for outputting stats every second
  Timerfd stats_timer;
  const timespec stats_interval {1, 0};
  stats_timer.set_time(stats_interval, stats_interval);

  poller.register_event(stats_timer, Poller::In,
    [&]()
    {
      if (stats_timer.read_expirations() == 0) {
        return;
      }

      // output stats every second
      encoder.output_periodic_stats();
    }
  );

  // main loop
  while (true) {
    poller.poll(-1);
  }

  return EXIT_SUCCESS;
}
