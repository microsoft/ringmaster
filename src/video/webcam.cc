#include <getopt.h>
#include <iostream>
#include <string>

#include "image.hh"
#include "v4l2.hh"
#include "sdl.hh"
#include "conversion.hh"
#include "poller.hh"

using namespace std;

void print_usage(const string & program_name)
{
  cerr <<
  "Usage: " << program_name << " [options] width height\n\n"
  "Options:\n"
  "-d, --device <video_device>    webcam device (default: /dev/video0)"
  << endl;
}

int main(int argc, char * argv[])
{
  string video_device_path = "/dev/video0";

  const option cmd_line_opts[] = {
    {"device", required_argument, nullptr, 'd'},
    { nullptr, 0,                 nullptr,  0 },
  };

  while (true) {
    const int opt = getopt_long(argc, argv, "d:", cmd_line_opts, nullptr);
    if (opt == -1) {
      break;
    }

    switch (opt) {
      case 'd':
        video_device_path = optarg;
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

  const uint16_t width = narrow_cast<uint16_t>(strict_stoi(argv[optind]));
  const uint16_t height = narrow_cast<uint16_t>(strict_stoi(argv[optind + 1]));

  // create a video device and set it to non-blocking
  VideoDevice cam(video_device_path, width, height);
  cam.set_blocking(false);

  // create an SDL display
  VideoDisplay display(width, height);

  // allocate a raw image
  RawImage raw_img(width, height);

  // poll video frames from camera
  Poller poller;

  poller.register_event(cam.fd(), Poller::In,
    [&]()
    {
      if (not cam.read_frame(raw_img)) {
        cerr << "try to read frame from camera later" << endl;
        return;
      }

      display.show_frame(raw_img);
    }
  );

  while (not display.signal_quit()) {
    poller.poll(-1);
  }

  return EXIT_SUCCESS;
}
