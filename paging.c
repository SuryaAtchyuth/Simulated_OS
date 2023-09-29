#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include "simos.h"

#define noFreeFrame -1
#define pageInit -2

typedef struct {
	int pid;
	int page;
	int age;
	bool dirty;
	bool free;
}metaData;

typedef struct{
	int fno;
	struct freeList *next;
}freeList;

metaData **mframe;
freeList *fhead, *ftail;
int **pageTable;


void init_process_pagetable (int pid){
	int page, nop=PCB[pid]->NOP;
    pageTable[pid] = (int*)calloc(nop,sizeof(int));
    
    for (page=0;page<nop;page++){
        *(pageTable[pid]+page)=pageInit;
    }
    PCB[pid]->PTptr = pageTable[pid];
}

void update_process_pagetable (int pid, int page, int frame){
	PCB[pid] -> PTptr[page] = frame;
}

void initialize_mframe_manager (){
	int mf;
	mframe    = (metaData**)calloc(numFrames,sizeof(metaData*));
	pageTable = (int**)calloc(maxProcess,sizeof(int*)); 
	fhead     = (freeList*)calloc(1,sizeof(freeList));
	
	for(mf=0;mf<numFrames;mf++){
		mframe[mf]= (metaData*)calloc(1,sizeof(metaData)); 
		mframe[mf]->pid=-1;
		mframe[mf]->page=-1;
		mframe[mf]->dirty=0;
		mframe[mf]->free=1;
	}
 
    fhead = NULL;
    for(mf=2; mf<numFrames; mf++){
       addto_free_frame(mf);
       initialize_agescan();
    }
}


void addto_free_frame(int fno){
	freeList *link = (freeList *)malloc(sizeof(freeList));
	link -> fno = fno;
	link -> next = NULL;
	if(fhead == NULL){
		fhead = link;
	}
	else{
		freeList *curr = fhead;
		while(curr ->next != NULL){
			curr = curr -> next;
	    }
		curr -> next = link;
	}
}

int get_free_frame(){
	int freeFrame;
	if(fhead == NULL){
		return -1;
	}
	freeList *tempLink = fhead;
	fhead = fhead -> next;
	freeFrame = tempLink -> fno;
	return freeFrame;
}

void initialize_agescan (){
	int frame;
	for(frame=2;frame<numFrames;frame++){
		mframe[frame]->age=0;
	} 

	add_timer(agescanPeriod, CPU.Pid, actAgeInterrupt, 1 ) ;     	
}

void memory_agescan (){
	int mf; 
	if(memDebug){
		fprintf(bugF,"\tScanning Memory Frame Age\n");  
	}  
	for(mf=2;mf<numFrames;mf++){  
		if(memDebug){
			fprintf(bugF,"Frame:%d Age:%x\n",mf, mframe[mf]->age); 
		}    
		mframe[mf]->age>>=1;
	}
}

int calculate_memory_address (unsigned offset, int rwflag) 
{
	int frame,PMA;
	frame = *((PCB[CPU.Pid]->PTptr)+(offset/pageSize));
	PMA=frame*pageSize + offset % pageSize ;  

	if(rwflag == 1 && ( PCB[CPU.Pid]->progLen < offset)){   	
		return mError;
	}
	if(frame == -2){                              
		set_interrupt(pFaultException);
		PCB[CPU.Pid]->numPF+=1;
		return mPFault;                           
	}
	else{
		if(rwflag == 2){
			mframe[frame]->dirty=1;
        }	
	}
	set_Left_AgeBit(frame);
	return PMA;                                    	
}

void set_Left_AgeBit(int frameNo){ 
	if(memDebug){
			fprintf(bugF,"\tIn set_Left_AgeBit\n");
			fprintf(bugF,"Frame:%d Before Updation:%x\n",frameNo,mframe[frameNo] -> age);
		}
	mframe[frameNo] -> age = ((mframe[frameNo] -> age) | (1<<7)) ; 
	if(memDebug){
		fprintf(bugF,"Frame:%d After Updation:%x\n",frameNo,mframe[frameNo] -> age);
	}
}

void free_process_memory (int pid){
    int mf;
    for(mf=2;mf<numFrames;mf++){
		if(mframe[mf]->pid==pid){
         	mframe[mf]->pid=-1;
         	mframe[mf]->page=-1;
         	mframe[mf]->age=0;
          	mframe[mf]->dirty=0;
          	mframe[mf]->free=0;
          	addto_free_frame(mf);
		}
	}
}

void swap_in_page (int pidin, int pagein, int finishact){
	unsigned *Buffer;  
	int luf, ff, mf, minage, minDirty; //luf = least used frame, ff=free frame, mf = mem frame
	bool ageZero = 0; //age zero flag to check any frames with age 0 to swap out if no free frames
	ff = get_free_frame();
	if(ff == noFreeFrame){
		for(mf=2;mf<numFrames;mf++){ //checking age 0 frames
			if(memDebug){
				fprintf(bugF," Before freeing memframe: %d of age: %x\n", mf, mframe[mf]->age);
		}
			if(mframe[mf]->age == 0){
				ageZero = 1;
				if(mframe[mf]-> dirty == 1){
					Buffer = Memory+(mf*pageSize); 
					insert_swapQ(mframe[mf]->pid, mframe[mf]->page, Buffer , actWrite, freeBuf);
				}
				if(memDebug){
					fprintf(bugF," Freeing Frame %d of Age %d\n",mf, mframe[mf]->age);
				}
				PCB[mframe[mf]->pid] -> PTptr[mframe[mf]->page] = pageInit; //updating page table for freed frame with age = 0
				mframe[mf]->pid = -1; //updating frame metadata for freed frame with age = 0
				mframe[mf]->page = -1;
				mframe[mf] -> age = 0;
				mframe[mf] -> dirty = 0;
				mframe[mf] -> free = 1; 
				addto_free_frame(mf);
				
			}
		}
		if(ageZero == 0){ //if not found any frame with age equals zero
			minage = mframe[2] -> age; minDirty = mframe[2] -> dirty;
			for(mf=2;mf<numFrames;mf++){
				if(minage > mframe[mf]->age){
					minage = mframe[mf]->age;
					minDirty = mframe[mf]->dirty;
					luf = mf;
				}
				if(minage == mframe[mf]->age && mframe[mf]->dirty < minDirty){
					luf = mf;
					minDirty = mframe[mf]->dirty;
				}
			}
			if(minDirty == 1){
				Buffer = Memory+(luf*(pageSize-1));
				insert_swapQ(mframe[luf]->pid, mframe[luf]->page, Buffer , actWrite, freeBuf);		
			}
			if(memDebug){
				fprintf(bugF,"Freeing memframe: %d of age: %d\n", mf, mframe[mf]->age);
			}
			PCB[mframe[luf]->pid] -> PTptr[mframe[luf]->page] = pageInit; //updating page table for freed frame with not age 0
			mframe[luf]->pid = -1; //updating frame metadata for freed frame with not age  0
			mframe[luf]->page = -1;
			mframe[luf] -> age = 0;
			mframe[luf] -> dirty = 0;
			mframe[luf] -> free = 1; 
			addto_free_frame(luf);
		}
		ff = get_free_frame();
	}
	Buffer = Memory+(ff * pageSize);

    insert_swapQ(pidin, pagein, Buffer , actRead, finishact);

	PCB[pidin] -> PTptr[pagein] = ff; //updating page table
	mframe[ff]->pid = pidin; // updating frame info
	mframe[ff] -> page = pagein;
	mframe[ff] -> age = 1<<7;
	mframe[ff] -> dirty = 0;
	mframe[ff] -> free = 0;
}

void page_fault_handler (){  
	int pfault;
    pfault = (CPU.PC)/pageSize;
	if(PCB[CPU.Pid] -> PTptr[pfault] != -2){
		pfault = (CPU.IRoperand/pageSize);
    }
    	fprintf(infF,"Handiling Page Fault : PID=%d,Page=%d\n",CPU.Pid,pfault);
 	swap_in_page(CPU.Pid,pfault, toReady);
}

void update_frame_info (int findex, int pid, int page){
	mframe[findex]->pid = pid; 
	mframe[findex] ->page = page;
}

void dump_free_list (FILE *outf){
	int frame;
	fprintf(outf,"\n********** Memory Free Frame Dump **********\n");
	for(frame=0;frame<numFrames-1;frame++){
		if(mframe[frame]->free==0){
			fprintf(outf," %d,",frame);
		}
		fprintf(outf," %d.",frame);
	}
}

void dump_process_pagetable (FILE *outf, int pid){
	int page,fo;
    fprintf(outf,"\n\t\t Page Table of PID:%d\n");
	for(page=0;page<PCB[pid]->NOP ;page++){
		fprintf(outf,"Page->Frame:%d -> %d\n",page,PCB[pid]->PTptr[page]);
	}
	fprintf(outf,"__________________________________________________________________________________________\n\n");
}

void dump_process_memory (FILE *outf, int pid){
	int page,frame,offset,fo,limit,loc=0;
	unsigned *buf;
	fprintf(outf,"\t########## Memory Dump for Process %d ##########\n",pid);
	for(page=0;page<PCB[pid]->NOP;page++){
        frame=PCB[pid]->PTptr[page];
		if((PCB[pid]->PTptr[page]==-2) || (PCB[pid]->PTptr[page]==-1)){
			fprintf(outf,"\t***P/F = %d/%d :: Content of process %d swap page %d:\n",page,frame,pid,page);
			read_swap_page (pid, page, buf);
			fprintf(outf,"\t");
			for(fo=0;fo<pageSize;fo++){
 				fprintf(outf,"%d.[%x] ",loc++,buf[fo]);
 		    }
 		    fprintf(outf,"\n");
		}
		else{
			fprintf(outf,"\t***P/F:%d,%d :: pid/page/age=%d,%d,%x, dir/free=%d/%d\n",page,frame,mframe[frame]->pid,
				         mframe[frame]->page,mframe[frame]->age,mframe[frame]->dirty,mframe[frame]->free);
			limit=frame*pageSize+(pageSize-1);
			fprintf(outf,"\t");
			for(fo=frame*pageSize;fo<=limit;fo++){
 				fprintf(outf,"%d.[%x] ",loc++,Memory[fo]);
 		    }
 		    fprintf(outf,"\n");
 		}
 		fprintf(outf,"\n");
	}
	dump_process_pagetable(outf,pid);
}

void dump_memory (FILE *outf){
	int frame, offset=0, fo;
	fprintf(outf,"\t********** Full Memory Dump **********\n");
 	for(frame=0; frame<numFrames; frame++) {
 		fprintf(outf,"Frame %d :[%d,%d]:",frame,offset,offset+pageSize-1);
 		for(fo=0;fo<pageSize;fo++){
 			fprintf(outf," %x",Memory[offset++]);
 		}
 		fprintf(outf,"\n\n");
  	}
}

void dump_memoryframe_info (FILE *outf){
	int frame;
	fprintf(outf,"\t\t   ~~~~~ Frame MetaData ~~~~~\n");
	for(frame=2;frame<numFrames;frame++){
		fprintf(outf,"\tFrame: %d :: pid/page/age=%d,%d,%x, dir/free=%d/%d \n",frame,mframe[frame]->pid,
		mframe[frame]->page,mframe[frame]->age,mframe[frame]->dirty,mframe[frame]->free);
	}
}
