#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define rdtsc(x)      __asm__ __volatile__("rdtsc \n\t" : "=A" (*(x)))

typedef char *addrs_t;
typedef void *any_t;


// The structure of the metadata attached with each block of memory
struct metaData{ //size of metaData is 24 [int = 4 bytes + padding = 8bytes * 2 = 16byes.  + 8 bytes for pointer]
    size_t size; //size of block described by it
    int free; //flag to show if free or not
    struct metaData *next; // points to the next block of memory
};
struct metaData *heapBasePointer; //define global variables


addrs_t baseptr; //define global variables - base pointer
addrs_t endptr; // define global variables - end of the area in mem
int sizeGlobal; // define global variables - size of originally allocated amount via C library malloc function
int VsizeGlobal;
int RTCounter = 0; // define global variables - RT counter
addrs_t* RT[32768] = {NULL}; // define global variables - redirection table of addresses
int globalRTsize = 32768;
long globalAllocatedSize = 0;
long VglobalAllocatedSize = 0;
long mallocReq = 0;
long VmallocReq = 0;
long VfreeReq = 0;
long failReq = 0;
long VfailReq = 0;
unsigned long start, finish;
long mallocClockCycleTotal = 0;
long freeClockCycleTotal = 0;
long VmallocClockCycleTotal = 0;
long VfreeClockCycleTotal = 0;


//To insert a new block, we split a free block.
//fitting_block: Pointer to metaData block that references free space
//size_t: size of new black
void split(struct metaData *fitting_block, size_t size) {
    int sizeofblock = fitting_block->size;
    //void* pointerToNewData = (void*)(fitting_block+24);
    void* pointerToNewData = (char *)fitting_block + 24 +size;
    fitting_block->size = size; //set the fitting block to the correct amount
    fitting_block->free=0; //flag it as not free
    if(sizeofblock-size-24 == 0){
        return;
    }else{
        struct metaData *new = pointerToNewData; //new metaData points to the new area in mem
        new->size = (sizeofblock - size - 24); //new area of memo only has free space size - size of data - size of struct (24)
        new->free=1; //flag for new block set to free
        new->next=fitting_block->next; //next for new block is set to the next of the fitting block
        //memcpy((char*)fitting_block+24+size,new, 24); //since new is not in the mem area, we need to copy the 24 bytes to the area in mem right after the fitting block
        fitting_block->next=pointerToNewData; //set the next of fitting block to this new block we copied
        return;
    }

}

addrs_t *VMalloc(size_t size){
    rdtsc(&start);
    VmallocReq++;
    if(size%8 !=0) // to align the size on 8 byte bound
    {
        do{
            size++;
        }while(size%8!=0);
// cealing function
    }
    struct metaData *current;
    struct metaData *prev; // initialing variables
    addrs_t result;
    RT;
    current = heapBasePointer; // set current to the base pointer of the heap, or the metaData freeHeap
    while(current != NULL) {
        if ((current->size) ==
            size && current->free==1) { //if this condition is meet, we have found a block of memory that exactly fits the 'size'
            current->free = 0; //set free flag to 0
            result = current; //result is the current block address
            RT[RTCounter] = (char*)result+24; //Set the RT[RTCounter] = the area in mem we've allocated
            //RTable[RTCounter] = RT[RTCounter];
            RTCounter++; // increment the RT counter
            rdtsc(&finish);
            VmallocClockCycleTotal+= finish - start;
            return &RT[RTCounter-1]; //return the block of memory
        } else if ((current->size) >
                   (size + 24) && current->free==1) { //if this condition is meet, we've found a block of memory that's smaller than 'size'
            current->free = 0;
            split(current, size); //we need to split the block that has too much space
            result = current; // return the current pointer to the size that has enough space
            current->size = size;

            if(RTCounter==globalRTsize)//IF RT IS FULL -> THEN YOU HAVE TO START TRAVERSING THE RT TABLE AND LOOK FOR NULL AND POINT TO THAT INSTED
            {
                for(int i =0; i < globalRTsize; i++)
                   if(RT[i]==NULL){
                       RT[i] = (char*)result+24;
                       rdtsc(&finish);
                       VmallocClockCycleTotal+= finish - start;
                       return &RT[i];
                }
            } else{
            RT[RTCounter] = (char*)result+24; //Set the RT[RTCounter] = the area in mem we've allocated
            RTCounter++;
            rdtsc(&finish);
            VmallocClockCycleTotal+= finish - start;
            return &RT[RTCounter-1]; //return array addr to block of mem.
            }
        }
        
        current=current->next;
    };
    //else not enough memory to fit anywhere :-(
    result=NULL;
    VfailReq++;
    //printf("Sorry. No sufficient memory to allocate\n");
    rdtsc(&finish);
    VmallocClockCycleTotal+= finish - start;
    return result;
}

void VMerge(int RTcounter){

    struct metaData *current = heapBasePointer;
    struct metaData *counter = heapBasePointer;
    char* RTPointer = *RT;

    //current = (char*)RT[RTcounter]-24;
    while(current->free==0){
    current = current->next; 
    }
    int amountToAdd = current->size+24; //keep track of the amount of mem we're freeing up
    struct metaData* traverse = current;
    int offset = 0;
    int k = 0;
    while(traverse!=NULL){ //traverse the heap, re allocating data
           	if (traverse->free==1 && traverse->next==NULL){ // if this is the last block of mem
        			traverse->size += amountToAdd;
         			memcpy((char*)traverse-amountToAdd, traverse, amountToAdd);
            }else if(traverse->free!=1 && traverse->next==NULL){ //if this is the last data, but it is not free [full heap]
           			 traverse->free=1; 
            }else if(traverse->next->free==1 && traverse->next->next==NULL){ //if there is only one block of mem
            		memcpy(traverse, traverse->next, amountToAdd);
            		traverse->size+=amountToAdd; 
            }else{
                int x = traverse->size + 24; //keep value to use at the end
                for (int i = k; i < (globalRTsize); i++){
                    if((char*)RT[i]-24 == traverse->next){
                        RT[i] = (char*)current + offset + 24; //re assign the RT value
                        k = i; 
                        break; 
                        }
                }
                memcpy((char*) current + offset, traverse->next, traverse->size + 24); //shift the block in mem
                traverse->next = (char*)traverse+traverse->size+24; 
                offset += x;
            }
            if(current->next!=NULL){
           	 	if((current->free) && (current->next->free)){ //if current is free & current.next is free
            	current->size+=(current->next->size)+24; //current.size = current size + size of metaData + size of current.next
            	current->next=current->next->next; //remove reference to irrelevant metadata now
            	}
        }
        traverse = traverse->next;
    }
         /*   struct metaData *helper = traverse;
            if(traverse->free==1 && traverse->next==NULL){ //if we've reached the last block of data; huge free area 
                    traverse->size += amountToAdd;
            }else if(traverse->next==NULL && traverse->free==0){ //if We've hit the last block in mem, the heap was filled to capacity
           			memcpy((char*) current + offset, traverse, traverse->size + 24); //shift the block in mem
                	traverse->size += amountToAdd-24;
                    traverse->free = 1; 
                    RT[globalRTsize-1] = (char*)traverse+24; 
            }else{
                int x = traverse->size + 24; //keep value to use at the end
                for (i; i < (globalRTsize); i++){
                    if((char*)RT[i]-24 == traverse->next){
                        RT[i] = (char *) current + 24 + offset; //re assign the RT value
                        memcpy((char*) current + offset, traverse->next, traverse->size + 24); //shift the block in mem
                        traverse->next= (char*)traverse+traverse->size+24; 
                        break; 
                        }
                }
                offset += x;
                if (traverse->next ==NULL && traverse->free == 1) {//check if this is the last block in mem
                    //memcpy((char*) helper + 24 + helper->size, helper->next, helper->next->size + 24);
                    //helper->next = (char *) helper + 24 + helper->size;
                    traverse->size += amountToAdd;
                }
            }
        traverse = traverse->next;
    }*/
    RT[RTcounter] = NULL;
}


void VFree (addrs_t *addr){
    rdtsc(&start);
    VfreeReq++;
    struct metaData* current = (char*)*addr-24;
    if(current->next==NULL){ //if current.next is null, means that this is the only block of mem
        current->free=1; //set current to free
        int counter = 0; 
        while(RT[counter] != *addr){//traverse the RT to find the block of mem we are trying to remove
            counter++;
            }
        RT[counter]= NULL; 
        rdtsc(&finish);
        VfreeClockCycleTotal+= finish - start;
        return;
    } else { //else multiple blocks of mem
        current->free=1; //set current to free
        int counter = 0;
        while(RT[counter] != *addr){//traverse the RT to find the block of mem we are trying to remove
            counter++;
        }
        VMerge(counter); // merge the rest of the data
        rdtsc(&finish);
        VfreeClockCycleTotal+= finish - start;
        return;
    }
    rdtsc(&finish);
    freeClockCycleTotal+= finish - start;
    return;
}


addrs_t VPut (any_t data, size_t size) {
    VglobalAllocatedSize+=size;
    /*Allocate size bytes from M2 using VMalloc().
     * Copy size bytes of data into Malloc'd memory.
     * You can assume data is a storage area outside M2.
     * Return pointer to redirection table for Malloc'd memory. */


    addrs_t *start  = VMalloc(size);//clear area in memory and create pointer
    if(start == NULL){ //catch all 
        return NULL;
    }
    memcpy(*start, data, size); // copying the data 
    // *((int*)result) = *((int*)data); //set result to be equal to the data 
    return start; //return start of the data
}
void VGet (any_t return_data, addrs_t *addr, size_t size) {
    VglobalAllocatedSize-=size;
    /*Copy size bytes from the memory area, M2, to data address.
     * The addr argument specifies a pointer to a redirection table entry.
     * As with VPut(), you can assume data is a storage area outside M2.
     * Finally, de-allocate size bytes of memory using VFree()
     * with addr pointing to a redirection table entry. */


    /*for (int i=0; i<size; i++) {
        int* buffer = (return_data + i);
        *buffer = addr+24+i;
    }*/

    struct metaData* current = (char*)*addr-24;
    memcpy(return_data, *addr, size);
    current->free=1;
    VFree(addr);

}

void VInit(size_t size) {

/* Use the system malloc() routine (or new in C++) ONLY to allocate size bytes
 * for the initial memory area, M1. baseptr is the starting address of M1. */

    VsizeGlobal = size;
    baseptr = (addrs_t)malloc(size); //Setting baseptr to point to the start of the heap + size of the RT
    heapBasePointer = baseptr; //create new MetaData pointing to the empty heap
    heapBasePointer->size = size-24; //size of heap = size of heap - size of metadata
    heapBasePointer->free = 1; //free flag set to true
    heapBasePointer->next = NULL; //no other data in the heap, next -> null

}

void VheapChecker(){
    struct metaData* current = heapBasePointer;
    long numOfAllocBlocks = 0;
    long numOfFreeBlocks = 0;
    long numOfTotalIncPadding = 0;
    long numOfRawTotalFree = 0;

    while(current !=NULL){
        if(current->free == 0){
            numOfAllocBlocks++;
            numOfTotalIncPadding+=current->size;
        }
        if(current->free ==1){
            numOfFreeBlocks++;
            numOfRawTotalFree+= current->size;
        }

        current = current->next;

    }
    printf("<<Part 2 for Region M2>>\n");
    printf("Number of allocated blocks : %lu\n", numOfAllocBlocks);
    printf("Number of free blocks : %lu\n", numOfFreeBlocks);
    printf("Raw total number of bytes allocated : %lu\n", VglobalAllocatedSize);
    printf("Padded total number of bytes allocated : %lu\n", numOfTotalIncPadding);
    printf("Raw total number of bytes free : %lu\n", numOfRawTotalFree);
    long numAlignedTotalFree = VsizeGlobal - (numOfAllocBlocks+1*24)-numOfTotalIncPadding;
    printf("Aligned total number of bytes free : %lu\n", numAlignedTotalFree);
    printf("Total number of Malloc requests : %lu\n", VmallocReq);
    printf("Total number of Free requests: %lu\n", VfreeReq);
    printf("Total number of request failures:  %lu\n", VfailReq);
    printf("Average clock cycles for a Malloc request:  %lu\n", (mallocClockCycleTotal/VmallocReq));
    printf("Average clock cycles for a Free request:  %lu\n", (freeClockCycleTotal/VfreeReq));
    printf("Total clock cycles for all requests:  %lu\n", (VmallocClockCycleTotal+VfreeClockCycleTotal));
};


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char *addrs_t;
typedef void *any_t;


// The structure of the metadata attached with each block of memory
struct metaData{ //size of metaData is 24[size_t = 8 bytes  + 4 bytes + padding  + 8 bytes for pointer]
    size_t size; //size of block described by it
    int free; //flag to show if free or not
    struct metaData *next; // points to the next block of memory
};
struct metaData *heapBasePointer; //define global variables


addrs_t baseptr; //define global variables - base pointer
addrs_t endptr; // define global variables - end of the area in mem
int sizeGlobal; // define global variables - size of originally allocated amount via C library malloc function
int VsizeGlobal;
int RTCounter = 0; // define global variables - RT counter
addrs_t* RT[32768] = {NULL}; // define global variables - redirection table of addresses
int globalRTsize = 32768;// define global variables - RT size 
long VglobalAllocatedSize = 0; // define global variables - heap checker variable 
long VmallocReq = 0; // define global variables - heap checker variable 
long VfreeReq = 0; // define global variables - heap checker variable 
long VfailReq = 0; // define global variables - heap checker variable 
unsigned long start, finish; // define global variables - heap checker variable 
long VmallocClockCycleTotal = 0; // define global variables - heap checker variable 
long VfreeClockCycleTotal = 0; // define global variables - heap checker variable z


//To insert a new block, we split a free block.
//fitting_block: Pointer to metaData block that references free space
//size_t: size of new black
void split(struct metaData *fitting_block, size_t size) {
    int sizeofblock = fitting_block->size;
    //void* pointerToNewData = (void*)(fitting_block+24);
    void* pointerToNewData = (char *)fitting_block + 24+size;
    fitting_block->size = size; //set the fitting block to the correct amount
    fitting_block->free=0; //flag it as not free
    if(sizeofblock-size-24== 0){
        return;
    }else{
        struct metaData *new = pointerToNewData; //new metaData points to the new area in mem
        new->size = (sizeofblock - size - 24); //new area of memo only has free space size - size of data - size of struct (24)
        new->free=1; //flag for new block set to free
        new->next=fitting_block->next; //next for new block is set to the next of the fitting block
        //memcpy((char*)fitting_block+24+size,new, 24); //since new is not in the mem area, we need to copy the 24bytes to the area in mem right after the fitting block
        fitting_block->next=pointerToNewData; //set the next of fitting block to this new block we copied
        return;
    }

}

addrs_t *VMalloc(size_t size){
    rdtsc(&start);
    VmallocReq++;
    if(size%8 !=0) // to align the size on 8 byte bound
    {
        do{
            size++;
        }while(size%8!=0);
    }
    struct metaData *current;
    struct metaData *prev; // initialing variables
    addrs_t result;
    current = heapBasePointer; // set current to the base pointer of the heap, or the metaData freeHeap
    while(current != NULL) {
        if ((current->size) ==
            size && current->free==1) { //if this condition is meet, we have found a block of memory that exactly fits the 'size'
            current->free = 0; //set free flag to 0
            result = current; //result is the current block address
            RT[RTCounter] = (char*)result+24; //Set the RT[RTCounter] = the area in mem we've allocated
            //RTable[RTCounter] = RT[RTCounter];
            RTCounter++; // increment the RT counter
            rdtsc(&finish);
            VmallocClockCycleTotal+= finish - start;
            return &RT[RTCounter-1]; //return the block of memory
        } else if ((current->size) >
                   (size + 24) && current->free==1) { //if this condition is meet, we've found a block of memory that's smaller than 'size'
            current->free = 0;
            split(current, size); //we need to split the block that has too much space
            result = current; // return the current pointer to the size that has enough space
            current->size = size; 

            if(RTCounter==globalRTsize)//IF RT IS FULL -> THEN YOU HAVE TO START TRAVERSING THE RT TABLE AND LOOK FOR NULL AND POINT TO THAT INSTED
            {
                for(int i =0; i < globalRTsize; i++) //traverse RT
                   if(RT[i]==NULL){ //if we find a free block of mem 
                       RT[i] = (char*)result+24; //redefine RT[i]
                       rdtsc(&finish);
                       VmallocClockCycleTotal+= finish - start;
                       return &RT[i]; //return &RT
                }
            } else{
            RT[RTCounter] = (char*)result+24; //Set the RT[RTCounter] = the area in mem we've allocated
            RTCounter++; //increment global counter 
            rdtsc(&finish);
            VmallocClockCycleTotal+= finish - start;
            return &RT[RTCounter-1]; //return array addr to block of mem.
            }
        }
        
        current=current->next; //increment current
    };
    //else not enough memory to fit anywhere :-(
    result=NULL; //return null 
    VfailReq++;
    rdtsc(&finish);
    VmallocClockCycleTotal+= finish - start;
    return result;
}

void VMerge(int RTcounter){

    struct metaData *current = heapBasePointer;
    struct metaData *counter = heapBasePointer;
    char* RTPointer = *RT;
    while(current->free==0){ //traverse to find the block of mem we're trying to free
    current = current->next; 
    }
    int amountToAdd = current->size+24; //keep track of the amount of mem we're freeing up
    struct metaData* traverse = current;
    int offset = 0;
    int k = 0;
    while(traverse!=NULL){ //traverse the heap, re allocating data
           	if (traverse->free==1 && traverse->next==NULL){ // if this is the last block of mem
        			traverse->size += amountToAdd; //increment size by amount to add 
         			memcpy((char*)traverse-amountToAdd, traverse, amountToAdd); // copy the relevent info 
            }else if(traverse->free!=1 && traverse->next==NULL){ //if this is the last data, but it is not free [full heap]
           			 traverse->free=1;  //set the metadata to free 
            }else if(traverse->next->free==1 && traverse->next->next==NULL){ //if there is only one block of mem
            		memcpy(traverse, traverse->next, amountToAdd); //copy the next pointer 
            		traverse->size+=amountToAdd;  //increment the size of traverse 
            }else{
                int x = traverse->size + 24; //keep value to use at the end for offset 
                for (int i = k; i < (globalRTsize); i++){ //traverse the RT to re align the value we're shifting 
                    if((char*)RT[i]-24== traverse->next){ //if the RT value = traverse->next 
                        RT[i] = (char*)current + offset + 24; //re assign the RT value
                        k = i; // keep i so we dont have to traverse the ENTIRE RT each time 
                        break; //once found break out of the function 
                        }
                }
                memcpy((char*) current + offset, traverse->next, traverse->size + 24); //shift the block in mem
                traverse->next = (char*)traverse+traverse->size+24;  //set the traverse->next to be the address traverse+traverse->size+24 
                offset += x; //increment the offset 
            }
            if(current->next!=NULL){
           	 	if((current->free) && (current->next->free)){ //if current is free & current.next is free
            	current->size+=(current->next->size)+24; //current.size = current size + size of metaData + size of current.next
            	current->next=current->next->next; //remove reference to irrelevant metadata now
            	}
        }
        traverse = traverse->next; //increment traverse 
    }
         
    RT[RTcounter] = NULL; //set the RT counter to be NULL
}

//function to free data 
void VFree (addrs_t *addr){
    rdtsc(&start);
    VfreeReq++;
    struct metaData* current = (char*)*addr-24; //set a current metadata which is equal to addr 
    if(current->next==NULL){ //if current.next is null, means that this is the only block of mem
        current->free=1; //set current to free
        int counter = 0;  // set the counter to 0 
        while(RT[counter] != *addr){//traverse the RT to find the block of mem we are trying to remove
            counter++;
            }
        RT[counter]= NULL;  //set the RT counter to be null
        rdtsc(&finish);
        VfreeClockCycleTotal+= finish - start;
        return;
    } else { //else multiple blocks of mem
        current->free=1; //set current to free
        int counter = 0; // set the counter to 0 
        while(RT[counter] != *addr){//traverse the RT to find the block of mem we are trying to remove
            counter++;
        }
        VMerge(counter); // merge the rest of the data
        rdtsc(&finish);
        VfreeClockCycleTotal+= finish - start;
        return;
    }
    rdtsc(&finish);
    VfreeClockCycleTotal+= finish - start;
    return; 
}


addrs_t VPut (any_t data, size_t size) {
    VglobalAllocatedSize+=size;
    /*Allocate size bytes from M2 using VMalloc().
     * Copy size bytes of data into Malloc'd memory.
     * You can assume data is a storage area outside M2.
     * Return pointer to redirection table for Malloc'd memory. */
    addrs_t *start  = VMalloc(size);//clear area in memory and create pointer
    if(start == NULL){ //catch all 
        return NULL;
    }
    memcpy(*start, data, size); // copying the data  
    return start; //return start of the data
}
void VGet (any_t return_data, addrs_t *addr, size_t size) {
    VglobalAllocatedSize-=size;
    /*Copy size bytes from the memory area, M2, to data address.
     * The addr argument specifies a pointer to a redirection table entry.
     * As with VPut(), you can assume data is a storage area outside M2.
     * Finally, de-allocate size bytes of memory using VFree()
     * with addr pointing to a redirection table entry. */

    struct metaData* current = (char*)*addr-24;
    memcpy(return_data, *addr, size);
    current->free=1;
    VFree(addr);
}

void VInit(size_t size) {
	/* Use the system malloc() routine (or new in C++) ONLY to allocate size bytes
 	* for the initial memory area, M1. baseptr is the starting address of M1. */

    VsizeGlobal = size; //set the global size to the data allocated 
    baseptr = (addrs_t)malloc(size); //Setting baseptr to point to the start of the heap + size of the RT
    heapBasePointer = baseptr; //create new MetaData pointing to the empty heap
    heapBasePointer->size = size-24; //size of heap = size of heap - size of metadata
    heapBasePointer->free = 1; //free flag set to true
    heapBasePointer->next = NULL; //no other data in the heap, next -> null

}

void VheapChecker(){
    struct metaData* current = heapBasePointer; // define variables 
    long numOfAllocBlocks = 0; // define variables 
    long numOfFreeBlocks = 0; // define variables 
    long numOfTotalIncPadding = 0; // define variables 
    long numOfRawTotalFree = 0; // define variables 

    while(current !=NULL){ //traverse the heap to get relevent data 
        if(current->free == 0){
            numOfAllocBlocks++;
            numOfTotalIncPadding+=current->size;
        }
        if(current->free ==1){
            numOfFreeBlocks++;
            numOfRawTotalFree+= current->size;
        }

        current = current->next;

    }
    
    //print results 
    printf("<<Part 2 for Region M2>>\n");
    printf("Number of allocated blocks : %lu\n", numOfAllocBlocks);
    printf("Number of free blocks : %lu\n", numOfFreeBlocks);
    printf("Raw total number of bytes allocated : %lu\n", VglobalAllocatedSize);
    printf("Padded total number of bytes allocated : %lu\n", numOfTotalIncPadding);
    printf("Raw total number of bytes free : %lu\n", numOfRawTotalFree);
    long numAlignedTotalFree = VsizeGlobal - (numOfAllocBlocks+1*24)-numOfTotalIncPadding; //calculate numAlignedTotalFree = size - (number of blocks allocated + 1 [free block])* 24 (size of metadata) -padding
    printf("Aligned total number of bytes free : %lu\n", numAlignedTotalFree);
    printf("Total number of Malloc requests : %lu\n", VmallocReq);
    printf("Total number of Free requests: %lu\n", VfreeReq);
    printf("Total number of request failures:  %lu\n", VfailReq);
    printf("Average clock cycles for a Malloc request:  %lu\n", (VmallocClockCycleTotal/VmallocReq));
    printf("Average clock cycles for a Free request:  %lu\n", (VfreeClockCycleTotal/VfreeReq));
    printf("Total clock cycles for all requests:  %lu\n", (VmallocClockCycleTotal+VfreeClockCycleTotal));
};