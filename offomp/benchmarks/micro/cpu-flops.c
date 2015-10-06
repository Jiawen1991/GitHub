#include <unistd.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

double read_timer() {
    struct timespec ts;
#if defined(CLOCK_MONOTONIC_PRECISE)
	/* BSD. --------------------------------------------- */
	const clockid_t id = CLOCK_MONOTONIC_PRECISE;
#elif defined(CLOCK_MONOTONIC_RAW)
	/* Linux. ------------------------------------------- */
	const clockid_t id = CLOCK_MONOTONIC_RAW;
#elif defined(CLOCK_HIGHRES)
	/* Solaris. ----------------------------------------- */
	const clockid_t id = CLOCK_HIGHRES;
#elif defined(CLOCK_MONOTONIC)
	/* AIX, BSD, Linux, POSIX, Solaris. ----------------- */
	const clockid_t id = CLOCK_MONOTONIC;
#elif defined(CLOCK_REALTIME)
	/* AIX, BSD, HP-UX, Linux, POSIX. ------------------- */
	const clockid_t id = CLOCK_REALTIME;
#else
	const clockid_t id = (clockid_t)-1;	/* Unknown. */
#endif

	if ( id != (clockid_t)-1 && clock_gettime( id, &ts ) != -1 )
		return (double)ts.tv_sec +
			(double)ts.tv_nsec / 1000000000.0;
}

double addmul(double add, double mul, int ops){
	// need to initialise differently otherwise compiler might optimise away
	double sum1=0.1, sum2=-0.1, sum3=0.2, sum4=-0.2, sum5=0.0;
	double mul1=1.0, mul2= 1.1, mul3=1.2, mul4= 1.3, mul5=1.4;
	int loops=ops/10;          // we have 10 floating point ops inside the loop
	double expected = 5.0*add*loops + (sum1+sum2+sum3+sum4+sum5)
			+ pow(mul,loops)*(mul1+mul2+mul3+mul4+mul5);

	int i;
	for(i=0; i<loops; i++) {
		mul1*=mul; mul2*=mul; mul3*=mul; mul4*=mul; mul5*=mul;
		sum1+=add; sum2+=add; sum3+=add; sum4+=add; sum5+=add;
	}
	return  sum1+sum2+sum3+sum4+sum5+mul1+mul2+mul3+mul4+mul5 - expected;
}

double cpu_sustain_gflopss (double * flopss) {
	double x=M_PI;
	double y=1.0+1e-8;
	int n = 1000000;
	double timer = read_timer();
	x=addmul(x,y,n);
	timer = read_timer() - timer;
	*flopss = n/timer/1e9;
}

int main (int argc, char * argv[]) {
  	int ncores  = sysconf( _SC_NPROCESSORS_ONLN );
	double flops, err;
	err = cpu_sustain_gflopss(&flops);
	printf("Hello World, %d cores, per core perf: %f GFLOPS/s, err: %f!\n", ncores, flops, err);
}
