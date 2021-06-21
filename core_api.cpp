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

int execute_instruction (Instruction inst, std::vector<thread> * thread_vec, tcontext *context, int thread_id){
	switch(inst.opcode){
		case CMD_ADD:
		  context[thread_id].reg[inst.dst_index]=context[thread_id].reg[inst.src1_index]+context[thread_id].reg[inst.src2_index_imm];	  
		  break;

		case CMD_SUB:
		  context[thread_id].reg[inst.dst_index]=context[thread_id].reg[inst.src1_index]-context[thread_id].reg[inst.src2_index_imm];	  
		  break;

		case CMD_ADDI:
		  context[thread_id].reg[inst.dst_index]=context[thread_id].reg[inst.src1_index]+inst.src2_index_imm;	  
		  break;

		case CMD_SUBI:
		  context[thread_id].reg[inst.dst_index]=context[thread_id].reg[inst.src1_index]-inst.src2_index_imm;	  		
		  break;

		case CMD_LOAD:
		  int address=context[thread_id].reg[inst.src1_index];
		  if(inst.isSrc2Imm){
			  address += inst.src2_index_imm;
		  }
		  else{
			  address += context[thread_id].reg[inst.src2_index_imm];
		  }
		  SIM_MemDataRead(address,&(context[thread_id].reg[inst.dst_index]));
		  (*thread_vec)[thread_id].wait_CC_remaining=SIM_GetLoadLat();
		  
		  break;

		case CMD_STORE:
	//	#	STORE $dst, $src1, $src2 	(Mem[dst + src2] <- src1  src2 may be an immediate)
		  int address=context[thread_id].reg[inst.dst_index];
		  if(inst.isSrc2Imm){
			  address += inst.src2_index_imm;
		  }
		  else{
			  address += context[thread_id].reg[inst.src2_index_imm];
		  }
  		  SIM_MemDataWrite(address,context[thread_id].reg[inst.src1_index]);
		  (*thread_vec)[thread_id].wait_CC_remaining=SIM_GetStoreLat();

		  break;
		case CMD_HALT:
		  (*thread_vec)[thread_id].halted_flag=1;

		  break;

	}


}

int get_next_thread(std::vector<thread> * thread_vec, int current_CC){
	int i=0, min_thread_id =0, thread_available_flag=0;
	int last_run_cycle, minimum_last_CC = current_CC+1 ;
	bool thread_active;

	for(i=0; i<thread_vec->size(); i++)
	{	
		thread_active = !(*thread_vec)[i].halted_flag && (*thread_vec)[i].wait_CC_remaining==0;
		last_run_cycle = (*thread_vec)[i].prev_run_CC;

		if( thread_active && last_run_cycle < minimum_last_CC){
			minimum_last_CC=last_run_cycle;
			thread_available_flag =1;
			min_thread_id = i;
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
	thread_vec[0].halted_flag=1; 																				// thread 0 is initiated with halt
	num_of_halted_threads++;
	CC++;

	/////////////////////////////////////
	///main run loop
	/////////////////////////////////////
	//current_thread=1;
	while(num_of_halted_threads<thread_num){
		
		if(thread_vec[current_thread].halted_flag || thread_vec[current_thread].wait_CC_remaining>0){		  	//check if current thread can run now

			int temp = get_next_thread(&thread_vec, CC);
			
			while(temp<0){ //idle time
				CC++;
				update_wait_time(&thread_vec, 1);
				if(thread_vec[current_thread].halted_flag || thread_vec[current_thread].wait_CC_remaining>0){ 	//check if current thread is ready now
					temp = get_next_thread(&thread_vec, CC);
				}
				else{																						  	//current thread is ready, we can continue with it
					temp=current_thread;
				}
			}

			current_thread = temp;
			CC+=switch_overhead;
			update_wait_time(&thread_vec, switch_overhead);
		}

		SIM_MemInstRead(thread_vec[current_thread].next_line_to_read,&current_instruction,current_thread);
		thread_vec[current_thread].prev_run_CC=CC;
		thread_vec[current_thread].next_line_to_read++;
		//
		CC++;
		update_wait_time(&thread_vec, 1);

		execute_instruction(current_instruction, &thread_vec, blocked, CC);
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
	for (int i=0; i<7; i++){
		context[threadid].reg[i] = blocked[threadid].reg[i];
	}
}


void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
