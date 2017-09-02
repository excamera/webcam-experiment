AM_CPPFLAGS = -I$(srcdir)/../util -I$(srcdir)/../input \
              $(CXX14_FLAGS) $(PULSE_CFLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = audiowriter audioreader

audiowriter_SOURCES = audiowriter.cc
audiowriter_LDADD = ../util/libutil.a ../input/libinput.a $(PULSE_LIBS)

audioreader_SOURCES = audioreader.cc
audioreader_LDADD = ../util/libutil.a ../input/libinput.a $(PULSE_LIBS)
