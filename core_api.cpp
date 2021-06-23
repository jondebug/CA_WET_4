/* 046267 Computer Architecture - Spring 2021 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>
#include <stdio.h>
#include <iostream>

using namespace std;

class thread {
	public:
	int thread_id;
	int prev_run_CC;
	int wait_CC_remaining;
	bool halted_flag;
	int next_line_to_read=0;
	thread():thread_id(-1), prev_run_CC(-1), wait_CC_remaining(0), halted_flag(false){} //empty contructor

	thread(int thread_id,	int prev_run_CC,	int wait_CC_remaining, bool halted_flag):
									thread_id(thread_id), prev_run_CC(prev_run_CC), wait_CC_remaining(wait_CC_remaining), halted_flag(halted_flag){}
};

void execute_instruction (Instruction inst, std::vector<thread> * thread_vec, tcontext *context, int thread_id){
	////cout<<" executing instruction: " << inst.opcode <<" for thread " << thread_id << endl;
	int address =0;
	int loaded_value =25;
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
		  address =context[thread_id].reg[inst.src1_index];
		  //cout << " executing load instruction for thread: " << thread_id ;
		  //cout << " address: "<<address<<endl;
		  if(inst.isSrc2Imm){
			  address += inst.src2_index_imm;
	  		  //cout << " immidiate addition: "<<address<<endl;

		  }
		  else{
			  address += context[thread_id].reg[inst.src2_index_imm];
  	  		  //cout << " reg addition: "<<address<<endl;

		  }
		  
		  // cout << " executing load instruction for thread: " << thread_id << " address: "<<address<<endl;
		  //cout << "register: " << inst.dst_index << " holds value before load: " << context[thread_id].reg[inst.dst_index] << endl;
		  SIM_MemDataRead(address,&(context[thread_id].reg[inst.dst_index]));
		  (*thread_vec)[thread_id].wait_CC_remaining=SIM_GetLoadLat();
		  //cout << " executing load instruction for thread: " << thread_id << " address: "<<address << "now register: " << inst.dst_index << " holds value after load: " << context[thread_id].reg[inst.dst_index] << endl;
		  SIM_MemDataRead(address,&loaded_value);
		  //cout << "--------------loaded_value: " <<loaded_value<<endl;
		  break;

		case CMD_STORE:
	//	#	STORE $dst, $src1, $src2 	(Mem[dst + src2] <- src1  src2 may be an immediate)
		  address=context[thread_id].reg[inst.dst_index];
		  if(inst.isSrc2Imm){
			  address += inst.src2_index_imm;
		  }
		  else{
			  address += context[thread_id].reg[inst.src2_index_imm];
		  }
		  //	cout<< " ----- executing store instruction for thread: " << thread_id << " in address: " <<address << " writing value: "<< context[thread_id].reg[inst.src1_index] << endl;
  		  SIM_MemDataWrite(address,context[thread_id].reg[inst.src1_index]);
		  (*thread_vec)[thread_id].wait_CC_remaining=SIM_GetStoreLat();

		  break;
		case CMD_HALT:
		  (*thread_vec)[thread_id].halted_flag=1;
		
		  break;

		  ////cout<<" done executing instruction" << endl;

	}


}
int get_next_thread_FG(std::vector<thread> * thread_vec, int current_thread){
	int thread_num = (*thread_vec).size(), thread_it=0;
	bool thread_active;
	//int next_thread=-1;
	for(int i=1; i<= thread_num; i++){
		thread_it=(current_thread+i)%thread_num;
		thread_active = !(*thread_vec)[thread_it].halted_flag && (*thread_vec)[thread_it].wait_CC_remaining==0;
		if(thread_active){
			return thread_it;
		}
	}
	return -1;

}

int get_next_thread_B(std::vector<thread> * thread_vec, int current_CC){
	int i=0, min_thread_id =0, thread_available_flag=0;
	int last_run_cycle, minimum_last_CC = current_CC+1 ;
	bool thread_active;
	

	for(i=0; i<thread_vec->size(); i++)
	{
		thread_active = !(*thread_vec)[i].halted_flag && (*thread_vec)[i].wait_CC_remaining==0;
		last_run_cycle = (*thread_vec)[i].prev_run_CC;
		////cout << " thread: " << i << " last ran in CC: " << last_run_cycle <<" halted: "<< (*thread_vec)[i].halted_flag <<" remaining wait CC: "<< (*thread_vec)[i].wait_CC_remaining << endl;

		if( thread_active && last_run_cycle < minimum_last_CC){
			minimum_last_CC=last_run_cycle;
			thread_available_flag =1;
			min_thread_id = i;
		}
	}
	////cout << "current_CC is:"<< current_CC<<" -----next thread is: ----- " << ((thread_available_flag)?min_thread_id:-1) << endl;

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
tcontext *fine_grained;
double block_CPI = 0;
double fineGrained_CPI = 0;
void init_regs(tcontext *refs, int thread_num){

}

void CORE_BlockedMT() {
	/////////////////////////////////////
	///initializing variables
	/////////////////////////////////////
	int i=0, current_thread=0, CC=0, num_of_halted_threads=0, thread_num, switch_overhead, num_of_instructions = 0;
	switch_overhead = SIM_GetSwitchCycles();
	thread_num = SIM_GetThreadsNum();
	std::vector<thread> thread_vec;
	Instruction current_instruction;
	blocked =(tcontext*)calloc(thread_num, sizeof(tcontext));

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

	while(num_of_halted_threads < thread_num){
		
		if(thread_vec[current_thread].halted_flag || thread_vec[current_thread].wait_CC_remaining>0){		  	//check if current thread can run now

			int temp = get_next_thread_B(&thread_vec, CC);
			
			while(temp<0){ //idle time
				CC++;
				update_wait_time(&thread_vec, 1);
				if(thread_vec[current_thread].halted_flag || thread_vec[current_thread].wait_CC_remaining>0){ 	//check if current thread is ready now
					temp = get_next_thread_B(&thread_vec, CC);
				}
				else{																						  	//current thread is ready, we can continue with it
					temp=current_thread;
				}
			}
			if (current_thread != temp){
				current_thread = temp;
				CC+=switch_overhead;
				update_wait_time(&thread_vec, switch_overhead);
			}
		}

		SIM_MemInstRead(thread_vec[current_thread].next_line_to_read,&current_instruction,current_thread);
		thread_vec[current_thread].prev_run_CC=CC;
		////cout<< "thread: " << current_thread << " is running in cycle " << CC << endl;
		thread_vec[current_thread].next_line_to_read++;
		//
		CC++;
		update_wait_time(&thread_vec, 1);
		num_of_instructions++;
		execute_instruction(current_instruction, &thread_vec, blocked, current_thread);
		if (current_instruction.opcode == CMD_HALT){
			num_of_halted_threads++;
		}
	}
	////cout <<"clock cycles "<< CC <<" num of instructions: "<<num_of_instructions<< endl;
	block_CPI = CC / (double)num_of_instructions;
}

void CORE_FinegrainedMT(){
	//printf("\n-----starting Finegrained MT Simulation -----\n");
	//////// init /////////

	int i=0, current_thread=-1, CC=0, num_of_halted_threads=0, thread_num, num_of_instructions = 0;
	thread_num = SIM_GetThreadsNum();
	std::vector<thread> thread_vec;
	Instruction current_instruction;
	fine_grained =(tcontext*)calloc(thread_num , sizeof(tcontext));

	/////////////////////////////////////
	///initialize instruction vector
	/////////////////////////////////////

	thread_vec.resize(thread_num);

	for(i=0; i<thread_vec.size(); i++)
	{
		thread_vec[i].thread_id=i;
	}

	//////// main loop /////////
	int next_thread=-1;
	while(num_of_halted_threads < thread_num){ // means we can still execute
		//cout<< "prev_thread is: " <<next_thread;
		next_thread = get_next_thread_FG(&thread_vec, current_thread);
		
		while(next_thread < 0){ // idle time
			CC++;
			update_wait_time(&thread_vec, 1);
			next_thread = get_next_thread_FG(&thread_vec, next_thread);
		}
		current_thread = next_thread;
		


		// no switch overhead in fineGrained
		
		SIM_MemInstRead(thread_vec[current_thread].next_line_to_read,&current_instruction,current_thread);
		thread_vec[current_thread].prev_run_CC=CC;
		thread_vec[current_thread].next_line_to_read++;
		////cout << "thread to run is: " << current_thread << " cycle " << CC << " instruction: " << current_instruction.opcode <<  endl;
		CC++;
		update_wait_time(&thread_vec, 1);
		num_of_instructions++;
		//cout<< " current_thread is: " <<next_thread << " opcode: " << current_instruction.opcode << endl;;
		execute_instruction(current_instruction, &thread_vec, fine_grained, current_thread);
		if (current_instruction.opcode == CMD_HALT){
			num_of_halted_threads++;
		}
	}
	//cout <<"clock cycles "<< CC <<" num of instructions: "<<num_of_instructions<< endl;

	fineGrained_CPI = CC / (double)num_of_instructions;

}

double CORE_BlockedMT_CPI(){
	return block_CPI;
}

double CORE_FinegrainedMT_CPI(){
	return fineGrained_CPI;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	for (int i=0; i<REGS_COUNT; i++){
		context[threadid].reg[i] = blocked[threadid].reg[i];
	}
}


void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	for (int i=0; i<REGS_COUNT; i++){
		context[threadid].reg[i] = fine_grained[threadid].reg[i];
	}
}
