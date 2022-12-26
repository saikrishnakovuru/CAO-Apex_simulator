/*
 * main.c
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apex_cpu.h"

int main(int argc, char const *argv[])
{
  APEX_CPU *cpu;

  // fprintf(stderr, "APEX CPU Pipeline Simulator v%0.1lf\n", VERSION);
  // input arguments entred must be always between 2 and 4,
  // cannot be less than 2 or more than 4.
  switch (argc)
  {
  case 2:
  case 3:
  case 4:
  {
    // only acceptes the input values between 2 to 4 else the error would be thrown saying
    // the printf statemetn in the default statement.
    break;
  }
  default:
  {
    // default message will be printed if teh  input arguments less than 2 or greater than 4.
    fprintf(stderr, "APEX_Help: Usage %s <input_file>\n", argv[0]);
    exit(1);
  }
  }

  cpu = APEX_cpu_init(argv[1]);
  if (!cpu)
  {
    // if argument one is not the valid one then we have to throe an error message,
    // the first argument must be apex_sim
    // if not we will the thrown as error saying "APEX_Error: Unable to initialize CPU".
    fprintf(stderr, "APEX_Error: Unable to initialize CPU\n");
    exit(1);
  }

  // the argument lenght must be greater than 2 and must be less than 4.
  // enters into this function when arguments are greater than 2 or 3 or equla to 4.
  // that means we are entering either simulate or display or show_mem followed by the no. of cycles.
  if (argc > 2 && argc > 3 && argc == 4)
  {
    APEX_cpu_display_simulate_show_mem(cpu, atoi(argv[3]), argv[2]);
    APEX_cpu_stop(cpu);
  }

  // if we only run the simulator only eith the two commands then the APEX_cup run and stop will be executed
  // sinlge_step prodess will the executes we have to keep pressing command/enter to execute the further cycles.
  else if (argc == 2 && !(argc == 3) && !(argc == 4))
  {
    APEX_cpu_run(cpu);
    APEX_cpu_stop(cpu);
  }
  return 0;
}
