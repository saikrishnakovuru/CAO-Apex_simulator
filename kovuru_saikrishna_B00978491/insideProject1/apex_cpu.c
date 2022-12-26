/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */

struct flagCheck check;
// Used these variables to implement teh stall functionality.
int inUse = 1;
int notInUse = 0;
// to check if the register values are empty, I used while stalling.
int isRegisterValueEmpty = 0;

static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_DIV:
    case OPCODE_AND:
    case OPCODE_OR:
    case OPCODE_XOR:
    case OPCODE_LDR:
    {
        // since all the abouve OPCODE functions need the same stage variables rd, rs1,rs2 used them in common.
        printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->rs2);
        break;
    }

    case OPCODE_MOVC:
    {
        printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
        break;
    }

    case OPCODE_LOAD:
    case OPCODE_ADDL:
    case OPCODE_SUBL:
    {
        // above three OPCODE functions need rd, rs1, imm.
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->imm);
        break;
    }

    case OPCODE_STORE:
    {
        // store takes the immegiate value and rs2.
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
               stage->imm);
        break;
    }

    case OPCODE_STR:
    {
        // functionality is same as STORE but has the rs3 value.
        printf("%s,R%d,R%d,R%d", stage->opcode_str, stage->rs1, stage->rs2, stage->rs3);
        break;
    }

    case OPCODE_BZ:
    case OPCODE_BNZ:
    {
        // since BZ and BNZ accepts only immediate value.
        printf("%s,#%d ", stage->opcode_str, stage->imm);
        break;
    }

    case OPCODE_HALT:
    case OPCODE_NOP:
    {
        // we just take the opcode entred and nothing ither than that.
        printf("%s", stage->opcode_str);
        break;
    }

    case OPCODE_CMP:
    {
        // compare takes the two registers and compate the values.
        printf("%s,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2);
        break;
    }
    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
}

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;

    if (cpu->fetch.has_insn)
    {
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }

        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.imm = current_ins->imm;

        if (cpu->decode.is_stalled != notInUse || cpu->decode.is_stalled == inUse)
        {
            // nothing to increment as the stalling is needed set is_stalled true
            // just setting the value 1 as it has to be stalled.
            cpu->fetch.is_stalled = inUse;
        }
        /* Stop fetching new instructions if HALT is fetched */
        else if (cpu->fetch.opcode == OPCODE_HALT)
        {
            if (cpu->decode.is_stalled == notInUse)
            {
                // commented as interrupting o/p
                //  printf("\nFETCH stage HALT\n");
                cpu->decode = cpu->fetch;
                // if the instruction is HALT we should not receve any of the new instructions so setting fetch.has_insn to false.
                cpu->fetch.has_insn = FALSE;
            }
        }
        else if (cpu->decode.is_stalled == notInUse)
        {
            /* Update PC for next instruction */
            // incements the PC and takes the next instruction only when stall is not in use
            cpu->pc += 4;

            /* Copy data from fetch latch to decode latch*/

            cpu->decode = cpu->fetch;
        }

        // printing the stage content
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Instruction at FETCH STAGE --->           ", &cpu->fetch);
        }
    }

    else if (cpu->fetch.opcode != OPCODE_HALT)
    {
        if (cpu->decode.is_stalled == notInUse)
        {
            // if not halt or stalled, pass the fetch data instructions to decode.
            cpu->decode = cpu->fetch;
        }
    }
    else if (ENABLE_DEBUG_MESSAGES)
    {
        // else just display "empty" .
        cpu->fetch.has_insn = FALSE;
        printf("Instruction at FETCH STAGE --->           : EMPTY\n");
    }
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{

    if (cpu->decode.has_insn && cpu->decode.is_stalled == notInUse && cpu->fetch.is_stalled == notInUse)
    {
        int addressOfRegisterOne = 0;
        int addressOfRegisterTwo = 0;
        int addressOfRegisterThree = 0;
        int valueInReg1 = 0;
        int valueInReg2 = 0;
        int valueInReg3 = 0;
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_DIV:
        case OPCODE_MUL:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_LDR:
        {
            // only enter when the register values are empty if not it goes to else
            if (cpu->regCheck[cpu->decode.rs1] == isRegisterValueEmpty && cpu->regCheck[cpu->decode.rs2] == isRegisterValueEmpty)
            {
                addressOfRegisterOne = cpu->decode.rs1;
                valueInReg1 = cpu->regs[addressOfRegisterOne];
                // passing theh value of rs1 in register to rs1_value and same operaion to rs2_value.
                cpu->decode.rs1_value = valueInReg1;

                addressOfRegisterTwo = cpu->decode.rs2;
                valueInReg2 = cpu->regs[addressOfRegisterTwo];

                cpu->decode.rs2_value = valueInReg2;
                // settign the value to the new register

                cpu->regCheck[cpu->decode.rd] = inUse;
                addressOfRegisterOne = 0;
                valueInReg1 = 0;
                addressOfRegisterTwo = 0;
                valueInReg2 = 0;
            }
            else
            {
                // since the register values are not empty some value has been aready set and has to be moved to stall.
                cpu->decode.is_stalled = inUse;
                cpu->fetch.is_stalled = inUse;
            }
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        {
            if (cpu->regCheck[cpu->decode.rs1] == isRegisterValueEmpty)
            {
                addressOfRegisterOne = cpu->decode.rs1;
                valueInReg1 = cpu->regs[addressOfRegisterOne];
                // passing theh value of rs1 in register to rs1_value and same operaion to rs2_value.
                cpu->decode.rs1_value = valueInReg1;

                // cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->regCheck[cpu->decode.rd] = inUse;
                addressOfRegisterOne = 0;
                valueInReg1 = 0;
            }
            else
            {
                // in the else part since the register values are not empty some value has been aready set and has to be moved to stall.
                cpu->decode.is_stalled = inUse;
                cpu->fetch.is_stalled = inUse;
            }
            break;
        }

        case OPCODE_STORE:
        {
            if (cpu->regCheck[cpu->decode.rs1] == isRegisterValueEmpty && cpu->regCheck[cpu->decode.rs2] == isRegisterValueEmpty)
            {
                addressOfRegisterOne = cpu->decode.rs1;
                valueInReg1 = cpu->regs[addressOfRegisterOne];
                // passing theh value of rs1 in register to rs1_value and same operaion to rs2_value.
                cpu->decode.rs1_value = valueInReg1;

                addressOfRegisterTwo = cpu->decode.rs2;
                valueInReg2 = cpu->regs[addressOfRegisterTwo];

                cpu->decode.rs2_value = valueInReg2;

                cpu->decode.rd = inUse;
                addressOfRegisterOne = 0;
                valueInReg1 = 0;
                addressOfRegisterTwo = 0;
                valueInReg2 = 0;
            }
            else
            {
                // in the else part since the register values are not empty some value has been aready set and has to be moved to stall.
                cpu->decode.is_stalled = inUse;
                cpu->fetch.is_stalled = inUse;
            }
            break;
        }

        case OPCODE_STR:
        {
            if (cpu->regCheck[cpu->decode.rs1] == isRegisterValueEmpty && cpu->regCheck[cpu->decode.rs2] == isRegisterValueEmpty && cpu->regCheck[cpu->decode.rs3] == isRegisterValueEmpty)
            {
                addressOfRegisterOne = cpu->decode.rs1;
                valueInReg1 = cpu->regs[addressOfRegisterOne];
                cpu->decode.rs1_value = valueInReg1;

                addressOfRegisterTwo = cpu->decode.rs2;
                valueInReg2 = cpu->regs[addressOfRegisterTwo];
                cpu->decode.rs2_value = valueInReg2;

                addressOfRegisterThree = cpu->decode.rs3;
                valueInReg3 = cpu->regs[addressOfRegisterThree];
                cpu->decode.rs1_value = valueInReg3;

                cpu->decode.rd = inUse;

                addressOfRegisterOne = 0;
                valueInReg1 = 0;
                addressOfRegisterTwo = 0;
                valueInReg2 = 0;
                addressOfRegisterThree = 0;
                valueInReg3 = 0;
            }
            else
            {
                // in the else part since the register values are not empty some value has been aready set and has to be moved to stall.
                cpu->decode.is_stalled = inUse;
                cpu->fetch.is_stalled = inUse;
            }

            break;
        }

        case OPCODE_MOVC:
        {

            cpu->regCheck[cpu->decode.rd] = inUse;
            break;
        }

        case OPCODE_NOP:
        case OPCODE_HALT:
        case OPCODE_BZ:
        case OPCODE_BNZ:
        {
            // since they don't have registers.
            cpu->decode.rd = inUse;
            break;
        }

        case OPCODE_CMP:
        {
            if (cpu->regCheck[cpu->decode.rs1] == isRegisterValueEmpty && cpu->regCheck[cpu->decode.rs2] == isRegisterValueEmpty)
            {
                addressOfRegisterOne = cpu->decode.rs1;
                valueInReg1 = cpu->regs[addressOfRegisterOne];
                // passing theh value of rs1 in register to rs1_value and same operaion to rs2_value.
                cpu->decode.rs1_value = valueInReg1;

                addressOfRegisterTwo = cpu->decode.rs2;
                valueInReg2 = cpu->regs[addressOfRegisterTwo];
                cpu->decode.rs2_value = valueInReg2;

                addressOfRegisterOne = 0;
                valueInReg1 = 0;
                addressOfRegisterTwo = 0;
                valueInReg2 = 0;
                cpu->decode.rd = inUse;
            }
            else
            {
                // in the else part since the register values are not empty some value has been aready set and has to be moved to stall.
                cpu->decode.is_stalled = inUse;
                cpu->fetch.is_stalled = inUse;
            }
            break;
        }
        }
        if (cpu->execute.is_stalled == notInUse && cpu->decode.is_stalled == notInUse)
        {
            /* Copy data from decode latch to execute latch*/
            cpu->execute = cpu->decode;
            // exeEmpty = 0;

            print_stage_content("Instruction at DECODE_RF_STAGE --->          ", &cpu->decode);
        }
        else
        {
            // if none of the operations are performed EMPTY as to be printed
            printf("Instruction at DECODE_RF_STAGE --->      : EMPTY\n");
        }
    }
    else
    {
        // I'm not checking "ENABLE_DEBUG_MESSAGES" because it is TRUE by default and if passes every time.
        printf("Instruction at DECODE_RF_STAGE --->      : EMPTY\n");
    }
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{

    if (cpu->execute.has_insn && cpu->execute.is_stalled == notInUse)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
        case OPCODE_ADD:
        {
            // had the printf operations to display which values are being added but it's interrupting the o/p.
            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.rs2_value;

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            // setZeroFlagBasedOnResultBuffer(cpu);
            break;
        }

        case OPCODE_ADDL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            // setZeroFlagBasedOnResultBuffer(cpu);
            break;
        }

        case OPCODE_SUBL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.imm;
            // printf("\n Executing SUBL %d %d \n", cpu->execute.rs1_value, cpu->execute.imm);

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            // setZeroFlagBasedOnResultBuffer(cpu);
            break;
        }

        case OPCODE_SUB:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value - cpu->execute.rs2_value;
            // printf("\n Executing Substraction %d %d \n", cpu->execute.rs1_value, cpu->execute.rs2_value);

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            // setZeroFlagBasedOnResultBuffer(cpu);
            break;
        }
        case OPCODE_DIV:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value / cpu->execute.rs2_value;
            // printf("\n Executing Division %d %d \n", cpu->execute.rs1_value, cpu->execute.rs2_value);

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            // setZeroFlagBasedOnResultBuffer(cpu);
            break;
        }
        case OPCODE_MUL:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value * cpu->execute.rs2_value;
            // printf("\n Executing Multiplication %d %d \n", cpu->execute.rs1_value, cpu->execute.rs2_value);

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            // setZeroFlagBasedOnResultBuffer(cpu);
            break;
        }
        case OPCODE_AND:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value & cpu->execute.rs2_value;
            // printf("\n Executing AND %d %d \n", cpu->execute.rs1_value, cpu->execute.rs2_value);

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            // setZeroFlagBasedOnResultBuffer(cpu);
            break;
        }
        case OPCODE_OR:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value | cpu->execute.rs2_value;
            // printf("\n Executing OR %d %d \n", cpu->execute.rs1_value, cpu->execute.rs2_value);

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            // setZeroFlagBasedOnResultBuffer(cpu);
            break;
        }
        case OPCODE_XOR:
        {
            cpu->execute.result_buffer = cpu->execute.rs1_value ^ cpu->execute.rs2_value;
            // printf("\n Executing XOR %d %d \n", cpu->execute.rs1_value, cpu->execute.rs2_value);

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            // setZeroFlagBasedOnResultBuffer(cpu);
            break;
        }

        case OPCODE_LOAD:
        {
            // cpu->execute.memory_address = cpu->execute.memory_address + 9;
            // printf(" \n address before load operation %d \n ", cpu->execute.memory_address);
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
            // printf(" \n address after load operaion %d \n ", cpu->execute.memory_address);
            // initially I set a values "MOVC R9,#10" and "MOVC R3,#20"
            //'MOVC R9,#10' later few steps I have run the command 'LOAD R9,R3,#4'
            // now the value R3+4=24 --> 24th address values has to be set to "R9" sine we don't know the value at address 24
            // the R9 value has been reset to "0".
            break;
        }

        case OPCODE_LDR:
        {
            // load memory operations are same as LOAD operation, but will have the register value instead of literal value.
            cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.rs2_value;
            // printf("\n execute stage LDR MemoryAddress: %d rs1_value: %d rs2_value :%d\n", cpu->execute.memory_address, cpu->execute.rs1_value, cpu->execute.rs2_value);
            break;
        }

        case OPCODE_STORE:
        {
            // though there is nothign to execute rs1 value since we have to set that value into the memory address of rs2+imm
            // we set send it to the result buffer.
            cpu->execute.result_buffer = cpu->execute.rs1_value;
            cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.imm;
            break;
        }

        case OPCODE_STR:
        {
            // though there is nothign to execute rs1 value since we have to set that value into the memory address of rs2+rs3
            // we set send it to the result buffer.

            cpu->execute.result_buffer = cpu->execute.rs1_value;
            cpu->execute.memory_address = cpu->execute.rs2_value + cpu->execute.rs3_value;

            break;
        }

        case OPCODE_BZ:
        {
            if (cpu->zero_flag == TRUE)
            {
                /* Calculate new PC, and send it to fetch unit */
                cpu->pc = cpu->execute.pc + cpu->execute.imm;

                /* Since we are using reverse callbacks for pipeline stages,
                 * this will prevent the new instruction from being fetched in the current cycle*/
                cpu->fetch_from_next_cycle = TRUE;

                /* Flush previous stages */
                cpu->decode.has_insn = FALSE;

                /* Make sure fetch stage is enabled to start fetching from new PC */
                cpu->fetch.has_insn = TRUE;
            }
            break;
        }

        case OPCODE_BNZ:
        {
            if (cpu->zero_flag == FALSE)
            {
                /* Calculate new PC, and send it to fetch unit */
                cpu->pc = cpu->execute.pc + cpu->execute.imm;

                /* Since we are using reverse callbacks for pipeline stages,
                 * this will prevent the new instruction from being fetched in the current cycle*/
                cpu->fetch_from_next_cycle = TRUE;

                /* Flush previous stages */
                cpu->decode.has_insn = FALSE;

                /* Make sure fetch stage is enabled to start fetching from new PC */
                cpu->fetch.has_insn = TRUE;
            }
            break;
        }

        case OPCODE_MOVC:
        {
            // right away passses the vlaues ot teh reulst Buffer and sets to the register value
            cpu->execute.result_buffer = cpu->execute.imm;

            /* Set the zero flag based on the result buffer */
            if (cpu->execute.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            break;
        }

        case OPCODE_CMP:
        {
            // cmp just compares the register values.
            if (cpu->execute.rs1_value == cpu->execute.rs2_value)
            {
                // printf("\nRegister values are equal\n");
                cpu->zero_flag = TRUE;
            }
            else
            {
                // printf("\nRegister values are not equal\n");
                cpu->zero_flag = FALSE;
            }
            break;
        }
        case OPCODE_NOP:
        case OPCODE_HALT:
        {
            // no Execute operations for NOP and HALT.
            break;
        }
        }

        /* Copy data from execute latch to memory latch*/
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Instruction at EX STAGE --->                 ", &cpu->execute);
        }
    }
    else
    {
        // I'm not checking "ENABLE_DEBUG_MESSAGES" because it is TRUE bu default and if passes every time.
        printf("Instruction at EX STAGE --->            : EMPTY\n");
    }
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{

    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_DIV:
        case OPCODE_MUL:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_MOVC:
        case OPCODE_NOP:
        case OPCODE_HALT:
        case OPCODE_CMP:
        {
            // no operations here.
            break;
        }

        case OPCODE_LOAD:
        case OPCODE_LDR:
        {
            /* Read from data memory */
            cpu->memory.result_buffer = cpu->data_memory[cpu->memory.memory_address];
            break;
        }

        case OPCODE_STORE:
        {
            // storing hte memory address of rs1, as we have to place the added values address of rs2 and literal in rs1's memory address.

            // cpu->memory.result_buffer = cpu->data_memory[cpu->memory.rs1_value];
            cpu->data_memory[cpu->memory.memory_address] = cpu->memory.result_buffer;
            // Used the above commented line so memory values were not displaying while run time so changed the logic.

            // printf("\n MemoryAddress to store rs1_value in STORE operation is: %d\n", cpu->data_memory[cpu->memory.memory_address]);
            break;
        }

        case OPCODE_STR:
        {
            // thought of rightaway setting the value of rs2 at data_memory but memory values were not displaying while run time so changed the logic
            // cpu->memory.result_buffer = cpu->data_memory[cpu->memory.rs1_value];
            cpu->data_memory[cpu->memory.memory_address] = cpu->memory.result_buffer;
            break;
        }
        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Instruction at MEMORY STAGE --->            ", &cpu->memory);
        }
    }
    else
    {
        // I'm not checking "ENABLE_DEBUG_MESSAGES" because it is TRUE bu default and if passes every time.
        printf("Instruction at MEMORY STAGE --->         : EMPTY\n");
    }
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{

    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_DIV:
        case OPCODE_MUL:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        case OPCODE_MOVC:
        case OPCODE_LOAD:
        case OPCODE_LDR:
        {
            // settingt hte result buffer to write back stage.
            cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
            // settig all the registers are not in use and make them not stalled.
            cpu->regCheck[cpu->writeback.rd] = notInUse;
            cpu->fetch.is_stalled = notInUse;
            cpu->decode.is_stalled = notInUse;
            break;
        }

        case OPCODE_STORE:
        case OPCODE_STR:
        {

            cpu->data_memory[cpu->writeback.memory_address] = cpu->writeback.result_buffer;
            break;
        }

        case OPCODE_NOP:
        case OPCODE_HALT:
        case OPCODE_CMP:
        case OPCODE_BZ:
        case OPCODE_BNZ:
        {
            // no implementation for CMP in memory stage as we are just comparing the both the register values.
            // no operations for NOP and HALT.
            // as mentioned in the documentation Bz abd BNZ has been implemented I just left empty
            break;
        }
        }

        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Instruction at WRITEBACK_STAGE --->       ", &cpu->writeback);
        }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }
    }
    else
    {
        // I'm not checking "ENABLE_DEBUG_MESSAGES" because it is TRUE bu default and if passes every time.
        printf("Instruction at WRITEBACK_STAGE --->      : EMPTY\n");
    }

    /* Default */
    return 0;
}
// implicit declaration of function 'print_state_of_architectural_register_file' is invalid in C99 while declaring at last
void print_state_of_architectural_register_file(APEX_CPU *cpu)
{
    printf("\n================== STATE OF ARCHITECTURAL REGISTER FILE ================\n");

    for (int i = 0; i < ((sizeof(cpu->regs) / sizeof(cpu->regs[0]))); ++i)
    {
        // used char array of 10 because we know INVALID and VALID.
        char status[10];
        if (cpu->regCheck[i])
        {
            // would through invalid if the regCheck crosses 16 as we declared 16 already.
            strcpy(status, "INVALID");
        }

        else
            strcpy(status, "VALID");

        printf("|         REG[%d]         |     Value = %d       |     Status = %s     \n", i, cpu->regs[i], status);
    }
}

void print_state_of_data_memory(APEX_CPU *cpu)
{
    printf("\n ================ STATE OF DATA MEMORY ================\n");
    // declared the count as 70 as we know that teh dataMemory wont be much longer,
    // and will incerase when the load and store operations increases.
    for (int i = 0; i < 70; i++)
    {
        /*
         * prints the state of dataMemory set in Memory State.
         * for teh instructions like LDR and LOAD we set the moemory.
         */
        // prints the data_memory besed on the count.
        printf("|         MEM[%d]          |     Data Value = %d     |\n", i, cpu->data_memory[i]);
    }
}

void APEX_cpu_display_simulate_show_mem(APEX_CPU *cpu, int cyclesEntred, const char *functionType)
{
    // if display is entred in commandLine
    //  display function has to just display the executions till the number of cycles is reached mentioned in the command line.
    if (strcmp(functionType, "display") == 0)
    {
        printf("\n display   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   display\n");
        // int initialCount = cyclesEntred;
        for (int i = 0; i < cyclesEntred; cyclesEntred--)
        // decremetning the cycles and comparing they are not equal to 0.
        {
            // the same functionality as in the function "APEX_cpu_run(APEX_CPU *cpu)" but with out the single_step functionality
            // since the display method displays ony the no. of cycles entred.
            if (ENABLE_DEBUG_MESSAGES)
            {
                printf("--------------------------------------------\n");
                // as mentioned in the specifications clock cyle has to be printed from '1'.
                int clockCycle = cpu->clock + 1;
                printf("Clock Cycle #: %d\n", clockCycle);
                printf("--------------------------------------------\n");
            }
            // whole this functionality is in "APEX_cpu_run(APEX_CPU *cpu)"
            if (APEX_writeback(cpu))
            {
                int clockCycle = cpu->clock + 1;
                /* Halt in writeback stage */
                printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", clockCycle, cpu->insn_completed);
                break;
            }
            // followed teh same functionality as in "APEX_cpu_run(APEX_CPU *cpu)" because the
            // fetch decode execute memory and writeBack stages are same
            APEX_memory(cpu);
            APEX_execute(cpu);
            APEX_decode(cpu);
            APEX_fetch(cpu);
            print_reg_file(cpu);
            // incerementing teh clock cycles in cpu as it should go to new instruction every time
            cpu->clock++;
        }
        // we dont need even 'q' or 'Q' because the cycles run at a time and stop at last.
        // so not needed to quit the operation being executed.

        // since we are supposed to display the state of architectural registers and state of data memory
        // i'm caling print_state_of_architectural_register_file(APEX_CPU *cpu) and print_state_of_data_memory(APEX_CPU *cpu) functions
        print_state_of_architectural_register_file(cpu);
        print_state_of_data_memory(cpu);
    }

    // if simulate is entred in command line the simulation function will be continued till the no. of instructions we enter
    // in the command line ans continues with the single_line later.
    else if (strcmp(functionType, "simulate") == 0)
    {
        char user_prompt_val;
        printf("\nsimulate   #########################################################    simulate\n");
        for (int i = 0; i < cyclesEntred; --cyclesEntred)
        {
            // the same functionality as in the function "APEX_cpu_run(APEX_CPU *cpu)" but with out the single_step functionality
            // since the display method displays ony the no. of cycles entred.
            if (ENABLE_DEBUG_MESSAGES)
            {
                printf("--------------------------------------------\n");
                // as mentioned in the specifications clock cyle has to be printed from '1'.
                int clockCycle = cpu->clock + 1;
                printf("Clock Cycle #: %d\n", clockCycle);
                printf("--------------------------------------------\n");
            }
            // whole this functionality is in "APEX_cpu_run(APEX_CPU *cpu)"
            if (APEX_writeback(cpu))
            {
                int clockCycle = cpu->clock + 1;
                /* Halt in writeback stage */
                printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", clockCycle, cpu->insn_completed);
                break;
            }
            // followed teh same functionality as in "APEX_cpu_run(APEX_CPU *cpu)" because the
            // fetch decode execute memory and writeBack stages are same
            APEX_memory(cpu);
            APEX_execute(cpu);
            APEX_decode(cpu);
            APEX_fetch(cpu);
            print_reg_file(cpu);
            // incerementing teh clock cycles in cpu as it should go to new instruction every time
            cpu->clock++;
        }

        // same as in the "APEX_cpu_run(APEX_CPU *cpu)" fuctnion, since we are supposed to execute the instructions prefered in the command line argument
        // and if the whole numner of cycles are 100 for example, and if the command line argument is 80 we will run only the 80 cycles adn
        // then iniitate the single_step function.

        // here we are initiating the single step. which is already implemented in "APEX_cpu_run(APEX_CPU *cpu)"
        while (TRUE)
        {
            if (ENABLE_DEBUG_MESSAGES)
            {
                printf("--------------------------------------------\n");
                // as mentioned in the specifications clock cyle has to be printed from '1'.
                int clockCycle = cpu->clock + 1;
                printf("Clock Cycle #: %d\n", clockCycle);
                printf("--------------------------------------------\n");
            }

            if (APEX_writeback(cpu))
            {
                int clockCycle = cpu->clock + 1;
                /* Halt in writeback stage */
                printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", clockCycle, cpu->insn_completed);
                break;
            }

            APEX_memory(cpu);
            APEX_execute(cpu);
            APEX_decode(cpu);
            APEX_fetch(cpu);

            print_reg_file(cpu);

            if (cpu->single_step)
            {
                printf("Press any key to advance CPU Clock or <q> to quit:\n");
                scanf("%c", &user_prompt_val);

                if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
                {
                    int clockCycle = cpu->clock + 1;
                    printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", clockCycle, cpu->insn_completed);
                    break;
                }
            }
            cpu->clock++;
        }
        // since we are supposed to display the state of architectural registers and state of data memory
        // i'm caling print_state_of_architectural_register_file(APEX_CPU *cpu) and print_state_of_data_memory(APEX_CPU *cpu) functions
        print_state_of_architectural_register_file(cpu);
        print_state_of_data_memory(cpu);
    }

    // show_mem method displays the no. of instructions entred and prints the state of architecture
    else if (strcmp(functionType, "show_mem") == 0)
    {
        for (int i = 0; i < cyclesEntred; cyclesEntred--)
        {
            if (APEX_writeback(cpu))
            {
                int clockCycle = cpu->clock + 1;
                /* Halt in writeback stage */
                printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", clockCycle, cpu->insn_completed);
                // break;
            }
            // followed teh same functionality as in "APEX_cpu_run(APEX_CPU *cpu)" because the
            // fetch decode execute memory and writeBack stages are same
            APEX_memory(cpu);
            APEX_execute(cpu);
            APEX_decode(cpu);
            APEX_fetch(cpu);
            // tried not to print the register file as we don't want to print all those in the show_mem stage.
            // print_reg_file(cpu);
            // incerementing teh clock cycles in cpu as it should go to new instruction every time
            cpu->clock++;
        }
        print_state_of_architectural_register_file(cpu);
        print_state_of_data_memory(cpu);
    }
}
/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        // added the register rs3 value as while using STORE and LOAD we will be having 3 registers.
        printf("%-9s %-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2", "rs3",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            // added the register rs3 value as while using STORE and LOAD we will be having 3 registers.
            printf("%-9s %-9d %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].rs3, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    // printf(" has ins %d", cpu->fetch.has_insn);
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            // as mentioned in the specifications clock cyle has to be printed from '1'.
            int clockCycle = cpu->clock + 1;
            printf("Clock Cycle #: %d\n", clockCycle);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            int clockCycle = cpu->clock + 1;
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", clockCycle, cpu->insn_completed);
            break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                int clockCycle = cpu->clock + 1;
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", clockCycle, cpu->insn_completed);
                break;
            }
        }
        cpu->clock++;
        // print_state_of_architectural_register_file(cpu);
    }
    print_state_of_architectural_register_file(cpu);
    print_state_of_data_memory(cpu);
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}