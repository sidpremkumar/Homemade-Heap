#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define rdtsc(x)      __asm__ __volatile__("rdtsc \n\t" : "=A" (*(x)))
typedef char *addrs_t;
typedef void *any_t;
#define RTSIZE = 10;

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
char *RT[32768]; // define global variables - redirection table of addresses
int globalRTsize = 32768;
long globalAllocatedSize = 0;
long VglobalAllocatedSize = 0;
long mallocReq = 0;
long VmallocReq = 0;
long freeReq = 0;
long VfreeReq = 0;
long failReq = 0;
long VfailReq = 0;
unsigned long start, finish;
long mallocClockCycleTotal = 0;
long freeClockCycleTotal = 0;
long VmallocClockCycleTotal = 0;
long VfreeClockCycleTotal = 0;


//function to check if metaData.next is null




//this functions merges blocks in order to free up space
void merge(struct metaData *block){
    struct metaData *current;
    current = block; //current is the start of the heap
    int sizetoCopy = 0;

    while(current->next !=NULL && current!=NULL){ //while current.next isnt null
        if((current->free) && (current->next->free)){ //if current is free & current.next is free
            current->size+=(current->next->size)+24; //current.size = current size + size of metaData + size of current.next
            current->next=current->next->next; //remove reference to irrelevant metadata now
        }

        current=current->next;
        if(current == NULL){
            break;
        }
    }
}


void Free (addrs_t addr) {
    rdtsc(&start);
freeReq++;
/*This frees the previously allocated size bytes starting from address
 * addr in the memory area, M1. You can assume the size argument is
 * stored in a data structure after the Malloc() routine has been called,
 * just as with the UNIX free() command.*/

    if((baseptr<=addr)&&(addr<=baseptr+sizeGlobal)) { //if the address is valid
        struct metaData* current = (char*)addr-24; // DOUBLE CHECK IF IT SHOULD BE -24 OR JUST -0
        if(current->next==NULL){//if current.next is null, means that this is the only block of mem
            current->free=1; //set current to free
            //printf("Freed memory\n");
            rdtsc(&finish);
            freeClockCycleTotal+= finish - start;
            return;
        } else { //else multiple blocks of mem
            /*int sizeToCopy = current->size+24; //size of data to add to free'd area
            struct metaData* counter = current; // counter
            while(counter->free == 0){ //traverse until we've found the freed area in memory
                counter = counter->next;
            }
            counter->size+= sizeToCopy; //add the size of this free to free'd area */
            current->free=1; //set current to free
            merge(current); // merge the rest of the data
            rdtsc(&finish);
            freeClockCycleTotal+= finish - start;
            return;
            //printf("Freed memory\n");
        }

    } else ("Pointer not Valid\n");
    rdtsc(&finish);
    freeClockCycleTotal = finish - start;
    return;

}


void Get (any_t return_data, addrs_t addr, size_t size) {
    globalAllocatedSize-=size;
/*Copy size bytes from addr in the memory area, M1, to
 * data address. As with Put(), you can assume data is
 * a storage area outside M1. De-allocate size bytes of
 * memory starting from addr using Free(). */
    if((baseptr<=addr)&&(addr<=baseptr+sizeGlobal)) { //if the address is valid
        //struct metaData *current = (addr - 24); //current metaData for chunk of memory
        /*for (int i=0; i<size; i++){ //clear out return data area
            int* buffer = (void*)(return_data+i);
            *buffer=0;
        }*/
        memcpy(return_data, addr, size);
        Free(addr);
    } else ("Pointer not Valid\n");

}




void Init(size_t size) {
    //printf("Allocating area of %lu bytes in memory\n", size);
/* Use the system malloc() routine (or new in C++) ONLY to allocate size bytes
 * for the initial memory area, M1. baseptr is the starting address of M1. */

    sizeGlobal = size;
    baseptr = (addrs_t)malloc(size); //Setting baseptr to point to the start of the heap
    endptr = baseptr+size;
    heapBasePointer = baseptr; //create new MetaData pointing to the empty heap
    heapBasePointer->size = size-24; //size of heap = size of heap - size of metadata
    heapBasePointer->free = 1; //free flag set to true
    heapBasePointer->next = NULL; //no other data in the heap, next -> null

    // printf("The starting memory address is: %u\n", baseptr);

}



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

addrs_t Malloc (size_t size) {
    rdtsc(&start);
mallocReq++;
    /* Implement your own memory allocation routine here.
     * This should allocate the first contiguous size bytes available in M1.
     * Since some machine architectures are 64-bit, it should be safe to
     * allocate space starting at the first address divisible by 8. Hence
     * align all addresses on 8-byte boundaries!
     * If enough space exists, allocate space and return the base address of the memory.
     * If insufficient space exists, return NULL.*/

    // WHEN SOMETHING IS ALLOCATED TO THE HEAP, IT ADDS A POINTER TO THE NEXT ALLOCATED SEGMENT OF THE HEAP
    // SEE PICTURES
    // IF P(1) -> 0x0
    // IF P(2) -> P(1)+0x1 WHICH IS A POINTER OT P(2)
    //USE LINKED LIST ITS EASIER
    merge(heapBasePointer);
    if(size%8 !=0) // to align the size on 8 byte bound
    {
        do{
            size++;
        }while(size%8!=0);
    }
    struct metaData *current;
    void *result;
    current = heapBasePointer; // set current to the base pointer of the heap, or the metaData freeHeap
    while(current != NULL) {
        if ((current->size) ==
            size && current->free==1) { //if this condition is meet, we have found a block of memory that exactly fits the 'size'
            current->free = 0; //set free flag to 0
            result = (char*)current+24; //result is the current block address
            // printf("Exact fitting block allocated\n");
            rdtsc(&finish);
            mallocClockCycleTotal+= finish - start;
            return result; //return the block of memory
        } else if ((current->size) >=
                   (size + 24)&& current->free==1) { //if this condition is meet, we've found a block of memory that's smaller than 'size'
            split(current,size); //we need to split the block that has too much space
            result = (char*)current+24; // return the current pointer to the size that has enough space
            //printf("Fitting block allocated with a split\n");
            rdtsc(&finish);
            mallocClockCycleTotal+= finish - start;
            return result;
        }
        current=current->next;
    };
    //else not enough memory to fit anywhere :-(
    result=NULL;
    failReq++;
   // printf("Sorry. No sufficient memory to allocate\n");
    rdtsc(&finish);
    mallocClockCycleTotal = finish - start;
    return result;


}


addrs_t Put (any_t data, size_t size) {
    globalAllocatedSize+=size;
/* Allocate size bytes from M1 using Malloc().
 * Copy size bytes of data into Malloc'd memory.
 * You can assume data is a storage area outside M1.
 * Return starting address of data in Malloc'd memory. */


    void* start  = Malloc(size);//clear area in memory and create pointer
    if(start == NULL){
        return NULL;
    }
    void* result = (char*) start; // start is already offset
   /* for(int i=0; i<size; i++){ //clear the area in M1 first using a buffer
        int* buffer = (start + 24+ i);
        *buffer = 0;
    }*/
    memcpy(result, data, size);
    // *((int*)result) = *((int*)data); //set result to be equal to the data

    return result; //return start of the data
}



void heapChecker(){
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
    printf("<<Part 1 for Region M1>>\n");
    printf("Number of allocated blocks : %lu\n", numOfAllocBlocks);
    printf("Number of free blocks : %lu\n", numOfFreeBlocks);
    printf("Raw total number of bytes allocated : %lu\n", globalAllocatedSize);
    printf("Padded total number of bytes allocated : %lu\n", numOfTotalIncPadding);
    printf("Raw total number of bytes free : %lu\n", numOfRawTotalFree);
    long numAlignedTotalFree = sizeGlobal - (numOfAllocBlocks+1*24)-numOfTotalIncPadding;
    printf("Aligned total number of bytes free : %lu\n", numAlignedTotalFree);
    printf("Total number of Malloc requests : %lu\n", mallocReq);
    printf("Total number of Free requests: %lu\n", freeReq);
    printf("Total number of request failures:  %lu\n", failReq);
    printf("Average clock cycles for a Malloc request:  %lu\n", (mallocClockCycleTotal/mallocReq));
    printf("Average clock cycles for a Free request:  %lu\n", (freeClockCycleTotal/freeReq));
    printf("Total clock cycles for all requests:  %lu\n", (mallocClockCycleTotal+freeClockCycleTotal));
};