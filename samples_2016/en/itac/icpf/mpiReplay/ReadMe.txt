An mpiReplay library for Intel(R) Trace Analyzer CPF

Introduction
_________________________
This is a prototype library for CPF that allows replaying MPI events back from a
trace to a source file. The library allows producing a source file with MPI events
happened in the original application which produced the trace. It is possible to
choose only the necessary events, such as P2P or Collective operations.

Using mpiReplay library
_________________________
traceanalyzer --cli --icpf [options] <tracefile> --simulator <simulator libraray> [simulator options]
where [simulator options] are:
        --func <type of processing messages> - change functions for processing, P2P,
            COLLOP, SINGLERANK. You can change a few options (for example: --func P2P --func COLLOP)
        --system <operating system> - change host operating system for the output source file,
            Linux or Windows
        --nosleep - disable processing non-MPI events 
        --filename <name> - an ouput source file name
        --help - prints help and exit

Output files
_________________________
The output source files are written into the directory with the trace file.
mpiReplay.cpp - this is the main source file
mpiReplay.rank0.cpp, mpiReplay.rank1.cpp etc - these are secondary source files with
events for different ranks
mpiReplay.ProcessedFunctions.txt - this is a file with the list of processed functions
mpiReplay.0.raw, mpiReplay.1.raw - these are files with raw data. They can be useful
for getting time of the desired MPI function.


Compiling output source files
_________________________
You can compile the obtained source files using Intel MPI and Intel C++ Compiler
with the following command line:

mpicxx mpiReplay.cpp mpiReplay.rank0.cpp mpiReplay.rank1.cpp -o mpiReplay
or
mpicxx mpiReplay.cpp mpiReplay.rank* -o mpiReplay

Note. If you obtained source files from a large trace, the size of these files can be big,
and they can take much time to compile.




