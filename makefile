path=/usr/local/cuda-6.0/targets/x86_64-linux/include
#Chnage path according to location of OpenCL installation path on your particular machine

all: Main.o OpenCL_functions.o General_functions.o
	gcc -I $(path) Main.o OpenCL_functions.o General_functions.o -o GPU_compress -lOpenCL

Main.o: Main.c
	gcc -c -I $(path) Main.c -lOpenCL

OpenCL_functions.o: OpenCL_functions.c
	gcc -c -I  $(path) OpenCL_functions.c -lOpenCL
	
General_functions.o: General_functions.c
	gcc -c General_functions.c 

clean:
	rm *o GPU_compress;
