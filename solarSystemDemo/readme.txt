the code in main.c does the following:
1.Displays the number of OpenCL platforms on a computer. 
2.query device info
3.Create a context with one device 
4.Create a command queue for the context and device. 
5.Read the program source code from the provided “source.cl” file and build the program. 
6.Find and display the number of kernels in the program. 
7.Create kernels from the program and display all the kernel function names. 