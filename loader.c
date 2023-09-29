#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "simos.h"

#define progError -1
#define progNormal 1
#define opcodeShift 24 


int load_process(int pid, char *fname){
	char line[50];
	int progLen,instrLen,dataLen;
	FILE *f;
	f=fopen(fname,"r");
	if(f== NULL){
		fprintf(infF,"Submission Error: Incorrect program file: %s!\n",fname);
		return progError;	
	}
	else{		
		if(fscanf(f,"%d%d%d",&progLen,&instrLen,&dataLen)==3){
			if(progLen != (instrLen + dataLen) ){  
				fprintf(infF,"Program Error: Incorrect data\n");
				fclose(f);
				return progError;		
			}
			if(init_program(fname,instrLen) == progError){
				fclose(f);
				return progError;
			}
	        else{
	        	PCB[pid]->progLen=progLen;
	        	if(load_process_to_swap(pid,fname,progLen,instrLen,dataLen)==progNormal){
	                init_process_pagetable(pid);
					load_pages_to_memory(pid,progLen);
			     }
	        }
	    	
	    }
	    else{
	    	fprintf(infF,"Program Error: Incorrect data\n");
	    	fclose(f);
	    	return progError;
	    }
    } 
    fclose(f);          
	return progNormal;
}

int init_program(FILE *fname,int instrLen){
	int limit=instrLen,c1,c2;;
	FILE *fpc=fopen(fname,"r");
	fscanf(fpc, "%*[^\n]\n");
	while(limit--){
		if (fscanf(fpc,"%d%d",&c1,&c2)!=2){
			fprintf(infF,"Program Error: Incorrect data\n");
			fclose(fpc);
			return progError;
		}
	}
    return progNormal;
}

int load_process_to_swap(int pid,FILE *fname,int progLen,int instrLen,int dataLen){
	int opc,opr,limit,progLoc=0,progAL,pageNum; 
    mType *Program,*Buffer;

      progAL=(progLen)*pageSize;

    if( (progLen%pageSize)>0 ){ 
    	progAL= ((progLen/pageSize)+1)*pageSize;
    }
    else{
    	progAL=progLen;
    }
    Program=(mType*)calloc(progAL,dataSize);
    Buffer=(mType*)calloc(pageSize,dataSize);
	
	FILE *f=fopen(fname,"r");
	fscanf(f, "%*[^\n]\n");
	limit=instrLen;
	while(limit--){
	
		fscanf(f,"%d",&opc);
		opc <<= opcodeShift;
		fscanf(f,"%d",&opr);
		Program[progLoc++].mInstr=opc+opr;  
	}

	limit=dataLen;
	while(limit--){
		fscanf(f,"%f",&Program[progLoc++].mData);
	}
	

    if((progLen%pageSize)>0){ 
    	limit=(progLen/pageSize)+1;
    }
    else{
    	limit=progLen/pageSize;
    }
    pageNum=0;
    while(pageNum<limit){
		for(int i=0;i<pageSize;i++){
				Buffer[i]=Program[(pageNum*pageSize)+i];
				write_swap_page (pid,pageNum, Buffer);
	    }
	    ++pageNum;
	}	
	PCB[pid]->NOP=pageNum;
	fclose(f); 
	free(Program);
	free(Buffer);
	return progNormal;  
}


void load_pages_to_memory(int pid,int progLen){
     
    int limit,page=0;
    if( (progLen%pageSize)>0 ){ 
    	limit=(progLen/pageSize)+1;
    }
    else{
    	limit=progLen/pageSize;
    }

    if( limit>loadPpages ){
    	limit=loadPpages;
    }

	while(page<limit){
		swap_in_page(pid,page++,Nothing);	
    }
    insert_endIO_list(pid);
    set_interrupt(endIOinterrupt);
}
