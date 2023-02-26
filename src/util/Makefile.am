AM_CPPFLAGS = $(CXX17_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

noinst_LIBRARIES = libutil.a

libutil_a_SOURCES = \
	exception.hh \
	conversion.hh conversion.cc \
	split.hh split.cc \
	mmap.hh mmap.cc \
	timestamp.hh timestamp.cc \
	timerfd.hh timerfd.cc \
	address.hh address.cc \
	serialization.hh serialization.cc \
	poller.hh poller.cc \
	epoller.hh epoller.cc \
	file_descriptor.hh file_descriptor.cc \
	socket.hh socket.cc \
	udp_socket.hh udp_socket.cc \
	tcp_socket.hh tcp_socket.cc
