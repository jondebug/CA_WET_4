/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>
#include <stdio>


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

int execute_instruction (Instruction inst, std::vector<thread> * thread_vec){

}

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
void update_wait_time(std::vector<thread> * thread_vec, int time){
	int i=0;
	for(i=0; i<thread_vec->size(); i++)
	{
		(*thread_vec)[i].wait_CC_remaining-=time;

		if((*thread_vec)[i].wait_CC_remaining<0){
			(*thread_vec)[i].wait_CC_remaining=0;
		}
	}
}


tcontext *blocked;

void CORE_BlockedMT() {
	/////////////////////////////////////
	///initializing variables
	/////////////////////////////////////
	int i=0, store_time, load_time , current_thread=0, CC=0, num_of_halted_threads=0, thread_num/*, current_line*/, switch_overhead;
	switch_overhead = SIM_GetSwitchCycles();
	thread_num = SIM_GetThreadsNum();
	load_time =SIM_GetLoadLat();
	store_time = SIM_GetStoreLat();
	std::vector<thread> thread_vec;
	Instruction current_instruction;
	blocked =(tcontext*)malloc(thread_num * sizeof(tcontext));

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

			int temp = get_next_thread(&thread_vec);
			
			while(temp<0){ //idle time
				CC++;
				update_wait_time(&thread_vec, 1);
				temp = get_next_thread(&thread_vec);
			}

			current_thread = temp;
			CC+=switch_overhead;
			update_wait_time(&thread_vec, switch_overhead);
		}

		SIM_MemInstRead(thread_vec[current_thread].next_line_to_read,&current_instruction,current_thread);
		thread_vec[current_thread].prev_run_CC=CC;
		thread_vec[current_thread].next_line_to_read++;
		

		CC++;
		update_wait_time(&thread_vec, 1);
	}
}

void CORE_FinegrainedMT(){

	//////// init /////////

	int i=0, store_time, load_time , current_thread=0, CC=0, num_of_halted_threads=0, thread_num;
	store_time = SIM_GetStoreLat();
	load_time =SIM_GetLoadLat();
	thread_num = SIM_GetThreadsNum();
	std::vector<thread> thread_vec;
	thread_vec.resize(thread_num)
	for (size_t i = 0; i < thread_vec.size(); i++)
	{
		thread_vec[i].thread_id = i;
	}
	

	Instruction current_instruction;
	fine_grained =(tcontext*)malloc(thread_num * sizeof(tcontext));

	//TODO: should we init a vector of empty threads or fill the each thread and then use fine_grained.push_back() ?

	//////// main loop /////////

	while(num_of_halted_threads < thread_num){ // means we can still execute
		int next_thread = get_next_thread(&thread_vec);
		
		while(next_thread < 0){ // idle time
			CC++;
			update_wait_time(thread_vec);
			next_thread = get_next_thread(&thread_vec);
		}

		current_thread = next_thread;
		CC++;
		// no switch overhead in fineGrained
		/* here need to handle the command and halt the thread if finished,
		or move to the next command */
	}

	// TODO: here implement what to do once all the threads have finished


}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	for (int i=0; i<7; i++){
		context[threadid].reg[i] = blocked[threadid].reg[i];
	}
}


void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
