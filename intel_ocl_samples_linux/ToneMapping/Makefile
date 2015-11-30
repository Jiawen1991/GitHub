HEADERS=../common/basic.hpp ../common/cmdparser.hpp ../common/oclobject.hpp ../common/utils.h stdafx.h
SOURCES=ToneMapping.cpp ToneMappingNative.cpp ../common/basic.cpp ../common/cmdparser.cpp ../common/oclobject.cpp ../common/utils.cpp

ifeq ($(CONFIG),debug)
	OPT =-O0 -g
else
	OPT =
endif

all: tone_mapping

tone_mapping: $(HEADERS) $(SOURCES) Makefile
	g++ $(SOURCES) -I../common -lOpenCL -otone_mapping -std=gnu++0x $(OPT)

clean:
	rm -f tone_mapping

