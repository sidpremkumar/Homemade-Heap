#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char *addrs_t;
typedef void *any_t;
#define RTSIZE = 10;


#define TESTSUITE_STR         "Heap"
#define INIT(msize)           Init(msize)
#define MALLOC(msize)         Malloc(msize)
#define FREE(addr,size)       Free(addr)
#define PUT(data,size)        Put(data,size)
#define GET(rt,addr,size)     Get(rt,addr,size)
#define ADDRS                 addrs_t
#define LOCATION_OF(addr)     ((size_t)addr)
#define DATA_OF(addr)         (*(addr))

#define KBLU  "\x1B[34m"
#define KRED  "\x1B[31m"
#define KRESET "\x1B[0m"

#define ERROR_OUT_OF_MEM    0x1
#define ERROR_DATA_INCON    0x2
#define ERROR_ALIGMENT      0x4
#define ERROR_NOT_FF        0x8

#define HELLO 
#define ALIGN 8

#define rdtsc(x)      __asm__ __volatile__("rdtsc \n\t" : "=A" (*(x)))



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
int RTCounter = 0; // define global variables - RT counter
addrs_t RT[2000]; // define global variables - redirection table of addresses
int globalRTsize = 2000;

//function to check if metaData.next is null




//this functions merges blocks in order to free up space
void merge(){
    struct metaData *current;
    current = heapBasePointer; //current is the start of the heap
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

    /*
    struct metaData *counter = current;
    struct metaData *copier;
    while(counter->free !=1){ //finding the node that is free'd
        counter=counter->next;
    }
    copier = counter;
    current = counter->next;
    while(current->free == 0){
        sizetoCopy+=current->size+24;
        current = current->next;
    }
    //NEED TO COPY EACH SET OF DATA STRUCT BY STRUCT
    sizetoCopy = sizeGlobal-sizetoCopy;
    while(copier != NULL){
        copier->size = copier->next->size;
        copier->free = copier->next->free;
       //
        memcpy((void*)copier+24, copier->next+24, copier->next->size);
        if(copier->next->next == NULL )//we have reached the free'd area in mem
        {
            copier->next=NULL;
            return;
        }
        copier = copier->next;
    }
  // memcpy((void*)counter, (void*)counter->next, sizetoCopy);
     */
}


void Free (addrs_t addr) {

/*This frees the previously allocated size bytes starting from address
 * addr in the memory area, M1. You can assume the size argument is
 * stored in a data structure after the Malloc() routine has been called,
 * just as with the UNIX free() command.*/

    if((baseptr<=addr)&&(addr<=baseptr+sizeGlobal)) { //if the address is valid
        struct metaData* current = (char*)addr-24; // DOUBLE CHECK IF IT SHOULD BE -24 OR JUST -0
        if(current->next==NULL){//if current.next is null, means that this is the only block of mem
            current->free=1; //set current to free
            //printf("Freed memory\n");
        } else { //else multiple blocks of mem
            /*int sizeToCopy = current->size+24; //size of data to add to free'd area
            struct metaData* counter = current; // counter
            while(counter->free == 0){ //traverse until we've found the freed area in memory
                counter = counter->next;
            }
            counter->size+= sizeToCopy; //add the size of this free to free'd area */
            current->free=1; //set current to free
            merge(); // merge the rest of the data
            //printf("Freed memory\n");
        }

    } else ("Pointer not Valid\n");


}

void Get (any_t return_data, addrs_t addr, size_t size) {

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
    merge();
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
            return result; //return the block of memory
        } else if ((current->size) >=
                   (size + 24)&& current->free==1) { //if this condition is meet, we've found a block of memory that's smaller than 'size'
            split(current,size); //we need to split the block that has too much space
            result = (char*)current+24; // return the current pointer to the size that has enough space
            //printf("Fitting block allocated with a split\n");
            return result;
        }
        current=current->next;
    };
    //else not enough memory to fit anywhere :-(
    result=NULL;
    printf("Sorry. No sufficient memory to allocate\n");
    return result;


}


addrs_t Put (any_t data, size_t size) {

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
addrs_t *VMalloc(size_t size){
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
            RT[RTCounter] = result; //Set the RT[RTCounter] = the area in mem we've allocated
            //RTable[RTCounter] = RT[RTCounter];
            RTCounter++; // increment the RT counter
            return *RT[RTCounter-1]; //return the block of memory
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
                       RT[i] = result;
                       return RT[i];
                }
            }
            RT[RTCounter] = result; //Set the RT[RTCounter] = the area in mem we've allocated
            //RTable[RTCounter] = RT[RTCounter];

            RTCounter++;
            return RT[RTCounter-1]; //return array addr to block of mem.
        }
        current=current->next;
    };
    //else not enough memory to fit anywhere :-(
    result=NULL;
    printf("Sorry. No sufficient memory to allocate\n");
    return result;
}
void VMerge(int RTcounter){
    /*
    struct metaData* current = heapBasePointer;
    int size;
    while(current != NULL){
        if(current->free == 1){ //if the current metadata block is free
            size = current->size;
            int amountToCopy = 0;
            struct metaData* collecting = current->next;
            while(collecting!=NULL){
                amountToCopy = amountToCopy + collecting->size+24;//amount to copy+= the size of the block + size of the meta data
                if(collecting->next == NULL && collecting->free == 1){ //if this is the last metaData increase the size of the space
                    collecting->size+= size;
                }
                collecting = collecting->next; //increment
            }
            memcpy(current, current->next, amountToCopy);
        }
        current = current->next;
    }*/

    struct metaData *current = heapBasePointer;
    struct metaData *counter = heapBasePointer;
    char* RTPointer = *RT;
    while(current->free!=1){ //find the free'd data
        current=current->next;
    }
    int amountToAdd = current->size+24; //keep track of the amount of mem we're freeing up
    struct metaData* traverse = current;
    int offset = 0;
    while(traverse!=NULL){ //traverse the heap, re allocating data
            struct metaData *helper = traverse;
            if(traverse->free!=1 || traverse->next == NULL){

            }else{
                int x = helper->size + 24; //keep value to use at the end
                int i = 0;
                for (i = 0; i < (globalRTsize); i++){
                    if(RT[i] == helper->next)
                        RT[i] = (char *) current + offset; //re assign the RT value
                }
                memcpy((char*) current + offset, (void *) helper->next, helper->size + 24); //shift the block in mem
                offset += x;
                //check if this is the last block in mem
                if (helper->next ==NULL && helper->free == 1) {
                    helper = traverse; // redundent
                    //memcpy((char*) helper + 24 + helper->size, helper->next, helper->next->size + 24);
                    //helper->next = (char *) helper + 24 + helper->size;
                    helper->size += amountToAdd;
                }
            }
        traverse = traverse->next;
    }
    RT[RTcounter] = NULL;




}
void VFree (addrs_t *addr){

    struct metaData* current = addr;
    if(current->next==NULL){ //if current.next is null, means that this is the only block of mem
        current->free=1; //set current to free
    } else { //else multiple blocks of mem
        current->free=1; //set current to free
        int counter = 0;
        while(RT[counter] != addr){ //traverse the RT to find the block of mem we are trying to remove
            counter++;
        }
        VMerge(counter); // merge the rest of the data
    }
}
addrs_t *VPut (any_t data, size_t size) {

    /*Allocate size bytes from M2 using VMalloc().
     * Copy size bytes of data into Malloc'd memory.
     * You can assume data is a storage area outside M2.
     * Return pointer to redirection table for Malloc'd memory. */


    addrs_t start  = VMalloc(size);//clear area in memory and create pointer
    void* result = (char*)start+24; // we want to store the data offset at a location offset by the size of struct metadata (24)
   /*for(int i=0; i<size; i++){ //clear the area in memory first using a buffer
        int* buffer = (char*)result+i;
        *buffer = 0;
    }*/
    memcpy(result, data, size);
    // *((int*)result) = *((int*)data); //set result to be equal to the data

    return (result); //return start of the data
}

void VGet (any_t return_data, addrs_t *addr, size_t size) {

    /*Copy size bytes from the memory area, M2, to data address.
     * The addr argument specifies a pointer to a redirection table entry.
     * As with VPut(), you can assume data is a storage area outside M2.
     * Finally, de-allocate size bytes of memory using VFree()
     * with addr pointing to a redirection table entry. */


    /*for (int i=0; i<size; i++) {
        int* buffer = (return_data + i);
        *buffer = addr+24+i;
    }*/

    struct metaData* current = addr;
    memcpy(return_data, (void*)addr+24, size);
    current->free=1;
    VFree(addr);

}

void VInit(size_t size) {
    //printf("Allocating area of %lu bytes in memory\n", size);
/* Use the system malloc() routine (or new in C++) ONLY to allocate size bytes
 * for the initial memory area, M1. baseptr is the starting address of M1. */

    sizeGlobal = size;
    baseptr = (addrs_t)malloc(size); //Setting baseptr to point to the start of the heap + size of the RT
    heapBasePointer = baseptr; //create new MetaData pointing to the empty heap
    heapBasePointer->size = size-24; //size of heap = size of heap - size of metadata
    heapBasePointer->free = 1; //free flag set to true
    heapBasePointer->next = NULL; //no other data in the heap, next -> null

    // printf("The starting memory address is: %u\n", baseptr);

}

/*
int main() {
    VInit(200);
    addrs_t P1;
    addrs_t P2;
    addrs_t P3;

    P1 = VPut(P1Data, 2);
    addrs_t P1Value = P1 + 24;
    P2 = VPut(P2Data, 2);
    addrs_t P2Value = P2 + 24;
    VGet(a, P1, 20);


    addrs_t** P1;
    addrs_t** P2;
    addrs_t** P3;
    char P1Data[2] = {'h', 'i'};
    char P2Data[2] = {'n', 'o'};
    char P3Data[3] = {'l', 'o','l'};
    char a[2];
    char n[3];
    addrs_t* RT0 = RT[0];
    addrs_t* RT1 = RT[1];
    addrs_t* RT2 = RT[2];

    P1 = VPut(P1Data, 2);
    addrs_t P1Value = (char*)P1 + 24;
    RT0 = RT[0];
    RT1 = RT[1];
    RT2 = RT[2];


    P2 = VPut(P2Data, 2);
    addrs_t P2Value = (char*)P2 + 24;
    RT0 = RT[0];
    RT1 = RT[1];
    RT2 = RT[2];


    P3 = VPut(P3Data, 3);
    addrs_t P3Value = (char*)P3 +24;
    RT0 = RT[0];
    RT1 = RT[1];
    RT2 = RT[2];


    VGet(a, P2, 2);
    RT0 = RT[0];
    RT1 = RT[1];
    RT2 = RT[2];

    VGet(n, RT[2], 3);
    RT0 = RT[0];
    RT1 = RT[1];
    RT2 = RT[2];



}
*/
/*
int main(){
    Init(1024);
    int err = 0;
    // Round 1 - 2 consequtive allocations should be allocated after one another
    addrs_t v1,v2,v3,v4;
    v1 = Malloc(8);
    v2 = Malloc(4);
    if (LOCATION_OF(v1) >= LOCATION_OF(v2))
        err |= ERROR_NOT_FF;
    if ((LOCATION_OF(v1) & (ALIGN-1)) || (LOCATION_OF(v2) & (ALIGN-1)))
        err |= ERROR_ALIGMENT;
    // Round 2 - New allocation should be placed in a free block at top if fits
    FREE(v1,8);
    v3 = Malloc(16);
    v4 = Malloc(5);
    if (LOCATION_OF(v4) != LOCATION_OF(v1) || LOCATION_OF(v3) < LOCATION_OF(v2))
        err |= ERROR_NOT_FF;
    if ((LOCATION_OF(v3) & (ALIGN-1)) || (LOCATION_OF(v4) & (ALIGN-1)))
        err |= ERROR_ALIGMENT;
    // Round 3 - Correct merge
    FREE(v4,5);
    FREE(v2,4);
    v4 = Malloc(10);
    if (LOCATION_OF(v4) != LOCATION_OF(v1))
        err |= ERROR_NOT_FF;
    // Round 4 - Correct Merge 2
    FREE(v4,10);
    FREE(v3,16);
    v4 = Malloc(256);
    if (LOCATION_OF(v4) != LOCATION_OF(v1))
        err |= ERROR_NOT_FF;
    // Clean-up
    FREE(v4,256);
    return err;
}

*/
int test_stability(int numIterations, unsigned long* tot_alloc_time, unsigned long* tot_free_time){
    int i, n, res = 0;
    char s[80];
    ADDRS addr1, addr2;
    char data[80];
    char data2[80];

    unsigned long start, finish;
    *tot_alloc_time = 0;
    *tot_free_time = 0;

    for (i = 0; i < numIterations; i++) {

       printf("%u\n", i);
        n = sprintf (s, "String 1, the current count is %d\n", i);
        rdtsc(&start);
        addr1 = VPut(s, n+1);
        rdtsc(&finish);
        *tot_alloc_time += finish - start;
        addr2 = VPut(s, n+1);
        // Check for heap overflow
        if (!addr1 || !addr2){
            res |= ERROR_OUT_OF_MEM;
            break;
        }
        // Check aligment
        if (((uint64_t)addr1 & (ALIGN-1)) || ((uint64_t)addr2 & (ALIGN-1)))
            res |= ERROR_ALIGMENT;
        // Check for data consistency
        rdtsc(&start);
        VGet((any_t)data2, addr2, n+1);
        rdtsc(&finish);
        *tot_free_time += finish - start;
        if (!strcmp(data,data2))
            res |= ERROR_DATA_INCON;
        VGet((any_t)data2, addr1, n+1);
        if (!strcmp(data,data2))
            res |= ERROR_DATA_INCON;
    }
    return res;
}
void print_testResult(int code){
    if (code){
        printf("[%sFailed%s] due to: ",KRED,KRESET);
        if (code & ERROR_OUT_OF_MEM)
            printf("<OUT_OF_MEM>");
        if (code & ERROR_DATA_INCON)
            printf("<DATA_INCONSISTENCY>");
        if (code & ERROR_ALIGMENT)
            printf("<ALIGMENT>");
        printf("\n");
    }else{
        printf("[%sPassed%s]\n",KBLU, KRESET);
    }
}
int test_ff(){
    int err = 0;
    // Round 1 - 2 consequtive allocations should be allocated after one another
    addrs_t v1,v2,v3,v4;
    v1 = VMalloc(8);
    v2 = VMalloc(4);
    if (LOCATION_OF(v1) >= LOCATION_OF(v2))
        err |= ERROR_NOT_FF;
    if ((LOCATION_OF(v1) & (ALIGN-1)) || (LOCATION_OF(v2) & (ALIGN-1)))
        err |= ERROR_ALIGMENT;
    // Round 2 - New allocation should be placed in a free block at top if fits
    VFree(v1);
    v3 = VMalloc(16);
    v4 = VMalloc(5);
    if ( LOCATION_OF(v4) != LOCATION_OF(v1) ||LOCATION_OF(v3) < LOCATION_OF(v2))
        err |= ERROR_NOT_FF;
    if ((LOCATION_OF(v3) & (ALIGN-1)) || (LOCATION_OF(v4) & (ALIGN-1)))
        err |= ERROR_ALIGMENT;
    // Round 3 - Correct merge
    VFree(v4);
    VFree(v2);
    v4 = VMalloc(10);
    if (LOCATION_OF(v4) != LOCATION_OF(v1))
        err |= ERROR_NOT_FF;
    // Round 4 - Correct Merge 2
    VFree(v4);
    VFree(v3);
    v4 = Malloc(256);
    if (LOCATION_OF(v4) != LOCATION_OF(v1))
        err |= ERROR_NOT_FF;
    // Clean-up
    VFree(v4);
    return err;
}

int test_maxNumOfAlloc(){
    int count = 0;
    char *d = "x";
    const int testCap = 1000000;
    ADDRS allocs[testCap];

    while ((allocs[count]=VPut(d,1)) && count < testCap){
        if (DATA_OF(allocs[count])!='x') break;
        if(count = 2035){
            printf("hello");
        }
        count++;
    }
    // Clean-up
    int i;
    for (i = 0 ; i < count ; i++)
        VFree(allocs[i]);
    return count;
}

int main(){
    VInit(1<<20); // Default)
    unsigned long tot_alloc_time, tot_free_time;
    int numIterations = 1000000;
    /*printf("Test 1 - Stability and consistency:\t");
    print_testResult(test_stability(numIterations,&tot_alloc_time,&tot_free_time));
    printf("\tAverage clock cycles for a Malloc request: %lu\n",tot_alloc_time/numIterations);
    printf("\tAverage clock cycles for a Free request: %lu\n",tot_free_time/numIterations);
    printf("\tTotal clock cycles for %d Malloc/Free requests: %lu\n",numIterations,tot_alloc_time+tot_free_time);
    print_testResult(test_stability(numIterations,&tot_alloc_time,&tot_free_time));
    //printf("Test 2 - First-fit policy:\t\t");
    //print_testResult(test_ff()); */
    printf("Test 3 - Max # of 1 byte allocations:\t");
    printf("[%s%i%s]\n",KBLU,test_maxNumOfAlloc(), KRESET);


};
/* ~~~~~~~~~~~~~~~~~~~~~USE THIS FOR TESTING~~~~~~~~~~~~~~~~~~~~~~~~~
 */
/*
void main (int argc, char **argv) {
    int i, n;
    char s[80];
    addrs_t addr1, addr2;
    char data[80];
    int mem_size = 1<<20; // Set DEFAULT_MEM_SIZE to 1<<20 bytes for a heap region
    if (argc > 2) {
        fprintf (stderr, "Usage: %s [memory area size in bytes]\n", argv[0]); exit (1);
    }
    else if (argc == 2)
        mem_size = atoi (argv[1]);
    Init (mem_size);
    for (i = 0;; i++) {
        n = sprintf (s, "String 1, the current count is %d\n", i);
        addr1 = Put (s, n+1);
        addr2 = Put (s, n+1);
        if (addr1)
            printf ("Data at %x is: %s\n", addr1, addr1);
        if (addr2)
            printf ("Data at %x is: %s\n", addr2, addr2);
        if (addr2)
            Get ((any_t)data, addr2, n+1);
        if (addr1)
            Get ((any_t)data, addr1, n+1); }
}
*/