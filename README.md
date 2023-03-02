# Ringmaster

Ringmaster is a videoconferencing research platform open-sourced along with our paper
published at NSDI '23 — [Tambur: Efficient loss recovery for videoconferencing via streaming
codes](https://www.usenix.org/conference/nsdi23/presentation/rudow), where
Ringmaster serves as the basis for developing and benchmarking forward error
correction (FEC) schemes in videoconferencing released [here](https://github.com/Thesys-lab/tambur).

Ringmaster is designed to be a readable and extensible replacement for WebRTC in videoconferencing
research, with the goal of supporting more use cases in the future
(e.g., congestion control, multiparty conferencing). [Contributions](#contributing) are welcome.

See [below](#emulating-a-video-call) for the basic usage of Ringmaster that emulates a 1:1
video call. More [documentation](https://github.com/microsoft/ringmaster/wiki/Documentation) is in
progress. Please contact the owner [Francis Yan](https://francisyyan.org) with any questions for now.

## Dependencies
- Required environment: Ubuntu >=18.04
- Install required packages
   ```
   sudo apt install autoconf libvpx-dev libsdl2-dev
   ```

## Building
Compile Ringmaster with
```
./autogen.sh
./configure
make -j
```

## Emulating a video call
Download a sample raw video
[ice_4cif_30fps.y4m](https://media.xiph.org/video/derf/y4m/ice_4cif_30fps.y4m),
which has a resolution of 704x576 and a frame rate of 30 fps.

Next, go to `src/app` and execute the following commands in two terminals, respectively
(run them without any arguments to see the usage):
```
./video_sender 12345 ice_4cif_30fps.y4m
./video_receiver 127.0.0.1 12345 704 576 --fps 30 --cbr 500
```
This emulates a 1:1 video call, where the caller compresses the raw video into VP9-encoded
video frames with an average bitrate of 500 kbps and transmits the packetized frames to the
callee over UDP.

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
