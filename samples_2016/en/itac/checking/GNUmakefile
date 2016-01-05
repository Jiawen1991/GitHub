###################################################################
#
# The programs in this directory demonstrate different error
# scenarios and how these are detected by
#
#               ITAC MPI Correctness  Checking.
#
# The following make targets are provided:
# - all: compile all binaries
# - run: run all binaries with 4 processes
# - print: provide a list of all binaries with a short description
# - <binary>: compile just that binary
# - run_<binary>: run just that binary
#
# Every program runs with an arbitrary number of processes.
# In order to do error checking, the LD_LIBRARY_PATH must include
# libVTmc.so. This can be set either manually or with the
# itacvars.sh/csh scripts.
#
###################################################################

# settings, can be overriden on the make command line
CC = mpicc
CFLAGS = -g
F77 = mpif77
FFLAGS = -g
MPIRUN = env MPIEXEC_TIMEOUT=120 mpiexec
MPIRUN_ARGS = -genv LD_PRELOAD libVTmc.so -genv VT_DEADLOCK_TIMEOUT 20s -genv VT_DEADLOCK_WARNING 25s
NP = -n 4

# find all example programs
SOURCES = $(shell find * -name '*.c' -o -name '*.f')
PROGS   = $(basename $(SOURCES))

# compile everything by default
all: $(PROGS)

# remove intermediate files
clean:
	rm -f $(PROGS)
	find . -name '*.prot' | xargs -r rm

# show what examples we have, using the second comment block as
# description
print:
	@ for source in $(SOURCES); do \
		echo $$source | sed -e 's/\.[cf]/:/'; \
	        perl -e '$$_ = join("", <>); s;/(?:\*.*?All Rights Reserved.*?\*/)?.*?(/\*.*?\*/).*;$$1;s; s/^/   /mg; print;' $$source; \
                echo; \
	done

# running one or all programs
run: $(PROGS:%=run_%)
$(PROGS:%=run_%) : run_% : %
	- cd $(dir $*); $(MPIRUN) $(MPIRUN_ARGS) $(NP) `pwd`/$(notdir $*)

