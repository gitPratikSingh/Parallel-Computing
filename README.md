# Parallel-Computing
Used MPI and Cuda to simulate a lake 


The program runs two versions of the algorithm, a CPU version, and a skeleton GPU version. The parallel program can simulate 100X larger lake in the same time as the serial program.

![alt text](https://github.com/gitPratikSingh/Parallel-Computing/blob/master/13pt_128_6_0.5_8.png)


This program models the surface of a lake, where some pebbles have been thrown onto the surface. The program works as follows. In the spatial domain, a centralized finite difference is used to inform a zone of how to update itself using the information from its neighbors. The time domain does something similarly, but here using information from the previous two times


