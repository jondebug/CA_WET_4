/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>



typedef struct _thread {
    Instruction curr_inst;
    tcontext regs_values;
    int num_of_clocks_to_finish; //for load and store commands
    bool is_halted;
    int command_num;
    int thread_id;
} thread_info;

typedef struct _all_threads{
    thread_info * all_threads;
    int cycle_count;
} all_threads;

//globals:
all_threads all_thrds_blk;
all_threads* p_all_threads_Blocked = &all_thrds_blk;

all_threads all_thrds_fngr;
all_threads* p_all_threads_fine_grained = &all_thrds_fngr;
//

void init_thread_info(thread_info * p_thread_info, int thread_id)
{

    int i;
    for(i=0;i<REGS_COUNT;i++) {
        p_thread_info->regs_values.reg[i] = 0;
    }
    p_thread_info->num_of_clocks_to_finish = 0;
    p_thread_info->is_halted = false;
    p_thread_info->command_num = 0;
    p_thread_info->thread_id = thread_id;

}



void init_all_threads(all_threads * p_all_threads)
{
    int num = SIM_GetThreadsNum();

    p_all_threads->all_threads = (thread_info *)malloc(num*sizeof(thread_info));

    int i;
    int thread_num = SIM_GetThreadsNum();
    for(i=0;i<thread_num;i++) {
        init_thread_info(&(p_all_threads->all_threads[i]), i);
    }

    p_all_threads->cycle_count = 0;

}

void execute_command(thread_info * p_thread_info)
{
    cmd_opcode opc = p_thread_info->curr_inst.opcode;
    int dst_index = p_thread_info->curr_inst.dst_index;
    int src1_index = p_thread_info->curr_inst.src1_index;
    int src2_index_imm = p_thread_info->curr_inst.src2_index_imm;
    bool is_immediate = p_thread_info->curr_inst.isSrc2Imm;

    int load_let = SIM_GetLoadLat();
    int save_let = SIM_GetStoreLat();

    switch (opc) {
        case CMD_NOP: // NOP
            break;

        case CMD_ADDI:
            p_thread_info->regs_values.reg[dst_index] = p_thread_info->regs_values.reg[src1_index] + src2_index_imm;
            break;
        case CMD_SUBI:
            p_thread_info->regs_values.reg[dst_index] = p_thread_info->regs_values.reg[src1_index] - src2_index_imm;
            break;
        case CMD_ADD:
            p_thread_info->regs_values.reg[dst_index] = p_thread_info->regs_values.reg[src1_index] + p_thread_info->regs_values.reg[src2_index_imm];
            break;
        case CMD_SUB:
            p_thread_info->regs_values.reg[dst_index] = p_thread_info->regs_values.reg[src1_index] - p_thread_info->regs_values.reg[src2_index_imm];
            break;
        case CMD_LOAD:
            if(is_immediate)
            {
                int addr = p_thread_info->regs_values.reg[src1_index] + src2_index_imm;
                SIM_MemDataRead(addr, &(p_thread_info->regs_values.reg[dst_index]));
            } else
            {
                int addr = p_thread_info->regs_values.reg[src1_index] + p_thread_info->regs_values.reg[src2_index_imm];
                SIM_MemDataRead(addr, &(p_thread_info->regs_values.reg[dst_index]));
            }
            p_thread_info->num_of_clocks_to_finish = load_let;
            break;
        case CMD_STORE:
            if(is_immediate)
            {
                //Mem[dst + src2] <- src1  (src2 may be an immediate)
                int addr = p_thread_info->regs_values.reg[dst_index]+src2_index_imm;
                SIM_MemDataWrite(addr, p_thread_info->regs_values.reg[src1_index]);
            } else
            {
                int addr = p_thread_info->regs_values.reg[dst_index] + p_thread_info->regs_values.reg[src2_index_imm];
                SIM_MemDataWrite(addr, p_thread_info->regs_values.reg[src1_index]);
            }
            p_thread_info->num_of_clocks_to_finish = save_let;
            break;
        case CMD_HALT:
            p_thread_info->is_halted = true;
            break;
    }
}

void handle_clock_cycle(thread_info * p_thread_info,bool in_front, cmd_opcode* cmd_opc)
{
    if(!p_thread_info->is_halted) {
        if (p_thread_info->num_of_clocks_to_finish == 0) {
            if(in_front) {
                SIM_MemInstRead(p_thread_info->command_num, &(p_thread_info->curr_inst), p_thread_info->thread_id);
                execute_command(p_thread_info);
                *cmd_opc = p_thread_info->curr_inst.opcode;
                p_thread_info->command_num++;
            }
        } else {
            if(in_front)
            {
                //this situation means that this cycle cmd is nop
                if(p_thread_info->num_of_clocks_to_finish == 1)
                {
                    *cmd_opc = CMD_NOP;
                }
            }
            p_thread_info->num_of_clocks_to_finish--;

        }
    }
}


bool all_threads_halted(all_threads* p_all_threads)
{

    int i;
    int thread_num = SIM_GetThreadsNum();
    for(i=0; i<thread_num;i++)
    {
        if(!(p_all_threads->all_threads[i].is_halted)) {

            return false;
        }
    }
    return true;
}

void CORE_BlockedMT() {
    init_all_threads(p_all_threads_Blocked);
    int switch_overhead = SIM_GetSwitchCycles();
    int thread_num = SIM_GetThreadsNum();
    int i = 0;
    int id = 0;
    bool is_in_front = true;
    int front_thread = 0;
    cmd_opcode opc = CMD_NOP;
    int remaining_cycles_to_finish_switch = 0;

    while(!all_threads_halted(p_all_threads_Blocked))
    {

        for(i=0;i<thread_num;i++)
        {
            is_in_front = (i == front_thread)&&(!(remaining_cycles_to_finish_switch));
            handle_clock_cycle(&(p_all_threads_Blocked->all_threads[i]), is_in_front,&opc);
            //now we know what was the cmd in front in this cycle
        }

        //check if need to change thread:
        if(opc == CMD_LOAD || opc == CMD_STORE || opc == CMD_HALT)
        {
            //time to change threat in front if there is a free thread

            for(i = 1; i<thread_num;i++)
            {
                id = (front_thread+i)%thread_num;
                thread_info * p_thread_info = &(p_all_threads_Blocked->all_threads[id]);
                if(!p_thread_info->is_halted) {
                    if (p_thread_info->num_of_clocks_to_finish == 0) {
                        front_thread = id;
                        remaining_cycles_to_finish_switch = switch_overhead+1;
                        opc = CMD_NOP;
                        break;
                    }
                }
            }
        }
        if(remaining_cycles_to_finish_switch>0) {
            remaining_cycles_to_finish_switch--;
        }

        p_all_threads_Blocked->cycle_count++;

    }

}

void CORE_FinegrainedMT() {
    init_all_threads(p_all_threads_fine_grained);
    int switch_overhead = 0;
    int thread_num = SIM_GetThreadsNum();
    int i = 0;
    int id = 0;
    bool is_in_front = true;
    int front_thread = 0;
    cmd_opcode opc = CMD_NOP;
    int remaining_cycles_to_finish_switch = 0;
    while(!all_threads_halted(p_all_threads_fine_grained))
    {
        for(i=0;i<thread_num;i++)
        {
            is_in_front = (i == front_thread)&&(!(remaining_cycles_to_finish_switch));
            handle_clock_cycle(&(p_all_threads_fine_grained->all_threads[i]), is_in_front,&opc);
            //now we know what was the cmd in front in this cycle
        }

        //time to change threat in front if there is a free thread

        for(i = 1; i<thread_num;i++)
        {
            id = (front_thread+i)%thread_num;
            thread_info * p_thread_info = &(p_all_threads_fine_grained->all_threads[id]);
            if(!p_thread_info->is_halted) {
                if (p_thread_info->num_of_clocks_to_finish == 0) {
                    front_thread = id;
                    remaining_cycles_to_finish_switch = switch_overhead+1;
                    opc = CMD_NOP;
                    break;
                }
            }
        }

        if(remaining_cycles_to_finish_switch>0) {
            remaining_cycles_to_finish_switch--;
        }

        p_all_threads_fine_grained->cycle_count++;

    }
}


double CORE_BlockedMT_CPI(){
    double num_of_cycles = p_all_threads_Blocked->cycle_count;
    double total_num_of_commands = 0;
    int thread_num = SIM_GetThreadsNum();
    int i;
    for(i=0;i<thread_num;i++)
    {
        total_num_of_commands += p_all_threads_Blocked->all_threads[i].command_num;
    }
    free(p_all_threads_Blocked->all_threads);
    return (num_of_cycles/total_num_of_commands);
}

double CORE_FinegrainedMT_CPI(){
    double num_of_cycles = p_all_threads_fine_grained->cycle_count;
    double total_num_of_commands = 0;
    int thread_num = SIM_GetThreadsNum();
    int i;
    for(i=0;i<thread_num;i++)
    {
        total_num_of_commands += p_all_threads_fine_grained->all_threads[i].command_num;
    }
    free(p_all_threads_fine_grained->all_threads);
    return (num_of_cycles/total_num_of_commands);
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
    int i;
    for(i=0;i<REGS_COUNT;i++)
    {

        context[threadid].reg[i] = p_all_threads_Blocked->all_threads[threadid].regs_values.reg[i];
    }

}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
    int i;
    for(i=0;i<REGS_COUNT;i++)
    {
        context[threadid].reg[i] = p_all_threads_fine_grained->all_threads[threadid].regs_values.reg[i];
    }
}
