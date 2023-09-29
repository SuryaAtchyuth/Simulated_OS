// =====================================================
// see https://en.wikipedia.org/wiki/System_Idle_Process
//    for more information about idle process
// ===
// This version of the idle process is dummy
//   Idle process is now totally decoupled with regular processes
//     purpose: no longer calls loader/paging functitons
//     making the implementation of loader/paging more independent
//   Idle process still has a PCB
//   Each execution will check interrupt, sleep, advance clock
//     no time quantum setting, no memory accesses
//   Other functions that are called from outside  become dummy now
//
// External changes required
//   process.c:  dump_process_memory (outf, idlePid);  -- commented out
//               dump_PCB starts from idle:  -- it is ok, idle has a PCB


#include "simos.h"


//=========================
// load idle process. Called from loader.c only.
//=========================

void load_idle_process ()
{ }


//=========================
// init and execute idle process. Called from process.c only.
//=========================

void init_idle_process ()
{ 
  // create and initialize PCB for the idle process
  PCB[idlePid] = (typePCB *) malloc ( sizeof(typePCB) );

  PCB[idlePid]->Pid = idlePid;  // idlePid = 1, set in ???
  PCB[idlePid]->PC = 0;
  PCB[idlePid]->AC = 0;
  load_idle_process ();
  if (cpuDebug)
    { dump_PCB (bugF, idlePid); dump_process_memory (bugF, idlePid); }
}

void execute_idle_process ()
{ // context_in (idlePid);
  // CPU.exeStatus = eRun;
  // add_timer (idleQuantum, CPU.Pid, actTQinterrupt, oneTimeTimer);
  //    -- number steps executing idle process = its time quantum
  // cpu_execution (); 
  //    -- extracted out the statements as below

  int i;
  for (i=0; i<idleQuantum; i++)
  { if (CPU.interruptV != 0) handle_interrupt ();
    usleep (instrTime); 
    advance_clock ();
  }
}

