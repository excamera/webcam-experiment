AM_CPPFLAGS = -I$(srcdir)/../util -I$(srcdir)/../display -I$(srcdir)/../input -I$(srcdir)/../capture $(XCBPRESENT_CFLAGS) $(XCB_CFLAGS) $(CXX14_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = my-camera

my_camera_SOURCES = my-camera.cc
my_camera_LDADD = -ldl -lm ../capture/libcapture.a ../input/libinput.a ../display/libdisplay.a ../util/libutil.a $(XCBPRESENT_LIBS) $(XCB_LIBS) $(PANGOCAIRO_LIBS) $(AVFORMAT_LIBS) $(AVCODEC_LIBS) $(AVUTIL_LIBS) $(AVFILTER_LIBS) $(AVDEVICE_LIBS) $(GLU_LIBS) $(GLEW_LIBS) $(GLFW3_LIBS)
my_camera_LDFLAGS = -pthread
