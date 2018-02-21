#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sched.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <assert.h>

#include "gt_include.h"
int policy; 

#define ROWS 256
#define COLS ROWS
#define SIZE COLS
#define GT_THREADS 1
#define NUM_CPUS 4
#define NUM_GROUPS NUM_CPUS
#define PER_GROUP_COLS (SIZE/NUM_GROUPS)

#define NUM_THREADS 128
#define PER_THREAD_ROWS (SIZE/NUM_THREADS)
int policy; 

/* A[SIZE][SIZE] X B[SIZE][SIZE] = C[SIZE][SIZE]
 * Let T(g, t) be thread 't' in group 'g'. 
 * T(g, t) is responsible for multiplication : 
 * A(rows)[(t-1)*SIZE -> (t*SIZE - 1)] X B(cols)[(g-1)*SIZE -> (g*SIZE - 1)] */

typedef struct matrix
{
	int m[SIZE][SIZE];

	int rows;
	int cols;
	unsigned int reserved[2];
} matrix_t;


/*typedef struct timingmeasurements
{
	float runtime;
	float totaltime;
	unsigned int tid;
} uthread_stats_t;*/

typedef struct __uthread_arg
{
	matrix_t *_A, *_B, *_C;
	unsigned int reserved0;
    int matrix_dim;
	unsigned int tid;
	unsigned int gid;
	int start_row; /* start_row -> (start_row + PER_THREAD_ROWS) */
	int start_col; /* start_col -> (start_col + PER_GROUP_COLS) */
	
}uthread_arg_t;

typedef struct uthread_perf_stats // performance of each thread. 
{
	float runtime_mean;
	float std_dev_total;
	float std_dev_run;
	float total_mean;
}uthread_perf_stats_t;
	
struct timeval tv1;

static void generate_matrix(matrix_t *mat, int val)
{

	int i,j;
	mat->rows = SIZE;
	mat->cols = SIZE;
	for(i = 0; i < mat->rows;i++)
		for( j = 0; j < mat->cols; j++ )
		{
			mat->m[i][j] = val;
		}
	return;
}

static void print_matrix(matrix_t *mat)
{
	int i, j;

	for(i=0;i<SIZE;i++)
	{
		for(j=0;j<SIZE;j++)
			printf(" %d ",mat->m[i][j]);
		printf("\n");
	}

	return;
}

static void * uthread_mulmat(void *p)
{
	int i, j, k;
	int count=0;
	int start_row, end_row;
	int start_col, end_col;
	unsigned int cpuid;
	struct timeval tv2;
    int matrix_dim;
   // uthread_stats_t *upstat;
#define ptr ((uthread_arg_t *)p)

	i=0; j= 0; k=0;

	start_row = 0;
	end_row = ptr->matrix_dim;
//#ifdef GT_GROUP_SPLIT
	//start_col = ptr->start_col;
	//end_col = (ptr->start_col + PER_THREAD_ROWS);
//#else
	start_col = 0;
	end_col = ptr->matrix_dim;
	matrix_dim=end_col;
	// printf("the matrix_dim part where the rows and columns were defined.\n");
//#endif

#ifdef GT_THREADS
	cpuid = kthread_cpu_map[kthread_apic_id()]->cpuid;
	fprintf(stderr, "\nThread(id:%d, group:%d, cpu:%d) started",ptr->tid, ptr->gid, cpuid);
#else
	fprintf(stderr, "\nThread(id:%d, group:%d) started",ptr->tid, ptr->gid);
#endif
    
	for(i = start_row; i < end_row; i++)
		{
			for(j = start_col; j < end_col; j++)
			{
				for(k = 0; k < SIZE; k++)
				{
					
						
						if (ptr->tid == 101 && count<10 && policy==1)
						 { 
						 	count++;
						 	gt_yield();
						 	
						 }

					ptr->_C->m[i][j] += ptr->_A->m[i][k] * ptr->_B->m[k][j];
				}
			}
		}

// store here
//upstat->runtime=ptr->run_time+ptr->run_time_temp;
#ifdef GT_THREADS
	fprintf(stderr, "\nThread(id:%d, group:%d, cpu:%d) finished (TIME : %lu s and %lu us)",
			ptr->tid, ptr->gid, cpuid, (tv2.tv_sec - tv1.tv_sec), (tv2.tv_usec - tv1.tv_usec));
#else
	gettimeofday(&tv2,NULL);
	fprintf(stderr, "\nThread(id:%d, group:%d) finished (TIME : %lu s and %lu us)",
			ptr->tid, ptr->gid, (tv2.tv_sec - tv1.tv_sec), (tv2.tv_usec - tv1.tv_usec));
#endif

#undef ptr
	return 0;
}

matrix_t A, B, C;

static void init_matrices()
{
	generate_matrix(&A, 1);
	generate_matrix(&B, 1);
	generate_matrix(&C, 0);

	return;
}


uthread_arg_t uargs[NUM_THREADS];
uthread_t utids[NUM_THREADS];

int main()
{
	
	uthread_arg_t *uarg;
	int inx;
	//int cred; 
	printf("Please enter the scheduler type (1 for credit based scheduler and any other variable for other scheduler): ");
	scanf("%d",&policy);
	gtthread_app_init();
	init_matrices();
    int grp=0;// inititalzing the groups
    int thread_num=0;// initializing the threads
	gettimeofday(&tv1,NULL);
 int weight=25; 
 int size =32;
	if (policy==1)
	{
			for(weight = 25; weight <= 100; weight += 25)
		{
			for(size = 32; size < 257; size *= 2)
			{		
				for(inx=0; inx < 8; inx++)
				{
					uarg = &uargs[thread_num];
					uarg->_A = &A;
					uarg->_B = &B;
					uarg->_C = &C;
					uarg->tid = thread_num;
					uarg->gid = (thread_num % 16);
					uarg->matrix_dim = size;
					//printf("the three for loops in the policy =1 part and the thread number= %d\n", thread_num);
					uthread_create(&utids[thread_num], uthread_mulmat, uarg, uarg->gid, weight);
					thread_num+=1;
				}
				grp++;
			}
		}
	}
	else
	{
    //int size=32;
		for(size = 32; size <= 256; size *= 2)
		{		
        int inx=0; 
				for(inx=0; inx < 64; inx++)
				{
					uarg = &uargs[thread_num];
					uarg->_A = &A;
					uarg->_B = &B;
					uarg->_C = &C;
					uarg->tid = thread_num;
					uarg->gid = (thread_num % 16);
					uarg->matrix_dim = size;
					//printf("the two for loops in default scheduler part\n");
					uthread_create(&utids[thread_num], uthread_mulmat, uarg, uarg->gid, 0);
					thread_num+=1;
				}
				grp++;
			
		}	
	}
	gtthread_app_exit();
	thread_num=0;
	grp=0;
	int thread_pointer=0;
	uthread_perf_stats_t *stats_t = (uthread_perf_stats_t *)calloc(16, sizeof(uthread_perf_stats_t)); // memory creation for stats
	//uthread_time *uthread_time= (uthread_time*)calloc(128, sizeof(uthread_time));
	printf("-------------------Printing Stats for uthreads all measurements are in us----------------------\n");
	printf ("credits     | Size of the matrix   | runtime standard deviation| total time standard deviation|    mean run time   | mean total time\n");
	weight=25;
 int i;
  for (weight=25; weight<=100; weight+=25)
	{
    int matrixsize=32;
		for(matrixsize=32; matrixsize<=256; matrixsize*=2)
		{
			thread_num=thread_pointer;
			for(i=0;i<8;i++)
			{
				assert(thread_num==uthread_time[thread_num]->id);
				stats_t[grp].runtime_mean += (uthread_time[thread_num]->runtime.tv_sec*1000000 + uthread_time[thread_num]->runtime.tv_usec);
				stats_t[grp].total_mean+= (uthread_time[thread_num]->totaltime.tv_sec*1000000 + uthread_time[thread_num]->totaltime.tv_usec);
				thread_num+=16;	
			}
			stats_t[grp].runtime_mean /= 8;
			stats_t[grp].total_mean/=8;
			thread_num=thread_pointer;
			for (i=0;i<8;i++)
			{
				stats_t[grp].std_dev_run+=pow(((uthread_time[thread_num]->runtime.tv_sec*1000000 + uthread_time[thread_num]->runtime.tv_usec)-stats_t[grp].runtime_mean),2);
				stats_t[grp].std_dev_total+=pow(((uthread_time[thread_num]->totaltime.tv_sec*1000000 + uthread_time[thread_num]->totaltime.tv_usec)-stats_t[grp].total_mean),2);
				thread_num+=16;
			}
			stats_t[grp].std_dev_run=sqrt(stats_t[grp].std_dev_run/8);
			stats_t[grp].std_dev_total=sqrt(stats_t[grp].std_dev_total/8);
			
			printf("%d       |%d 				  |%f 	  | %f 	  |%f 	  |%f\n", weight, matrixsize, stats_t[grp].std_dev_run, stats_t[grp].std_dev_total, stats_t[grp].runtime_mean, stats_t[grp].total_mean);
			grp++;
			thread_pointer++;
		}
		
	}	
  return (0);
}
