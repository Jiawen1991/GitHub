#PBS -q dirac_reg
#PBS -l nodes=2:ppn=8:fermi
#PBS -l walltime=00:10:00
#PBS -N my_job
#PBS -e my_job.$PBS_JOBID.err
#PBS -o my_job.$PBS_JOBID.out
#PBS -V

cd $PBS_O_WORKDIR
mpirun -np 1 ./jacobi
