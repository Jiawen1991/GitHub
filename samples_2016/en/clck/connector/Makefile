# GCC version 4.9.1 or later must be used to successfully use this
# sample.
CXX = g++
CXXFLAGS = -g -fPIC --std=c++11 -Wall
LDFLAGS = -shared -fpic

# Even when using the Intel(R) C++ Compiler, GCC version 4.9.1 or
# later must be installed to successfully use this sample.  If an
# older GCC version is used, the sample may compile, but will produce
# a runtime error when the connector extension is loaded.  If g++
# version 4.9.1 or later is not in your PATH, use the -gxx-name option
# to specify the location of where g++ version 4.9.1 or later is
# installed.
#CXX = icpc
#CXXFLAGS = -g -fPIC --std=c++11 -Wall
#CXXFLAGS += -gxx-name=/opt/gcc-4.9.1/bin/g++
#LDFLAGS = -static-intel -wd10237 -shared -fpic

all : libduck.so

clean :
	rm -f duck.o libduck.so

libduck.so : duck.o
	$(CXX) $(LDFLAGS) $^ -o $@

.cpp.o :
	$(CXX) $(CXXFLAGS) -c $< -o $@
