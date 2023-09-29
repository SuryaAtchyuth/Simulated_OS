#include "simos.h"

#define ADMIN "Batch-02"
void execute_process_iteratively ()
{ int round, i;

  fprintf (infF, "Iterative execution: #rounds? ");
  scanf ("%d", &round);
  for (i=0; i<round; i++) execute_process();
}

void one_admin_command (char act)
{ char fname[100];

  switch (act)
  { case 'T':  // Terminate simOS
      systemActive = 0; break;
    case 's':  // submit a program: should not be admin's command
      program_submission (); break;
    case 'x':  // execute once, duplicate with y, but more convenient
      execute_process (); break;
    case 'y':  // multiple rounds of execution
      execute_process_iteratively (); break;
    case 'q':  // dump ready queue and list of processes completed IO
      dump_ready_queue (stdout); dump_endIO_list (stdout); break;
    case 'r':   // dump the list of available PCBs
      dump_registers (stdout); break;
    case 'p':   // dump the list of available PCBs
      dump_PCB_list (stdout); break;
    case 'm':   // dump memory of each process
      dump_PCB_memory (stdout); break;
    case 'f':   // dump memory frames and free frame list
      dump_memoryframe_info (stdout); break;
    case 'n':   // dump the content of the entire memory
      dump_memory (stdout); break;
    case 'e':   // dump events in clock.c
      dump_events (stdout); break;
    case 't':   // dump terminal IO queue
      dump_termIO_queue (stdout); break;
    case 'w':   // dump swap queue
      dump_swapQ (stdout); break;
    case 'c':
      system("clear");system("date");break;
    default:   // can be used to yield to client submission input
      fprintf (infF, "Error: Incorrect command!!!\n");
  }
}

void process_admin_commands ()
{ char action[10];

  while (systemActive)
  { fprintf (infF, "%s: $", ADMIN);
    scanf ("%s", action);
    if (uiDebug) fprintf (bugF, "Command issued: %c\n", action[0]);
    // only first action character counts, discard remainder
    // character string is used to capture <ret>
    one_admin_command (action[0]);
  }
}

