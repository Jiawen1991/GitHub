HEADERS=../common/basic.hpp ../common/cmdparser.hpp ../common/oclobject.hpp ../common/utils.h
SOURCES=MedianFilter.cpp ../common/basic.cpp ../common/cmdparser.cpp ../common/oclobject.cpp ../common/utils.cpp

ifeq ($(CONFIG),debug)
	OPT =-O0 -g
else
	OPT =
endif

all: median_filter

median_filter: $(HEADERS) $(SOURCES) Makefile
	g++ $(SOURCES) -I../common -lOpenCL -omedian_filter -std=gnu++0x $(OPT)

clean:
	rm -f median_filter

