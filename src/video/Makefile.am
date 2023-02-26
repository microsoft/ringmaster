AM_CPPFLAGS = $(CXX17_FLAGS) -I$(srcdir)/../util
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

noinst_LIBRARIES = libvideo.a

libvideo_a_SOURCES = \
	image.hh image.cc \
	video_input.hh \
	yuv4mpeg.hh yuv4mpeg.cc \
	v4l2.hh v4l2.cc \
	sdl.hh sdl.cc

bin_PROGRAMS = webcam

webcam_SOURCES = webcam.cc
webcam_LDADD = libvideo.a ../util/libutil.a $(VPX_LIBS) $(SDL_LIBS) -lpthread
