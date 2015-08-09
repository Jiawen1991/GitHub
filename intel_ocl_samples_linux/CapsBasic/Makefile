ifeq ($(CONFIG),debug)
	OPT =-O0 -g
else
	OPT =
endif

all: capsbasic

capsbasic: capsbasic.cpp Makefile
	g++ capsbasic.cpp -lOpenCL -ocapsbasic $(OPT)

clean:
	rm -f capsbasic
