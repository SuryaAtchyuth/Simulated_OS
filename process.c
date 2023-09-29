#include <pthread.h>
#include <semaphore.h>
#include "simos.h"


int currentPid = 2;    // user pid should start from 2, pid=0/1 are OS/idle
int numUserProcess = 0; 

//============================================
// context switch, switch in or out a process pid
//============================================

void context_in (int pid)
{ CPU.Pid = pid;
  CPU.PC = PCB[pid]->PC;
  CPU.AC = PCB[pid]->AC;
  CPU.PTptr = PCB[pid]->PTptr;
  CPU.exeStatus = PCB[pid]->exeStatus;
}

void context_out (int pid)
{ PCB[pid]->PC = CPU.PC;
  PCB[pid]->AC = CPU.AC;
  PCB[pid]->exeStatus = CPU.exeStatus;
  //fprintf (stdout, "\n\t*********** Iamin context out :time %d, CPU.exeStatus:%d***********\n",CPU.numCycles,CPU.exeStatus);
}

//=========================================================================
// Ready queue management  -------- MLFQ implemented wih given policies
// Implemented as a linked list with head and tail pointers
// The ready queue needs to be protected in case insertion comes from
// process submission and removal from process execution
//=========================================================================

#define nullReady 0
   // when get_ready_process encoutered empty queue, nullReady is returned

typedef struct ReadyNodeStruct
{ int pid;
  struct ReadyNodeStruct *next;
} ReadyNode;

ReadyNode *readyHead1 = NULL;
ReadyNode *readyTail1 = NULL;
ReadyNode *readyHead2 = NULL;
ReadyNode *readyTail2 = NULL;
ReadyNode *readyHead3 = NULL;
ReadyNode *readyTail3 = NULL;
ReadyNode *readyHead4 = NULL;
ReadyNode *readyTail4 = NULL;

// increments wait time after clock advances by 1 unit
void check_wait_time()
{
ReadyNode *node1;  
node1 = readyHead1;
while (node1 != NULL)
    {  PCB[node1->pid]->TQwaited += 1 ;
    //fprintf(stdout," TQwaited: %d, pid = %d\n",PCB[node1->pid]->TQwaited,node1->pid);
     node1 = node1->next; }

ReadyNode *node2;  
node2 = readyHead2;
while (node2 != NULL)
    { PCB[node2->pid]->TQwaited += 1 ;
    //fprintf(stdout," TQwaited: %d, pid = %d\n",PCB[node2->pid]->TQwaited,node2->pid);
     node2 = node2->next; }

ReadyNode *node3;    
node3 = readyHead3;
while (node3 != NULL)
    { PCB[node3->pid]->TQwaited += 1 ;
    //fprintf(stdout," TQwaited: %d, pid = %d\n",PCB[node3->pid]->TQwaited,node3->pid);
     node3 = node3->next; }

ReadyNode *node4;  
node4 = readyHead4;
while (node4 != NULL)
    { PCB[node4->pid]->TQwaited += 1 ;
    //fprintf(stdout," TQwaited: %d, pid = %d\n",PCB[node4->pid]->TQwaited,node4->pid);
     node4 = node4->next; }
}


//Detaches a node from present Queue and inserts in the above level Queue
void levelup_node( ReadyNode *node)
{
 int pid;
 pid = node->pid;
 int CurrentQ;
 CurrentQ = PCB[node->pid]->Qnum  ;

 // Level_Up from Q2 to Q1
 if(CurrentQ == 2)
 { 
   ReadyNode *temp = readyHead2;
   ReadyNode *prev = NULL;
  // if element is head
  if(temp != NULL && temp->pid == pid  )
  {
    readyHead2 = temp->next;   
    if(temp->next==NULL)
    {readyTail2 = NULL;}
    free(temp);
    insert_ready_process(pid,1);
    return;
  }
 //find element in the list
  while(temp != NULL && temp->pid != pid){
    prev = temp;
    temp = temp->next;
  }
  //if element is at tail
  if(temp->pid == readyTail2->pid)
   { prev->next=temp->next;
     readyTail2 == prev;
       free(temp);
       insert_ready_process(pid,1);
     return;
    }
    //if element is in middle
    else
    { prev->next=temp->next;
    free(temp);
    insert_ready_process(pid,1);
    return;
    }

 }
///

// Level_Up from Q3 to Q2
 if(CurrentQ == 3)
 { 
   ReadyNode *temp = readyHead3;
   ReadyNode *prev = NULL;
  // if element is head
  if(temp != NULL && temp->pid == pid  )
  {
    readyHead3 = temp->next;
    if(temp->next==NULL)
    {readyTail2 = NULL;}
    free(temp);
    insert_ready_process(pid,2);
    return;
  }
 //find element in the list
  while(temp != NULL && temp->pid != pid){
    prev = temp;
    temp = temp->next;
  }
  //if element is at tail
  if(temp->pid == readyTail3->pid)
   { prev->next=temp->next;
     readyTail3 == prev;
     free(temp);
     insert_ready_process(pid,2);
     return;
    }
    //if element is in middle
    else
    { prev->next=temp->next;
    free(temp);
    insert_ready_process(pid,2);
    return;
    }

 }
///
// Level_Up from Q4 to Q3
 if(CurrentQ == 4)
 { 
   ReadyNode *temp = readyHead4;
   ReadyNode *prev = NULL;
  // if element is head
  if(temp != NULL && temp->pid == pid  )
  {
    readyHead4 = temp->next;
   if(temp->next==NULL)
    {readyTail2 = NULL;}
    free(temp);
    insert_ready_process(pid,3);
    return;
  }
 //find element in the list
  while(temp != NULL && temp->pid != pid){
    prev = temp;
    temp = temp->next;
  }
  //if element is at tail
  if(temp->pid == readyTail4->pid)
   { prev->next=temp->next;
     readyTail4 == prev;
     free(temp);
     insert_ready_process(pid,3);
     return;
    }
    //if element is in middle
    else
    { prev->next=temp->next;
    free(temp);
    insert_ready_process(pid,3);
    return;
    }

 }

}



/// to insert into ready process
void insert_ready_process (int pid,int Qnumber)
{  ///submitted process or process alredy present in Q1 but not expired time quantum
  if((Qnumber == 0) ||( Qnumber== 1) )
  {ReadyNode *node;
  
  node = (ReadyNode *) malloc (sizeof (ReadyNode));
  node->pid = pid;
  node->next = NULL;
  PCB[pid]->RQintime=CPU.numCycles;  
  if (readyTail1 == NULL) // readyHead would be NULL also
    { readyTail1 = node; readyHead1 = node; }
  else // insert to tail
    { readyTail1->next = node; readyTail1 = node; }
  PCB[pid]->Qnum = 1;
  if(cpuDebug){
   fprintf(bugF,"\tProcess %d Entered Ready Queue-01 at Time:%d\n",pid, CPU.numCycles);
   }
  PCB[pid]->TQwaited =0;
  }

  else if(Qnumber==2)
  {ReadyNode *node;
  
  node = (ReadyNode *) malloc (sizeof (ReadyNode));
  node->pid = pid;
  node->next = NULL;
  PCB[pid]->RQintime=CPU.numCycles;  
  if (readyTail2 == NULL)  // readyHead would be NULL also
    { readyTail2 = node; readyHead2 = node; }
  else // insert to tail
    { readyTail2->next = node; readyTail2 = node; }
  PCB[pid]->Qnum = 2;
  if(cpuDebug){
  fprintf(bugF,"\tProcess %d Entered Ready Queue-02 at Time:%d\n",pid, CPU.numCycles);
  }
  PCB[pid]->TQwaited =0;
  }

else if(Qnumber==3)
  {ReadyNode *node;
  
  node = (ReadyNode *) malloc (sizeof (ReadyNode));
  node->pid = pid;
  node->next = NULL;
  PCB[pid]->RQintime=CPU.numCycles;  
  if (readyTail3 == NULL) // readyHead would be NULL also
    { readyTail3 = node; readyHead3 = node; }
  else // insert to tail
    { readyTail2->next = node; readyTail3 = node; }
  PCB[pid]->Qnum = 3;
  if(cpuDebug){fprintf(bugF,"\tProcess %d Entered Ready Queue-03 at Time:%d\n",pid, CPU.numCycles);}
  PCB[pid]->TQwaited =0;
  }
else if(Qnumber==4)
  {ReadyNode *node;
  
  node = (ReadyNode *) malloc (sizeof (ReadyNode));
  node->pid = pid;
  node->next = NULL;
  PCB[pid]->RQintime=CPU.numCycles;  
  if (readyTail4 == NULL) // readyHead would be NULL also
    { readyTail4 = node; readyHead4 = node; }
  else // insert to tail
    { readyTail4->next = node; readyTail4 = node; }
  PCB[pid]->Qnum = 4;
  if(cpuDebug){ fprintf(bugF,"\tProcess %d Entered Ready Queue-04 at Time:%d\n",pid, CPU.numCycles);}
  PCB[pid]->TQwaited =0;
  }

}

//implements round robin in each queue level
int get_ready_process ()
{ ReadyNode *rnode;
  int pid;

  if (readyHead1 == NULL)
  { 
    if (readyHead2 == NULL)
  
  { if (readyHead3 == NULL)
 
  { if (readyHead4 == NULL)
  { if (cpuDebug) fprintf (bugF, "No ready process now!!!\n");
    return (nullReady); 
  }
  else
  {pid = readyHead4->pid;
   if(cpuDebug){ fprintf(bugF,"\tProcess %d Leaving Ready Queue-04 at Time:%d\n",pid, CPU.numCycles);}
    rnode = readyHead4;
    readyHead4 = rnode->next;
    free (rnode);
    if (readyHead4 == NULL) readyTail4 = NULL;
  }
  }

  else
  { pid = readyHead3->pid;
    if(cpuDebug){fprintf(bugF,"\tProcess %d Leaving Ready Queue-03 at Time:%d\n",pid, CPU.numCycles);}
    rnode = readyHead3;
    readyHead3 = rnode->next;
    free (rnode);
    if (readyHead3 == NULL) readyTail3 = NULL;
  }
  }

  else
  { pid = readyHead2->pid;
    if(cpuDebug){fprintf(bugF,"\tProcess %d Leaving Ready Queue-02 at Time:%d\n",pid, CPU.numCycles);}
    rnode = readyHead2;
    readyHead2 = rnode->next;
    free (rnode);
    if (readyHead2 == NULL) readyTail2 = NULL;
  }
  }

  else
  { pid = readyHead1->pid;
    if(cpuDebug){fprintf(bugF,"\tProcess %d Leaving Ready Queue-01 at Time:%d\n",pid, CPU.numCycles);}
    rnode = readyHead1;
    readyHead1 = rnode->next;
    free (rnode);
    if (readyHead1 == NULL) readyTail1 = NULL;
  }
  return (pid);
}

void dump_ready_queue (FILE *outf)
{ ReadyNode *node;

  fprintf (outf, "******************** Ready Queue 1 Dump(Time Quantum=%d)\n",cpuQuantum);
  node = readyHead1;
  while (node != NULL)
    { fprintf (outf, "PID=%d TW=%d, ", node->pid,PCB[node->pid]->TQwaited); node = node->next; }
  fprintf (outf, "\n");
  fprintf (outf, "******************** Ready Queue 2 Dump(Time Quantum=%d)\n",2*cpuQuantum);
  node = readyHead2;
   while (node != NULL)
    {  fprintf (outf, "PID=%d TW=%d, ", node->pid,PCB[node->pid]->TQwaited); node = node->next; }
  fprintf (outf, "\n");
  fprintf (outf, "******************** Ready Queue 3 Dump(Time Quantum=%d)\n",3*cpuQuantum);
  node = readyHead3;
   while (node != NULL)
    {  fprintf (outf, "PID=%d TW=%d, ", node->pid,PCB[node->pid]->TQwaited); node = node->next; }
  fprintf (outf, "\n");
  fprintf (outf, "******************** Ready Queue 4 Dump(Time Quantum=%d)\n",4*cpuQuantum);
  node = readyHead4;
   while (node != NULL)
    {  fprintf (outf, "PID=%d TW=%d, ", node->pid,PCB[node->pid]->TQwaited); node = node->next; }
  fprintf (outf, "\n");
}


//=========================================================================
// endIO list management
// processes that has finished waiting can be inserted into endIO list
//   -- when adding process to endIO list, should set endIO interrupt,
//      interrupt handler will move processes in endIO list to ready queue
// The list needs to be protected because multiple threads may insert
// to endIO list and a thread will remove nodes in the list concurrently
//=========================================================================

sem_t pmutex;

typedef struct EndIOnodeStruct
{ int pid;
  struct EndIOnodeStruct *next;
} EndIOnode;

EndIOnode *endIOhead = NULL;
EndIOnode *endIOtail = NULL;

void insert_endIO_list (int pid)
{ EndIOnode *node;

  sem_wait (&pmutex);
  node = (EndIOnode *) malloc (sizeof (EndIOnode));
  node->pid = pid;
  node->next = NULL;
  if (endIOtail == NULL) // endIOhead would be NULL also
    { endIOtail = node; endIOhead = node; }
  else // insert to tail
    { endIOtail->next = node; endIOtail = node; }
  sem_post (&pmutex);
}

// move all processes in endIO list to ready queue, empty the list
// need to set exeStatus from eWait to eReady

void endIO_moveto_ready ()
{ EndIOnode *node;

  sem_wait (&pmutex);
  while (endIOhead != NULL)
  { node = endIOhead;
    insert_ready_process (node->pid,PCB[node->pid]->Qnum);
    PCB[node->pid]->exeStatus = eReady;
    endIOhead = node->next;
    free (node);
  }
  endIOtail = NULL;
  sem_post (&pmutex);
}

void dump_endIO_list (FILE *outf)
{ EndIOnode *node;

  node = endIOhead;
  fprintf (outf, "endIO List = ");
  while (node != NULL)
    { fprintf (outf, "%d, ", node->pid); node = node->next; }
  fprintf (outf, "\n");
}

//=========================================================================
// Some support functions for PCB 
// PCB related definitions are in simos.h
//=========================================================================

void init_PCB_ptrarry ()
{ PCB = (typePCB **) malloc (maxProcess*addrSize); }

int new_PCB ()
{ int pid;

  pid = currentPid;
  currentPid++;
  if (pid >= maxProcess)
  { fprintf (infF, "\aExceeding maximum number of processes: %d\n", pid);
    // because we do not reuse pid, pid may run out, use max to
    // protect against potential integer overflow (though very unlikely)
    return (-1);
  }
  PCB[pid] = (typePCB *) malloc ( sizeof(typePCB) );
  PCB[pid]->Pid = pid;
  PCB[pid]->timeUsed = 0;
  PCB[pid]->RQintime = 0;   //assigned Manually.
  PCB[pid]->Qnum = 0;       //just submitted process // mod ph2   
  PCB[pid]->Qnew = 0;       // mod ph2  
  PCB[pid]->TQwaited = 0;   // mod ph2  
  PCB[pid]->numPF = 0;
  return (pid);
}

void free_PCB (int pid)
{
  free (PCB[pid]);
  if (cpuDebug) fprintf (bugF, "Free PCB: %d\n", PCB[pid]);
  PCB[pid] = NULL;
}

void dump_PCB (FILE *outf, int pid)
{
  fprintf (outf, "\t## PCB Dump for Process %d ##\n", pid);
  fprintf (outf, "\t______________________\n");
  fprintf (outf, "\t      PID = %d        \n", PCB[pid]->Pid);
  fprintf (outf, "\tPC = %d               \n", PCB[pid]->PC);
  fprintf (outf, "\tAC = "mdOutFormat"    \n", PCB[pid]->AC);
  fprintf (outf, "\tPTptr = %x            \n", PCB[pid]->PTptr);
  fprintf (outf, "\texeStatus = %d        \n", PCB[pid]->exeStatus);
  fprintf (outf, "\tRQInTime = %d         \n", PCB[pid]->RQintime);   // Manually Updated.
  fprintf (outf, "\tQnum = %d             \n", PCB[pid]->Qnum); // mod ph2 
  fprintf (outf, "\tNOP = %d, ProgLen = %d\n", PCB[pid]->NOP,PCB[pid]->progLen); 
  fprintf (outf, "\t______________________\n"); 
}

void dump_PCB_list (FILE *outf)
{ int pid;

  fprintf (outf, "********** Dump all PCB: From 0 to %d **********\n", currentPid);
  for (pid=idlePid; pid<currentPid; pid++)
    if (PCB[pid] != NULL) dump_PCB (outf, pid);
}

void dump_PCB_memory (FILE *outf)
{ int pid;

  fprintf (outf,
           "Dump memory/swap of all processes: From 1 to %d\n", currentPid-1);
  // dump_process_memory (outf, idlePid);
  for (pid=idlePid+1; pid<currentPid; pid++)
    if (PCB[pid] != NULL) dump_process_memory (outf, pid);
}


//=========================================================================
// process management
//=========================================================================

//#include "idle.c"

void clean_process (int pid)
{
  free_process_memory (pid);
  free_PCB (pid);  // PCB has to be freed last, other frees use PCB info
} 

void exiting_process (int pid)
{ PCB[pid]->exeStatus = CPU.exeStatus;
    // PCB[pid] is not updated, no point to do a full context switch

  // send exiting process printout to term.c, str will be freed by term.c
  char *str = (char *) malloc (80);
  if (CPU.exeStatus == eError)
  { fprintf (infF, "\aProcess %d has an error, dumping its states\n", pid);
    dump_PCB (infF, pid);
    dump_process_memory (infF, pid);
    printf("*-->IRopcode:%d, IRoperand:%d\n",CPU.IRopcode,CPU.IRoperand); 
    sprintf (str, "Process %d had encountered error in execution!!!\n", pid);
  }
  else  // was eEnd
  { fprintf (infF, "Process %d had completed successfully: Time=%d, PF=%d\n",
             pid, PCB[pid]->timeUsed, PCB[pid]->numPF);
    sprintf (str, "Process %d had completed successfully: Time=%d, PF=%d\n",
             pid, PCB[pid]->timeUsed, PCB[pid]->numPF);
  }
  insert_termIO (pid, str, exitProgIO);

  // invoke io to print str, process has terminated, so no wait state

  numUserProcess--;
  clean_process (pid); 
    // cpu will clean up process pid without waiting for printing to finish
    // so, io should not access PCB[pid] for end process printing
}

void initialize_process_manager ()
{
  init_PCB_ptrarry ();

  currentPid = 2;  // the next pid value to be used
  numUserProcess = 0;  // the actual number of processes in the system

  init_idle_process ();
  sem_init (&pmutex, 0, 1);
}

//================================================================
// submit_process always works on a new pid and the new pid will not be 
// used by anyone else till submit_process finishes working on it
// currentPid is not used by anyone else but the dump functions
// So, no conflict for PCB and Pid related data
// -----------------
// During insert_ready_process, there is potential of conflict accesses
//================================================================

int submit_process (char *fname)
{ int pid, ret, i;
    //fprintf(outf,"\n\tProcess Entering Ready Queue Time: %d\n",CPU.numCycles);
  // if there are too many processes s.t. each cannot get sufficient memory
  // then reject the process
  if ( ((numFrames-OSpages)/(numUserProcess+1)) < 2 ){
    fprintf(infF,"numFrames : %d\n OSpages: %d\n numUserProcess : %d\n",numFrames,OSpages,numUserProcess);
    fprintf (infF,"\aToo many processes => they may not execute properly due to page faults\n");
    }
  else
  { pid = new_PCB ();
    if (pid > idlePid)
    { ret = load_process (pid, fname);   // return #pages loaded
      if (ret > 0)  // loaded successfully
      { PCB[pid]->PC = 0;
        PCB[pid]->AC = 0;
        PCB[pid]->exeStatus = eReady;
        PCB[pid]->RQintime=CPU.numCycles;
        // swap manager will put the process to endIO list and then
        // process.c will eventually move it to ready queue
        // at this point, the process may not be loaded yet, but no problem
        numUserProcess++;
        return (pid);  // the only case of successful process creation
      }
      // else new_PCB returned -1, PCB has not been created
  } }
  // failed, PCB has not been created, exitProg
  char *str = (char *) malloc (80);
  fprintf (infF, "\aProgram %s has loading problem!!!\n", fname);
  sprintf (str, "Program %s has loading problem!!!\n", fname);
  insert_termIO (pid, str, exitProgIO);
  return (-1);
}

//================================================================
// execute_process: prepare; execute instruction; subsequent processing
// -----------------
// During insert_ready_process, there is potential of conflict accesses
//================================================================

void execute_process ()
{ 
  int pid, intime;
  genericPtr event;
  
  //checking and implementing MLFQ policy of if a process waits for 2 TQ of its level,
  //it will be send to upper level
ReadyNode *node2;  
node2 = readyHead2;
while (node2 != NULL)
    { if(PCB[node2->pid]->TQwaited >= (4*cpuQuantum ) )
    {
      levelup_node(node2);
    }
     node2 = node2->next; }

ReadyNode *node3;  
node3 = readyHead3;
while (node3 != NULL)
    { if(PCB[node3->pid]->TQwaited >= (6*cpuQuantum ) )
    {
      levelup_node(node3);
    }
     node3 = node3->next; }

ReadyNode *node4;  
node4 = readyHead4;
while (node4 != NULL)
    { if(PCB[node4->pid]->TQwaited >= (8*cpuQuantum ) )
    {
     levelup_node(node4);
    }
     node4 = node4->next; }


  pid = get_ready_process ();
  if (pid != nullReady)
    // execute the ready process (with pid# = pid)
    //   before and after the execution, need to do:
    //   (1) context switch,
    //   (2) set execution status + check status to do subsequent actions
    //   (3) set timer to stop execution at the time quantum
    //   (4) accounting: add execution time to PCB[?]->timeUsed,
  { context_in (pid);   // === (1) 
    CPU.exeStatus = eRun;   // === (2) 
    intime = CPU.numCycles;   // ===(4) 
    //(MLFQ policy 1 implementation)
   int Qlevel=PCB[pid]->Qnum;
    event = add_timer ((Qlevel*cpuQuantum), CPU.Pid,  // == (3) 
                       actTQinterrupt, oneTimeTimer);
    cpu_execution ();
    context_out (pid);  // === (1)
    PCB[pid]->timeUsed += (CPU.numCycles - intime);  // ===(4)
    if(cpuDebug){
    		fprintf(bugF,"\n\tCurrent Process Execution Time: %d\n", CPU.numCycles - intime);
    }   // Manually Updated.
    //inserts process into ready queue after time quantum expiration
    if (CPU.exeStatus == eReady) insert_ready_process (pid,PCB[pid]->Qnew);  // === (2)
    else if (CPU.exeStatus == ePFault || CPU.exeStatus == eWait) 
      // eWait: should have been handled by instruction execution
      // ePFault: calculate_memory_address should have set pFaultException,
      //   which is subsequently handled by page_fault_handler
      deactivate_timer (event);
    else // CPU.exeStatus == eError or eEnd, exiting
      { exiting_process (pid); deactivate_timer (event); }
    // Why deactivate_timer?
    //   If exeStatus != eReady ==> process is not stopped by time quantum
    //   but timer is still there ==> should be deactivated
    //   To deactivate: need the returned ptr from set timer: "event"
    //   If not: it will impact the execution of the next process
    // But if time quantum just expires and exeStatus != eReady
    //   No problem! eReady is only set upon tqInterrupt (in cpu.c)
    //    if the process had ePFault/eWait, it will not be set to eReady
  }
  else execute_idle_process ();

      

    // no ready process in the system, so execute idle process
    // ===== see https://en.wikipedia.org/wiki/System_Idle_Process
}

