#define __USE_GNU
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<pthread.h>
#include<sched.h>
#include<x86intrin.h>
#include"libpmem.h"

#define MAX_THREAD_COUNT   (32)

void flush_cache_range(void* start_addr, unsigned int size){
	void * end_addr = start_addr + size;
	asm volatile("":::"memory");
	for(;start_addr < end_addr; start_addr+=64){
		_mm_clflush(start_addr);
	};
	asm volatile("":::"memory");
}

enum rw_mode{
         SEQ_READ,
         SEQ_WRITE,
         RANDOM_READ,
         RANDOM_WRITE
};

typedef struct _rw_parameters{
        unsigned int thread_id;
        unsigned int block_size;
        unsigned int block_count;
        unsigned int media_type;
        unsigned int operation_mode;
        unsigned int *block_array;
        struct timespec start_time;
        struct timespec end_time;
}rw_para;

typedef struct _test_parameters{
        unsigned int thread_count;
        unsigned int block_size;
        unsigned int block_count;
        unsigned int media_type;
        unsigned int operation_mode;
 
}test_para;

typedef struct _test_result{
        double average_latency;
        double total_used_time;
        double single_bandwidth;
        double total_bandwidth;
        struct timespec start_time[MAX_THREAD_COUNT];
        struct timespec end_time[MAX_THREAD_COUNT];
        double used_time[MAX_THREAD_COUNT];
}test_result;

test_result t_result = {0};

void rw_test(rw_para * para ){
	
        struct timespec start_time, end_time;
        void * cache_buffer = malloc(para->block_size);
        memset(cache_buffer,0x10,para->block_size);
        void * src_buffer;
        unsigned int a,b;
	size_t mapped_length;
        memset(cache_buffer,0x10,para->block_size);
        if(para->media_type == 1){
                src_buffer = pmem_map_file("/home/lzm/pmem_disk/test.file" ,\
                                para->block_size*para->block_count,PMEM_FILE_CREATE,0666,&mapped_length,&b);
                if(b==0){
                        perror("pmem error\n");
                };

        }else{
                src_buffer = malloc(para->block_size*para->block_count);
                flush_cache_range(src_buffer,para->block_size*para->block_count);
        };

        memset(src_buffer,0x10, para->block_size*para->block_count);
        flush_cache_range(src_buffer,para->block_size*para->block_count);

        clock_gettime(CLOCK_MONOTONIC,&t_result.start_time[para->thread_id]);
        for(unsigned int i = 0; i < para->block_count; i++){
                memcpy(cache_buffer, src_buffer+i*para->block_size, para->block_size);
		switch(para->operation_mode){
			case SEQ_READ:
                		memcpy(cache_buffer, src_buffer+i*para->block_size, para->block_size);
				break;
			case RANDOM_READ:
                		memcpy(cache_buffer, src_buffer+para->block_array[i]*para->block_size, para->block_size);
				break;
			case SEQ_WRITE:
                		memcpy(src_buffer+i*para->block_size,cache_buffer, para->block_size);
        			flush_cache_range(src_buffer+i*para->block_size,para->block_size);
				break;
			case RANDOM_WRITE:
                		memcpy(src_buffer+para->block_array[i]*para->block_size,cache_buffer, para->block_size);
        			flush_cache_range(src_buffer+para->block_array[i]*para->block_size,para->block_size);
				break;
			default:
				perror("operation mode error !\n");
				;
		};

        };
        clock_gettime(CLOCK_MONOTONIC,&t_result.end_time[para->thread_id]);

        double  used_time  = (t_result.end_time[para->thread_id].tv_sec - t_result.start_time[para->thread_id].tv_sec)*1000000.0 + \
                       (t_result.end_time[para->thread_id].tv_nsec - t_result.start_time[para->thread_id].tv_nsec)/1000.0;
	t_result.used_time[para->thread_id] = used_time;
        used_time = used_time/para->block_count;

        printf("total count = %d , block size is %d,  used time is %lf us\n",\
			para->block_count,para->block_size, used_time);
}


void * thread_task(rw_para * para){

//	cpu_set_t cpu_info;
	//CPU_ZERO(&cpu_info);
	//CPU_SET(0,&cpu_info);
	//CPU_SET(1,&cpu_info);
	
	//if(0!=pthread_attr_setaffinity_np(pthread_self(),sizeof(cpu_set_t),&cpu_info)){
	//	printf("set affinity failed\n");
	//}

	rw_test(para);
}

void run_test(test_para * para){

	pthread_t threads[MAX_THREAD_COUNT];
	rw_para _para[MAX_THREAD_COUNT];

	unsigned int tmp_count = para->block_count;
	tmp_count = tmp_count/para->thread_count;


	for(unsigned int i = 0; i < para->thread_count; i++){
		_para[i].thread_id = i;
		_para[i].media_type = para->media_type;
		_para[i].block_count = para->block_count/para->thread_count;
		_para[i].block_size = para->block_size;
		_para[i].operation_mode = para->operation_mode;
		_para[i].block_array = malloc(tmp_count*sizeof(unsigned int));
		
		for(unsigned int j = 0; j < (para->block_count)/para->thread_count; j++){
			_para[i].block_array[j] = random()%para->block_count;
		};
	};

	for(unsigned int i = 0; i < para->thread_count; i++){
		pthread_create(&threads[i],NULL,(void *)thread_task,&_para[i]);
	};

	for(unsigned int i = 0; i < para->thread_count; i++){
		pthread_join(threads[i],NULL);
	};

	double total_used_time = 0;
	for(unsigned int i = 0; i < para->thread_count; i++){
		total_used_time += t_result.used_time[i];
	};
	printf("total used time is %lf/n",total_used_time);
	t_result.average_latency = total_used_time/para->block_count;
	t_result.single_bandwidth = para->block_size/(t_result.average_latency/1000000.0);

	printf("[result]: average_laency is %lf us\n",t_result.average_latency);
	printf("[result]: single bandwidth is %lf Bytes/s\n",t_result.single_bandwidth);
	
	double tmp_big = 0;
	double little = 100000000000000;
	for(unsigned int i = 0; i<para->thread_count; i++){
		double time_  = (t_result.end_time[i].tv_sec)*1000000.0 + \
                       (t_result.end_time[i].tv_nsec)/1000.0;

		if(tmp_big < time_){
			tmp_big = time_;
		};
	};

	for(unsigned int i = 0; i<para->thread_count; i++){
                double time_  = (t_result.start_time[i].tv_sec)*1000000.0 + \
                       (t_result.start_time[i].tv_nsec)/1000.0;

                if(little > time_){
                        little = time_;
                };
        };


	printf("[result]: true total used time is %lf us\n",tmp_big-little);
	printf("[result]: total bandwidth is %lf Bytes/s\n",(long)(para->block_size*para->block_count)/((tmp_big-little)/1000000.0));

};

int main(int argc, char* argv[])
{
	printf("test start ! \n");

	test_para  para;
	
        para.media_type =atoi(argv[1]);
	para.thread_count = atoi(argv[2]);
	para.block_count = atoi(argv[3]);
	para.block_size = atoi(argv[4]);
	para.operation_mode = RANDOM_READ;

	if(para.media_type == 1){
		printf("[begin]: media type is aep\n");
	}else{
		printf("[begin]: media type is dram\n");
	};
	printf("[begin]: thread count is %u\n",para.thread_count);
	printf("[begin]: block size is %u\n",para.block_size);
	printf("[begin]: total test size is %u\n",para.block_count*para.block_size);


	run_test(&para);

	return 0;
}
