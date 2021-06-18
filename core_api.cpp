/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>
#include <stdio.h>


class thread {
	public:
	int thread_id;
	int prev_run_CC;
	int wait_CC_remaining;
	bool halted_flag;
	int next_line_to_read=0;
	thread():thread_id(-1), prev_run_CC(-1), wait_CC_remaining(-1), halted_flag(false){} //empty contructor

	thread(int thread_id,	int prev_run_CC,	int wait_CC_remaining, bool halted_flag):
									thread_id(thread_id), prev_run_CC(prev_run_CC), wait_CC_remaining(wait_CC_remaining), halted_flag(halted_flag){}
};

int get_next_thread(std::vector<thread> * thread_vec){
	int i=0, min_thread_id =0, thread_available_flag=0;
	int current_run_cycle, minimum_run_cycle = (*thread_vec)[0].prev_run_CC ;
	bool thread_active;

	for(i=0; i<thread_vec->size(); i++)
	{	
		thread_active = !(*thread_vec)[i].halted_flag;
		current_run_cycle = (*thread_vec)[i].prev_run_CC;

		if( thread_active && (*thread_vec)[i].wait_CC_remaining==0 && current_run_cycle<= minimum_run_cycle){
			minimum_run_cycle=current_run_cycle;
			thread_available_flag =1;
			min_thread_id =i;
		}
	}

	if (!thread_available_flag) //no flags are available to run in this cycle
		return -1;

	return min_thread_id; //return the thread that has been waiting the longest time to run and is available. 
}

void CORE_BlockedMT() {
	/////////////////////////////////////
	///initializing variables
	/////////////////////////////////////
	int i=0, store_time, load_time , current_thread=0, CC=0, num_of_halted_threads=0, thread_num/*, current_line*/;
	thread_num = SIM_GetThreadsNum();
	load_time =SIM_GetLoadLat();
	store_time = SIM_GetStoreLat();
	std::vector<thread> thread_vec;
	Instruction current_instruction;
	
	/////////////////////////////////////
	///initialize instruction vector
	/////////////////////////////////////
	thread_vec.resize(thread_num);

	for(i=0; i<thread_vec.size(); i++)
	{
		thread_vec[i].thread_id=i;
	}
	/////////////////////////////////////
	///main run loop
	/////////////////////////////////////
	while(num_of_halted_threads<thread_num){

		if(thread_vec[current_thread].halted_flag || thread_vec[current_thread].wait_CC_remaining>0){//check if current thread can run now
			current_thread=get_next_thread(&thread_vec);
			if(current_thread<0){
				CC++; //TODO: add treatment for idle cycle(?)
				continue;
			}
		}

		SIM_MemInstRead(thread_vec[current_thread].next_line_to_read,&current_instruction,current_thread);
		CC++;
	}
}

void CORE_FinegrainedMT() {
}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
