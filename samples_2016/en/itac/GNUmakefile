###################################################################
#
# This is the GNU makefile for the
# the jacobic-, jacobif-, allpair- and allpairc-testprograms,
# instrumented for the Intel(R) Trace Collector (ITC) library.
# Settings in this file have been chosen to reflect the 
# properties of your platform. Make sure you know what you're
# doing before applying any changes. Read also the 
# README.examples-file for further information.
# This makefile will be called by shellscript palmake.
#
###################################################################

# C and Fortran compiler for MPI programs
CC        = mpicc
F77       = mpif77
ifeq "$(VT_MPI)" "mpich"
CC        = mpicc -shlib
F77       = mpif77 -shlib
endif
ifeq "$(VT_MPI)" "mpt"
CC        = gcc
F77       = gfortran
MPI_LIB   = -lmpi
endif
# C compiler for normal programs
NORMALCC  = gcc
# Fortran compiler for normal programs
NORMALF77 = gfortran
# include flags for C and Fortran for finding VT.h
IFLAGS    = -I$(VT_ROOT)/include -I.
# link flags for C and Fortran for finding VT libs
LFLAGS    = -L$(VT_LIB_DIR)
# compiler flags for C
CFLAGS    = -g   
# compiler flags for Fortran
FFLAGS    = -g  
# libs needed by ITC
LIBS      = $(MPI_LIB) $(VT_ADD_LIBS)
# link flags for MPI programs written in C
CLDFLAGS  = $(LFLAGS) $(TLIB) $(LIBS)
# link flags for MPI programs written in Fortran
FLDFLAGS  = $(LFLAGS) $(TLIB) $(LIBS)

# enable VT API calls in C files
CPPFLAGS += -DV_T

# object files that must be linked into each executable
JACOBI_C_OBJS		= jacobic.o cmdline.o setupmesh.o exchngc.o VTc.o
JACOBI_F_OBJS		= jacobif.o sweep.o exchngf.o diff2d.o twodbase.o decomp.o fndnbr2d.o VTf.o

ALLPAIR_C_OBJS		= allpairc.o
ALLPAIR_F_OBJS		= allpair.o

COUNTERSCOPE_C_OBJS     = counterscopec.o
MPICONSTANTS_OBJS       = mpiconstants.o
TIMERTEST_OBJS          = timertest.o timerperformance.o VT_usleep.o

# default is to build executables with ITC
default: libsvt

# build all executable
all: libsvt libsvn

# targets that build executables of one kind
libsvt: vtjacobic vtjacobif vtallpairc vtallpair vtcounterscopec
libsvn: vnjacobic vnjacobif vnallpairc vnallpair

# explicit dependencies for our targets
vtjacobic: $(JACOBI_C_OBJS)
vtjacobif: $(JACOBI_F_OBJS)
vtallpairc: $(ALLPAIR_C_OBJS)
vtallpair: $(ALLPAIR_F_OBJS)
vtcounterscopec: $(COUNTERSCOPE_C_OBJS)
vttimertest: $(TIMERTEST_OBJS)
vnjacobic: $(JACOBI_C_OBJS)
vnjacobif: $(JACOBI_F_OBJS)
vnallpairc: $(ALLPAIR_C_OBJS)
vnallpair: $(ALLPAIR_F_OBJS)

# tell GNU make where to find these source files
vpath %.c ./jacobi/jacobi-C
vpath %.f ./jacobi/jacobi-Fortran

# to build a certain "flavour" call make recursively with TLIB set to the desired
# library, then rename the result
vt%:
	$(MAKE) $* TLIB=-lVT
	mv $* $@
vn%:
	$(MAKE) $* TLIB=-lVTnull
	mv $* $@

# build rules for object files
%.o: %.f
	$(F77) $(IFLAGS) $(FFLAGS) -c $<

%.o: %.c
	$(CC) $(IFLAGS) $(CPPFLAGS) $(CFLAGS) -c $<

# disable implicit rule for building executables
%: %.o
%: %.c
%: %.f

ifneq "$(TLIB)" ""

# this section is used when make calls itself a second time to link the desired
# executable with the selected version of the ITC library


jacobic: $(JACOBI_C_OBJS)
	$(CC) $+ $(CLDFLAGS) -o $@

jacobif: $(JACOBI_F_OBJS)
	$(F77) $+ $(FLDFLAGS) -o $@

allpairc: $(ALLPAIR_C_OBJS)
	$(CC) $+ $(CLDFLAGS) -o $@

allpair: $(ALLPAIR_F_OBJS)
	$(F77) $+ $(FLDFLAGS) -o $@

counterscopec: $(COUNTERSCOPE_C_OBJS)
	$(CC) $+ $(CLDFLAGS) -o $@

timertest: $(TIMERTEST_OBJS)
	$(CC) $+ $(CLDFLAGS) -o $@

endif # $(TLIB) != ""

mpiconstants: $(MPICONSTANTS_OBJS)
	$(CC) $+ $(CLDFLAGS) -o $@

clean:
	-rm -f *.o

distclean: clean
	-rm -f vt* vn*


###################################################################
#
# The following section builds the examples that use libVTcs.
#
# simplecs and clientserver are MPI applications, but this
# is just the means to spawn processes and do communication.
# No automatic tracing of MPI is done.
#
# forkcs is an example for tracing an application that forks
# and communicates via pipes.
#
###################################################################

CS_MPI_EXAMPLES = simplecs clientserver timersync libVTcs_example
CS_NORMAL_EXAMPLES = forkcs forkcs2 timertestcs

timertestcs : timerperformance.o VT_usleep.o

$(CS_NORMAL_EXAMPLES): % : %.o
	$(NORMALCC) $+ -o $@ $(LFLAGS) -lVTcs $(LIBS)

$(CS_NORMAL_EXAMPLES:%=%.o): %.o : %.c
	$(NORMALCC) -c $< $(IFLAGS) $(CPPFLAGS) $(CFLAGS) -DNO_MPI

$(CS_MPI_EXAMPLES): % : %.o
	$(CC) $< -o $@ $(LFLAGS) -lVTcs $(LIBS)

