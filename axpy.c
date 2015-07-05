/*
 * AXPY  Y[N] = Y[N] + a*X[N]
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/timeb.h>

#include <pthread.h>

/* read timer in second */
double read_timer() {
    struct timeb tm;
    ftime(&tm);
    return (double) tm.time + (double) tm.millitm / 1000.0;
}

/* read timer in ms */
double read_timer_ms() {
    struct timeb tm;
    ftime(&tm);
    return (double) tm.time * 1000.0 + (double) tm.millitm;
}

#define REAL float
#define VECTOR_LENGTH 102400

/* initialize a vector with random floating point numbers */
void init(REAL A[], int N) {
    int i;
    for (i = 0; i < N; i++) {
        A[i] = (double) drand48();
    }
}

double check(REAL A[], REAL B[], int N) {
    int i;
    double sum = 0.0;
    for (i = 0; i < N; i++) {
        sum += A[i] - B[i];
    }
    return sum;
}

void axpy_base(int N, REAL Y[], REAL X[], REAL a);

void axpy_base_sub(int i_start, int Nt, int N, REAL Y[], REAL X[], REAL a);

void axpy_dist(int N, REAL Y[], REAL X[], REAL a, int num_tasks);
void axpy_pthread(int N, REAL Y[], REAL X[], REAL a, int num_tasks);
void axpy_openmp(int N, REAL Y[], REAL X[], REAL a, int num_tasks);

int main(int argc, char *argv[]) {
    int N = VECTOR_LENGTH;
    int num_tasks = 5; /* 5 is default number of tasks */
    double elapsed; /* for timing */
    double elapsed_pthread; /* for timing */
    double elapsed_openmp; /* for timing */
    if (argc < 2) {
        fprintf(stderr, "Usage: axpy <n> [<#tasks(%d)>]\n", num_tasks);
        exit(1);
    }
    N = atoi(argv[1]);
    if (argc > 2) num_tasks = atoi(argv[2]);
    REAL a = 123.456;
    REAL Y_base[N];
    REAL Y_pthread[N];
    REAL Y_openmp[N];
    REAL X[N];

    srand48((1 << 12));
    init(X, N);
    init(Y_base, N);
    memcpy(Y_pthread, Y_base, N * sizeof(REAL));
    memcpy(Y_openmp, Y_base, N * sizeof(REAL));

    /* example run */
    elapsed = read_timer();
    axpy_base(N, Y_base, X, a);
    elapsed = (read_timer() - elapsed);

    elapsed_pthread = read_timer();
    axpy_pthread(N, Y_pthread, X, a, num_tasks);
    elapsed_pthread = (read_timer() - elapsed_pthread);
    
    elapsed_openmp = read_timer();
    axpy_openmp(N, Y_openmp, X, a, num_tasks);
    elapsed_openmp = (read_timer() - elapsed_openmp);

    /* you should add the call to each function and time the execution */
    printf("======================================================================================================\n");
    printf("\tAXPY: Y[N] = Y[N] + a*X[N], N=%d, %d threads/tasks for dist\n", N, num_tasks);
    printf("------------------------------------------------------------------------------------------------------\n");
    printf("Performance:\t\tRuntime (ms)\t MFLOPS \t\tError (compared to base)\n");
    printf("------------------------------------------------------------------------------------------------------\n");
    printf("axpy_base:\t\t%4f\t%4f \t\t%g\n", elapsed * 1.0e3, (2.0 * N) / (1.0e6 * elapsed), check(Y_base, Y_base, N));
    printf("axpy_pthread:\t\t%4f\t%4f \t\t%g\n", elapsed_pthread * 1.0e3, (2.0 * N) / (1.0e6 * elapsed_pthread), check(Y_base, Y_pthread, N));
    printf("axpy_openmp:\t\t%4f\t%4f \t\t%g\n", elapsed_openmp * 1.0e3, (2.0 * N) / (1.0e6 * elapsed_openmp), check(Y_base, Y_openmp, N));
    return 0;
}

void axpy_base(int N, REAL Y[], REAL X[], REAL a) {
    int i;
    for (i = 0; i < N; ++i)
        Y[i] += a * X[i];
}

void axpy_openmp(int N, REAL Y[], REAL X[], REAL a, int num_tasks) {
    int i;
    #pragma omp parallel for shared(N, X, Y) private(i) num_threads(num_tasks)
    for (i = 0; i < N; ++i)
        Y[i] += a * X[i];
}

void axpy_base_sub(int i_start, int Nt, int N, REAL Y[], REAL X[], REAL a) {
    int i;
    for (i = i_start; i < i_start + Nt; ++i)
        Y[i] += a * X[i];
}

void dist(int tid, int N, int num_tasks, int *Nt, int *start) {
    int remain = N % num_tasks;
    int esize = N / num_tasks;
    if (tid < remain) { /* each of the first remain task has one more element */
        *Nt = esize + 1;
        *start = *Nt * tid;
    } else {
        *Nt = esize;
        *start = esize * tid + remain;
    }
}
void axpy_dist(int N, REAL Y[], REAL X[], REAL a, int num_tasks) {
    int tid;
    for (tid = 0; tid < num_tasks; tid++) {
        int Nt, start;
        dist(tid, N, num_tasks, &Nt, &start);
        axpy_base_sub(start, Nt, N, Y, X, a);
    }
}


struct axpy_pthread_data {
    int Nt;
    int start;
    int N;
    REAL *Y;
    REAL *X;
    REAL a;
};

void * axpy_thread_func(void * axpy_thread_arg) {
    struct axpy_pthread_data * arg = (struct axpy_pthread_data *) axpy_thread_arg;
    axpy_base_sub(arg->start, arg->Nt, arg->N, arg->Y, arg->X, arg->a);
    pthread_exit(NULL);
}

void axpy_pthread(int N, REAL Y[], REAL X[], REAL a, int num_tasks) {
    struct axpy_pthread_data pthread_data_array[num_tasks];
    pthread_t task_threads[num_tasks];
    int tid;
    for (tid = 0; tid < num_tasks; tid++) {
        int Nt, start;
        dist(tid, N, num_tasks, &Nt, &start);
        struct axpy_pthread_data *task_data = &pthread_data_array[tid];
        task_data->start = start;
        task_data->Nt = Nt;
        task_data->a = a;
        task_data->X = X;
        task_data->Y = Y;
        task_data->N = N;

        pthread_create(&task_threads[tid], NULL, axpy_thread_func, (void*)task_data);
    }

    for (tid = 0; tid < num_tasks; tid++) {
        pthread_join(task_threads[tid], NULL);
    }
}
