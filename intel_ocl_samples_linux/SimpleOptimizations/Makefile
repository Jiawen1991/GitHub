HEADERS=../common/basic.hpp ../common/cmdparser.hpp ../common/oclobject.hpp
SOURCES=SimpleOptimizations.cpp ../common/basic.cpp ../common/cmdparser.cpp ../common/oclobject.cpp

ifeq ($(CONFIG),debug)
	OPT =-O0 -g
else
	OPT =
endif

all: simple_optimizations

simple_optimizations: $(HEADERS) $(SOURCES) Makefile
	g++ $(SOURCES) -I../common -lOpenCL -osimple_optimizations -std=gnu++0x $(OPT)

clean:
	rm -f simple_optimizations

