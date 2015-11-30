HEADERS=../common/basic.hpp ../common/cmdparser.hpp ../common/oclobject.hpp
SOURCES=BitonicSort.cpp ../common/basic.cpp ../common/cmdparser.cpp ../common/oclobject.cpp

ifeq ($(CONFIG),debug)
	OPT =-O0 -g
else
	OPT =
endif

all: bitonic_sort

bitonic_sort: $(HEADERS) $(SOURCES) Makefile
	g++ $(SOURCES) -I../common -lOpenCL -obitonic_sort -std=gnu++0x $(OPT)

clean:
	rm -f bitonic_sort

