# Upenn CIS505 HW1  
threads use bubblesort, main thread merge sorted pieces like mergesort.  
the communcation is through pipe.  

## generate numbers in a file
./makeinput [size] [filename]
## run parallel sort
./mysort [-n number] [-t]  
-n followed by number of threads or processes, default is 4  
-t whether use multithread or multiprocess, default is process option
