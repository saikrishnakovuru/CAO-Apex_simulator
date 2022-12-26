/*
 * apex_cpu.h
 * Contains APEX cpu pipeline declarations
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_

#include "apex_macros.h"

struct flagCheck
{
  int inUse;
  int notInUse;
  int isRegisterValueEmpty;
} flagCheck;

/* Format of an APEX instruction  */
typedef struct APEX_Instruction
{
  char opcode_str[128];
  int opcode;
  int rd;
  int rs1;
  int rs2;
  // added for STR as STR's input looks like "STR R3,R4,R5" and we need the third variable,
  // and in addition R3 is also a known value unless like rd
  int rs3;
  int imm;
} APEX_Instruction;

/* Model of CPU stage latch */
typedef struct CPU_Stage
{
  int pc;
  char opcode_str[128];
  int opcode;
  int rs1;
  int rs2;
  // added for STR as STR's input looks like "STR R3,R4,R5" and we need the third variable,
  // and in addition R3 is also a known value unless like rd
  int rs3;
  int rd;
  int imm;
  int rs1_value;
  int rs2_value;
  // added for STR as STR's input which looks like "STR R3,R4,R5" and we need the third variable,
  // and in addition R3 is also a known value unless like rd.
  int rs3_value;
  int result_buffer;
  int memory_address;
  int has_insn;
  // created as we have to check the flag for stalling functionality.
  int is_stalled;
} CPU_Stage;

/* Model of APEX CPU */
typedef struct APEX_CPU
{
  int pc;                            /* Current program counter */
  int clock;                         /* Clock cycles elapsed */
  int insn_completed;                /* Instructions retired */
  int regs[REG_FILE_SIZE];           /* Integer register file */
  int code_memory_size;              /* Number of instruction in the input file */
  APEX_Instruction *code_memory;     /* Code Memory */
  int data_memory[DATA_MEMORY_SIZE]; /* Data Memory */
  int single_step;                   /* Wait for user input after every cycle */
  int zero_flag;                     /* {TRUE, FALSE} Used by BZ and BNZ to branch */
  int fetch_from_next_cycle;
  int regCheck[REG_FILE_SIZE];

  /* Pipeline stages */
  CPU_Stage fetch;
  CPU_Stage decode;
  CPU_Stage int_operations;
  CPU_Stage mul_operation;
  CPU_Stage load_operations;

  CPU_Stage writeback;
} APEX_CPU;

APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *filename);
void APEX_cpu_run(APEX_CPU *cpu);
void APEX_cpu_stop(APEX_CPU *cpu);
// added to perforn display simulate and show_mem operations.
void APEX_cpu_display_simulate_show_mem(APEX_CPU *cpu, int cyclesEntred, const char *functionType);
#endif
