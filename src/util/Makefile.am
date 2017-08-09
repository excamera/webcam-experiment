AM_CPPFLAGS = $(CXX14_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

noinst_LIBRARIES = libutil.a

libutil_a_SOURCES = chunk.hh exception.hh \
	file.hh file.cc file_descriptor.hh \
	mmap_region.hh mmap_region.cc \
	child_process.hh child_process.cc \
	signalfd.hh signalfd.cc \
	system_runner.hh system_runner.cc \
	2d.hh raster.hh raster.cc
