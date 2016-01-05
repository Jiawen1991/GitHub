# Source the Intel(R) Cluster Checker and Intel(R) MPI Library environment
# scripts before using this Makefile
CLCK_DB=$$HOME/.clck/3.1.0/clck.db
CC = mpiicc
CXX = mpiicpc
FC = mpiifort
CFLAGS = -Wall
CXXFLAGS = -std=c++11 -Wall
FFLAGS = -warn
LDFLAGS = -Wl,-rpath,$(CLCK_ROOT)/lib/intel64
LIBS = -lclck

all : mpi-c mpi-cpp mpi-f77 mpi-f90

clean :
	$(RM) clck.o mpi-c.o mpi-cpp.o mpi-f77.o mpi-f90.o \
	mpi-c mpi-cpp mpi-f77 mpi-f90

clck.o : clck.cpp
	$(CXX) $(CXXFLAGS) -DCLCK_DB=$(CLCK_DB) -DCLCK_ROOT=$(CLCK_ROOT) -c clck.cpp -o clck.o

mpi-c.o : clck.o mpi.c
	$(CC) $(CFLAGS) -c mpi.c -o mpi-c.o

mpi-cpp.o : mpi.cpp
	$(CXX) $(CXXFLAGS) -c mpi.cpp -o mpi-cpp.o

mpi-f77.o : mpi.f
	$(FC) $(FFLAGS) -c mpi.f -o mpi-f77.o

mpi-f90.o : mpi.f90
	$(FC) $(FFLAGS) -c mpi.f90 -o mpi-f90.o

mpi-c : clck.o mpi-c.o
	$(CC) $(LDFLAGS) -o mpi-c clck.o mpi-c.o $(LIBS)

mpi-cpp : clck.o mpi-cpp.o
	$(CXX) $(LDFLAGS) -o mpi-cpp clck.o mpi-cpp.o $(LIBS)

mpi-f77: clck.o mpi-f77.o
	$(FC) $(LDFLAGS) -o mpi-f77 clck.o mpi-f77.o $(LIBS)

mpi-f90 : clck.o mpi-f90.o
	$(FC) $(LDFLAGS) -o mpi-f90 clck.o mpi-f90.o $(LIBS)
