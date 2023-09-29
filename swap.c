#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include "simos.h"


//======================================================================
// This module handles swap space management.
// It has the simulated disk and swamp manager.
// First part is for the simulated disk to read/write pages.
//======================================================================

#define swapFname "swap.disk"
#define itemPerLine 8
int diskfd;
int swapspaceSize;
int PswapSize;
int pagedataSize;


//===================================================
// This is the simulated disk, including disk read, write, dump.
// The unit is a page
//===================================================
// each process has a fix-sized swap space, its page count starts from 0
// first 2 processes: OS=0, idle=1, have no swap space 
// OS frequently (like Linux) runs on physical memory address (fixed locations)
// virtual memory is too expensive and unnecessary for OS => no swap needed

// move to proper file location before read/write/dump
int move_filepointer (int pid, int page)
{ int currentoffset, newlocation, ret;

  if (pid <= idlePid || pid > maxProcess) 
  { fprintf (infF, "Error: Incorrect pid for disk dump: %d\n", pid);
    exit (-1); }
  if (page < 0 || page > maxPpages)
  { fprintf (infF, "Error: Incorrect page number: pid=%d, page=%d\n",
                   pid, page); 
    exit (-1);
  }
  currentoffset = lseek (diskfd, 0, SEEK_CUR);
  newlocation = (pid-2) * PswapSize + page*pagedataSize;
  ret = lseek (diskfd, newlocation, SEEK_SET);
  if (ret < 0) 
  { fprintf (infF, "Error lseek in move: ");
    fprintf (infF, "pid/page=%d,%d, loc=%d,%d, size=%d\n",
             pid, page, currentoffset, newlocation, pagedataSize);
    exit (-1);
  }
  return (currentoffset);
    // currentoffset cannot be larger than 2G
}

// originally prepared for dump, now not in use
void moveback_filepointer (int location)
{ int ret;

  ret = lseek (diskfd, location, SEEK_SET);
  if (ret < 0) 
  { fprintf (infF, "Error lseek in moveback: ");
    fprintf (infF, "location=%d\n", location);
    exit (-1);
  }
}

// read/write are called within swap thread, dump is called externally
// if called concurrently, they may change file location => error IO
// Solution: use mutex semaphore to protect them
// each function recomputes address, so there will be no problem

int read_swap_page (int pid, int page, unsigned *buf)
{ int ret;

  move_filepointer (pid, page);
  ret = read (diskfd, (char *)buf, pagedataSize);
  if (ret != pagedataSize) 
  { fprintf (infF, "Error: Disk read returned incorrect size: %d\n", ret); 
    exit(-1);
  }
  usleep (diskRWtime);  // simulate the delay for disk RW
}

int write_swap_page (int pid, int page, unsigned *buf)
{ int ret;

  move_filepointer (pid, page);
  ret = write (diskfd, (char *)buf, pagedataSize);
  if (ret != pagedataSize) 
  { fprintf (infF, "Error: Disk write returned incorrect size: %d\n", ret); 
    exit(-1);
  }
  usleep (diskRWtime);  // simulate the delay for disk RW
}

void dump_process_swap_page (FILE *outf, int pid, int page)
{ int oldloc, ret, count, k;
  int buf[pageSize];
  
  oldloc = move_filepointer (pid, page);
  ret = read (diskfd, (char *)buf, pagedataSize);
  if (ret != pagedataSize) 
  { fprintf (infF, "Error: Disk dump read incorrect size: %d\n", ret); 
    exit (-1);
  }

  fprintf (outf, "Content of process %d swap page %d:\n", pid, page);
  count = 0;
  for (k=0; k<pageSize; k++)
  { fprintf (outf, "%x ", buf[k]);
    count++; if (count == itemPerLine) { fprintf (outf, "\n"); count = 0; }
  }
  fprintf (outf, "\n");
}

// open the file with the swap space size, initialize content to 0
void initialize_swap_space ()
{ int ret, i, j, k;
  int buf[pageSize];

  swapspaceSize = maxProcess*maxPpages*pageSize*dataSize;
  PswapSize = maxPpages*pageSize*dataSize;
  pagedataSize = pageSize*dataSize;

  diskfd = open (swapFname, O_RDWR | O_CREAT, 0600);
  if (diskfd < 0) { perror ("Error open: "); exit (-1); }
  ret = lseek (diskfd, swapspaceSize, SEEK_SET); 
  if (ret < 0) { perror ("Error lseek in open: "); exit (-1); }
  for (i=2; i<maxProcess; i++)
    for (j=0; j<maxPpages; j++)
    { for (k=0; k<pageSize; k++) buf[k]=0;
      write_swap_page (i, j, buf);
    }
    // last parameter is the origin, offset from the origin, which can be:
    // SEEK_SET: 0, SEEK_CUR: from current position, SEEK_END: from eof
}


//===================================================
// Here is the swap space manager. Its main job is SwapQ management.
// Swap manager (disk device) runs in paralel with CPU-memory
// A swap request (read/write a page) is inserted to the swapQ (queue)
//    by other processes (loader, memory)
// Swap manager takes jobs from SwapQ and process them
// After a job is completed, it is pushed to endIO queue
//===================================================

typedef struct SwapQnodeStruct
{ int pid, page, act, finishact;
  unsigned *buf;
  struct SwapQnodeStruct *next;
} SwapQnode;
// pidin, pagein, inbuf: for the page with PF, needs to be brought in
// pidout, pageout, outbuf: for the page to be swapped out
// if there is no page to be swapped out (not dirty), then pidout = nullPid
// inbuf and outbuf are the actual memory page content

SwapQnode *swapQhead = NULL;
SwapQnode *swapQtail = NULL;

void print_one_swapnode (FILE *outf, SwapQnode *node)
{ fprintf (outf, "pid,page=(%d,%d), act,fact=(%d, %d), buf=%x\n", 
           node->pid, node->page, node->act, node->finishact, node->buf);
}

void dump_swapQ (FILE *outf)
{ SwapQnode *node;

  fprintf (outf, "******************** Swap Queue Dump\n");
  node = swapQhead;
  while (node != NULL)
    { fprintf (outf, "pid,page=(%d,%d), act,fact=(%d, %d), buf=%x\n", 
              node->pid, node->page, node->act, node->finishact, node->buf);
      node = node->next;
    }
  fprintf (outf, "\n");
}

void insert_swapQ (pid, page, buf, act, finishact)
int pid, page, act, finishact;
unsigned *buf;
{ SwapQnode *node;

  node = (SwapQnode *) malloc (sizeof (SwapQnode));
  node->pid = pid;
  node->page = page;
  node->buf = buf;
  node->act = act;
  node->finishact = finishact;
  if (swapDebug) { fprintf (bugF, "Insert swapQ ");
                   print_one_swapnode (bugF, node); }

  node->next = NULL;
  if (swapQtail == NULL) // swapQhead would be NULL also
    { swapQtail = node; swapQhead = node; }
  else // insert to tail
    { swapQtail->next = node; swapQtail = node; }
  if (swapDebug) dump_swapQ (bugF);
}

void process_one_swap ()
{ SwapQnode *node;

  // get one request from the head of the swap queue and process it
  if (swapQhead == NULL)
  { if (systemActive)
      fprintf (infF, "\aError: No process in swap queue!!!\n");
  }
  else
  { node = swapQhead;
    if (node->act == actWrite) 
      write_swap_page (node->pid, node->page, node->buf);
    else   // act == actRead
      read_swap_page (node->pid, node->page, node->buf);
    if (swapDebug) 
    { fprintf (bugF, "One Swap: ");
      dump_process_swap_page (bugF, node->pid, node->page); }

    // perform the finish action
    if (node->finishact == toReady || node->finishact == Both)
    { // *** ADD CODE: What to do if finsihact is "toReady" or "Both"?
      insert_endIO_list(node->pid);
      set_interrupt(endIOinterrupt);
    }
    if (node->finishact == freeBuf || node->finishact == Both)
      if (node->act == actWrite) free (node->buf);
      else // node->act == actRead, buf is to be returned, not freed
        { fprintf (infF, "Error: freeBuf requested for actRead\n"); exit(-1); }
    // else if (node->finishact == Nothing) ; // Do nothing

    if (swapDebug)
      { fprintf (bugF, "One Swap: "); print_one_swapnode (bugF, node); }
    swapQhead = node->next;
    if (swapQhead == NULL) swapQtail = NULL;
    free (node);
    if (swapDebug) dump_swapQ (bugF);
  }
}

void *process_swapQ ()
{
  while (systemActive) process_one_swap ();
  if (swapDebug) fprintf (bugF, "swapQ loop has ended\n");
}

//===================================================
// Initialization and cleaning up
// A thread is created here for the swap manager (invoked by system.c)
//===================================================

pthread_t swapThread;

void start_swap_manager ()
{ int ret;

  initialize_swap_space ();
  // *** ADD CODE: create swap thread, init semaphores for synchronization
}

void end_swap_manager ()
{ int ret;

  close (diskfd);
  // *** ADD CODE: thread and semaphore finishing actions
  fprintf (infF, "Swap Manager thread has terminated %d\n", ret);
}

