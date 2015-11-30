HEADERS=../common/basic.hpp ../common/cmdparser.hpp ../common/oclobject.hpp ../common/utils.h
SOURCES=GodRays.cpp GodRaysNative.cpp ../common/basic.cpp ../common/cmdparser.cpp ../common/oclobject.cpp ../common/utils.cpp

ifeq ($(CONFIG),debug)
	OPT =-O0 -g
else
	OPT =
endif

all: godrays

godrays: $(HEADERS) $(SOURCES) Makefile
	g++ $(SOURCES) -I../common -lOpenCL -ogodrays -std=gnu++0x $(OPT)

clean:
	rm -f godrays

