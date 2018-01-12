README - CS210 FINAL PROJECT - DEC 12, 2017 

This project was created by: 
	Sidhartha Premkumar

pa31.c: Contains the regular heap: 
	-  Free (addrs_t addr): To free data at a given address 'addr'
	-  Get (any_t return_data, addrs_t addr, size_t size): To get data of size 'size' from address 'addr' in the heap and place it in 'return_data' 
	-  Init(size_t size): clear an area in memory to allocate for the heap of size 'size' [via the C library malloc function]
	-  Malloc (size_t size): clear an area within the allocated area of size 'size' 
	-  Put (any_t data, size_t size): Put data 'data' of size 'size' in the heap 
	-  heapChecker(): heapChecker function 

pa32.c: Contrains the VHeap: 
	- VFree (addrs_t *addr): To free data at a given address 'addr'
	- VGet (any_t return_data, addrs_t *addr, size_t size): To get data of size 'size' from address 'addr' in the heap and place it in 'return_data' 
	- VInit(size_t size):  clear an area in memory to allocate for the heap of size 'size' [via the C library malloc function]
	- *VMalloc(size_t size): clear an area within the allocated area of size 'size' 
	- VPut (any_t data, size_t size): Put data 'data' of size 'size' in the heap 
	- VheapChecker(): heapChecker function 
	