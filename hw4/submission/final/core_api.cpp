/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <queue>

using namespace std;

class Thread {
public:
    int id;
    Instruction* cur_inst;
    int remaining_memory_latency;
    tcontext* regs;
    bool is_halted;
    int line_to_read; //aligned to 4 byte

    //constructor
    Thread (int thread_id) {

        id = thread_id;
        remaining_memory_latency = 0;
        cur_inst = new Instruction;
        regs = new tcontext;
        is_halted = false;
        line_to_read = 0;

        for(int i=0; i<REGS_COUNT; i++){
           regs->reg[i] = 0;
        }
    };

    //distructor
    ~Thread(){
        delete(regs);
        delete(cur_inst);
    };

    cmd_opcode execute_inst(){
        //read next inst to cur_inst
        SIM_MemInstRead( line_to_read, cur_inst, id);

        //get inst data
        cmd_opcode opcode = cur_inst->opcode;
        int dst_index = cur_inst->dst_index;
        int src1_index = cur_inst->src1_index;
        int src2_index_imm = cur_inst->src2_index_imm;
        bool isSrc2Imm = cur_inst->isSrc2Imm;
        int addr;

        //    CMD_NOP,
        //    CMD_ADD,     // dst <- src1 + src2
        //    CMD_SUB,     // dst <- src1 - src2
        //    CMD_ADDI,    // dst <- src1 + imm
        //    CMD_SUBI,    // dst <- src1 - imm
        //    CMD_LOAD,    // dst <- Mem[src1 + src2]  (src2 may be an immediate)
        //    CMD_STORE,   // Mem[dst + src2] <- src1  (src2 may be an immediate)
        //	  CMD_HALT,
        switch (opcode){
            case (CMD_NOP):
                break;
            case (CMD_ADDI):
                regs->reg[dst_index] = regs->reg[src1_index] + src2_index_imm;
                break;
            case (CMD_ADD):
                regs->reg[dst_index] = regs->reg[src1_index] + regs->reg[src2_index_imm];
                break;
            case (CMD_SUBI):
                regs->reg[dst_index] = regs->reg[src1_index] - src2_index_imm;
                break;
            case (CMD_SUB):
                regs->reg[dst_index] = regs->reg[src1_index] - regs->reg[src2_index_imm];
                break;
            case (CMD_LOAD):
                if (isSrc2Imm){
                    addr = regs->reg[src1_index] + src2_index_imm;
                }
                else {
                    addr = regs->reg[src1_index] + regs->reg[src2_index_imm];
                }

                SIM_MemDataRead(addr,&(regs->reg[dst_index]));
                remaining_memory_latency = SIM_GetLoadLat();
//                printf("load addr: %d, id: %d, load to %d <- %d\n", addr, id, dst_index, regs->reg[dst_index]);
                break;
            case (CMD_STORE):
                if (isSrc2Imm){
                    addr = regs->reg[dst_index] + src2_index_imm;
                }
                else {
                    addr = regs->reg[dst_index] + regs->reg[src2_index_imm];
                }
                SIM_MemDataWrite(addr, regs->reg[src1_index]);
                remaining_memory_latency = SIM_GetStoreLat();
//                printf("store to addr: %d , store val: %d, id: %d\n", addr, regs->reg[src1_index], id);
                break;
            case (CMD_HALT):
                is_halted = true;
                break;
        }

        return opcode;
    }
};

//finish if all threads halted
bool is_finished(Thread** threads){
    int num_threads = SIM_GetThreadsNum();
    for(int i=0; i<num_threads; i++){
        if(!threads[i]->is_halted){
            return false;
        }
    }
    return true;
}

class BlockedMT{
public:
    Thread** threads;
    int num_threads;
    int instruction_count;
    int cycles;
    int context_penalty; //num of cycles of context switch
    queue<int>* round_robin;

    //constructor
    BlockedMT(){

        num_threads = SIM_GetThreadsNum();
        threads = new Thread*[num_threads];
        round_robin = new queue<int>;
        for (int i=0; i< num_threads; i++){
            threads[i] = new Thread(i);
            round_robin->push(i);
        }

        instruction_count = 0;
        cycles = 0;
        context_penalty = SIM_GetSwitchCycles();
    }

    //distructor
    ~BlockedMT(){
        for(int i=0; i<num_threads; i++){
            delete (threads[i]);
        }
        delete[](threads);
        delete(round_robin);
    }

    void handle_penalty(){
        for(int i=0; i<num_threads; i++){
            if(!threads[i]->is_halted) {
                if (threads[i]->remaining_memory_latency >= context_penalty){
                    threads[i]->remaining_memory_latency -= context_penalty;
                }
                else if (threads[i]->remaining_memory_latency > 0){
                    threads[i]->remaining_memory_latency = 0;
                }
            }
        }
    }

    void CORE_BlockedMT() {

        cmd_opcode opcode;
        bool switch_thread;
        int last_id;

        //finish if all threads halted
        while(!is_finished(threads)){

            bool thread_ready = false;
            //check if we have free thread
            for (int i = 0; i < num_threads; i++) {
                if (!threads[i]->is_halted) {
                    if (threads[i]->remaining_memory_latency == 0) {
                        thread_ready = true;
                    }
                }
            }

            if (!thread_ready){
                //idle cycle
                for(int j=0; j<num_threads; j++){
                    if (!threads[j]->is_halted) {
//                        printf("threads[%d]->remaining_memory_latency: %d\n", j, threads[j]->remaining_memory_latency);
                        threads[j]->remaining_memory_latency--;
                    }
                }
//                printf("idle\n");
                cycles++;
            }
            else {

                //handle front command
                int cur_thread_id = round_robin->front();

                //front thread ready to execute
                if (threads[cur_thread_id]->remaining_memory_latency == 0) {
                    
                    if(cur_thread_id != last_id){
//                        printf("switch overhead\n");
                        cycles += context_penalty;
                        handle_penalty();
                    }

                    opcode = threads[cur_thread_id]->execute_inst();
                    last_id = cur_thread_id;
//                    printf("opcode: %d, cur_thread_id: %d\n", opcode, cur_thread_id);
                    instruction_count++;
                    cycles++;
                    threads[cur_thread_id]->line_to_read++;

                    switch_thread = false;

                    //check if we have other free thread
                    for (int i = 0; i < num_threads; i++) {
                        if (!threads[i]->is_halted && (i != cur_thread_id)) {
                            if (threads[i]->remaining_memory_latency != 0){
                                //other threads latency - execute cycle
                                threads[i]->remaining_memory_latency--;
//                                printf("threads[%d]->remaining_memory_latency: %d\n", i, threads[i]->remaining_memory_latency);
                            }
                            //free thread
                            if (threads[i]->remaining_memory_latency == 0){
                                switch_thread = true;
                            }
                        }
                    }

                    //if halted remove from round robin
                    if (opcode == CMD_HALT) {
                        round_robin->pop();
                    }

                    //other thread ready
                    if ((opcode == CMD_LOAD || opcode == CMD_STORE) && switch_thread) {
                        round_robin->pop();
                        round_robin->push(cur_thread_id);
                    }
                }
                else {
                    //fake cycle
                    round_robin->pop();
                    round_robin->push(cur_thread_id);
                }
            }
        }

    }

};

class FinegrainedMT{
public:
    Thread** threads;
    int num_threads;
    int instruction_count;
    int cycles;
    queue<int>* round_robin;

    //constructor
    FinegrainedMT(){

        num_threads = SIM_GetThreadsNum();
        threads = new Thread*[num_threads];
        round_robin = new queue<int>;
        for (int i=0; i< num_threads; i++){
            threads[i] = new Thread(i);
            round_robin->push(i);
        }

        instruction_count = 0;
        cycles = 0;
    }

    //distructor
    ~FinegrainedMT(){
        for(int i=0; i<num_threads; i++){
            delete (threads[i]);
        }
        delete[](threads);
        delete(round_robin);
    }

    void CORE_FinegrainedMT() {

        cmd_opcode opcode;

        //finish if all threads halted
        while (!is_finished(threads)) {

            bool thread_ready = false;
            //check if we have free thread
            for (int i = 0; i < num_threads; i++) {
                if (!threads[i]->is_halted) {
                    if (threads[i]->remaining_memory_latency == 0) {
                        thread_ready = true;
                    }
                }
            }

            if (!thread_ready){
                //idle cycle
                for(int j=0; j<num_threads; j++){
                    threads[j]->remaining_memory_latency--;
                }
//                printf("idle\n");
                cycles++;
            }
            else {
                //handle front command
                int cur_thread_id = round_robin->front();

                //if we are here there is no halted thread
                if (threads[cur_thread_id]->remaining_memory_latency == 0) {
                    opcode = threads[cur_thread_id]->execute_inst();
                    instruction_count++;
                    threads[cur_thread_id]->line_to_read++;
                    if (opcode == CMD_HALT) {
                        round_robin->pop();
                    }

                    //check if we have free thread
                    for (int i = 0; i < num_threads; i++) {
                        if (!threads[i]->is_halted && (i != cur_thread_id)) {
                            if (threads[i]->remaining_memory_latency != 0) {
                                threads[i]->remaining_memory_latency--;
                            }
                        }
                    }

                    if ((opcode != CMD_HALT)) {
                        round_robin->pop();
                        round_robin->push(cur_thread_id);
                    }
                    cycles++;
                }
                else {
                    //fake cycle
                    round_robin->pop();
                    round_robin->push(cur_thread_id);
                }
            }
        }
    }

};

BlockedMT* blockedmt_sim = NULL;
FinegrainedMT* finegrainedmt_sim = NULL;

void CORE_BlockedMT() {
    blockedmt_sim = new BlockedMT();
    blockedmt_sim->CORE_BlockedMT();
}

void CORE_FinegrainedMT() {
    finegrainedmt_sim = new FinegrainedMT();
    finegrainedmt_sim->CORE_FinegrainedMT();
}

double CORE_BlockedMT_CPI(){
    if(blockedmt_sim != NULL){
        double cycles = (double)blockedmt_sim->cycles;
        double ic = (double)blockedmt_sim->instruction_count;
        double cpi = cycles/ic;
        delete(blockedmt_sim);
        return cpi;
    }
    return 0;
}

double CORE_FinegrainedMT_CPI(){
    if(finegrainedmt_sim != NULL){
        double cycles = (double)finegrainedmt_sim->cycles;
        double ic = (double)finegrainedmt_sim->instruction_count;
        double cpi = cycles/ic;
        delete(finegrainedmt_sim);
        return cpi;
    }
    return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
    if(blockedmt_sim != NULL){
        for(int i=0; i<REGS_COUNT; i++){
            context[threadid].reg[i] = blockedmt_sim->threads[threadid]->regs->reg[i];
        }
    }
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
    if(finegrainedmt_sim != NULL){
        for(int i=0; i<REGS_COUNT; i++){
            context[threadid].reg[i] = finegrainedmt_sim->threads[threadid]->regs->reg[i];
        }
    }
}

