AM_CPPFLAGS = -I$(srcdir)/../util -I$(srcdir)/../capture $(CXX11_FLAGS) $(PULSE_CFLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS) $(NODEBUG_CXXFLAGS)

noinst_LIBRARIES = libinput.a

libinput_a_SOURCES = frame_input.hh \
                     camera.hh camera.cc \
                     audio.hh audio.cc
