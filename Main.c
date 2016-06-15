#include "General_functions.h"
#include "OpenCL_functions.h"

#define max_platforms 5
#define main_platform 0

#define max_devices 10
#define main_device 1

#define max_files 1024
#define max_filename_sz 256

int main(int argc, char *argv[]){

	struct timeval checkpoint[10];
	gettimeofday(&checkpoint[0], NULL);
	bool compress = true;
	bool recursive_compress = false;
	bool platform_info = false;
	bool gpu_info = false;
	bool timing_info = false;
	bool useOpenCL = true;
	cl_uint block_size = 49152;
	cl_uint HashTable_entries = 8192;
	cl_ushort NUM_THREADS = 128;
	cl_ushort pll = 1;
	
	int i;
	for(i = 1; i < argc; i++){
		if(argv[i][0] != '-'){
			break;
		}
		if(argv[i][1] == 0 || argv[i][2] != 0){
			printf("Invalid Tag !!\n");
			print_usage(argv[0]);
		}
		switch(argv[i][1]){
		
			case 'R':
			recursive_compress = true;
			break;
			
			case 'D':
			compress = false;
			char temp = atoi(argv[++i]);
			if(temp == 1) useOpenCL = false;
			else if(temp != 0){
				printf("Invalid Value for Decompress tag !!\n");
				print_usage(argv[0]);
			}
			break;
			
			case 'B':
			block_size = atoi(argv[++i]);
			if(block_size > 49152 || block_size < 1024 ){
				printf("Invalid Block Size !!\n");
				print_usage(argv[0]);
			}
			break;
			
			case 'T':
			NUM_THREADS = atoi(argv[++i]);
			if(NUM_THREADS < 8 || NUM_THREADS > 1024){
				printf("Invalid Work-Group size !!\n");
				print_usage(argv[0]);
			}
			break;
			
			case 'P':
			pll = atoi(argv[++i]);
			if(pll < 1){
				printf("Invalid value for tag -P !!\n");
				print_usage(argv[0]);
			}
			break;
			
			case 'p':
			platform_info = true;
			break;
			
			case 'g':
			gpu_info = true;
			break;
			
			case 't':
			timing_info = true;
			break;
			
			default:
			printf("Invalid Tag !!\n");
			print_usage(argv[0]);
		}
	}
	if(argc - i != 2){
		printf("Insufficient Parameters !!\n");
		print_usage(argv[0]);
	}
	
	char **files = (char **)malloc(max_files*sizeof(char *));
	char **out_files = (char **)malloc(max_files*sizeof(char *));
	if(files == NULL || out_files == NULL){
		printf("Malloc failed: files, out_files\n");
		exit(1);
	}
	
	int no_files = 0;
	if(recursive_compress){
		no_files = RecursiveCompressFiles(max_files, max_filename_sz, argv[argc-2], argv[argc-1], files, out_files);
	}
	else{
		no_files = 1;
		files[0] = argv[argc-2];
		out_files[0] = argv[argc-1];
	}
	if(no_files == 0){
		printf("Empty Directory cannot be compressed !!\n");
		exit(1);
	}
	
	gettimeofday(&checkpoint[1], NULL);
	if(timing_info) printf("Program Tag validation = %f ms\n",(double) (checkpoint[1].tv_usec - checkpoint[0].tv_usec) / 1000 + (double) (checkpoint[1].tv_sec - checkpoint[0].tv_sec)*1000);
	
	if(!useOpenCL) goto Label1;
	cl_int err_code;
	char buffer[10240];

	cl_uint num_platforms;
	cl_platform_id platforms[max_platforms];
	cl_platform_id default_platform;

	cl_uint num_devices;
	cl_device_id devices[max_devices];
	cl_device_id default_device;

	cl_context context;

	cl_command_queue command_queue;

	cl_mem in_memobj;
	cl_mem out_memobj;
	cl_mem idxes;

	cl_program program;

	cl_kernel kernel;


	err_code = clGetPlatformIDs(max_platforms, &platforms[0], &num_platforms);
	checkErr(err_code, "clGetPlatformIDs");
	printf("OpenCL platform(s) available = %d\n", num_platforms);
	default_platform = platforms[main_platform];
	printf("Setting Platform %d as default platform\n", main_platform);

	if(platform_info) Display_PlatformInfo(default_platform, buffer);

	err_code = clGetDeviceIDs(default_platform, CL_DEVICE_TYPE_GPU, max_devices, &devices[0], &num_devices);
	checkErr(err_code, "clGetDeviceIDs");
	printf("No of GPU('s) available = %d\n", num_devices);
	default_device = devices[main_device];
	printf("Using GPU %d as default device\n", main_device);

	if(gpu_info) Display_DeviceInfo(default_device, buffer);

	context = clCreateContext(NULL, 1, &default_device, NULL, NULL, &err_code);
	checkErr(err_code, "clCreateContext");

	if(compress) command_queue = clCreateCommandQueue(context, default_device,  CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err_code);
	else command_queue = clCreateCommandQueue(context, default_device,  0, &err_code);
	checkErr(err_code, "clCreateCommandQueue");

	char *srcpath;
	if(compress) srcpath = "compress.cl";
	else srcpath = "decompress.cl";
	FILE *src = fopen(srcpath, "r");
	if(src == NULL){
		printf("Couldn't open the souce file %s\n", srcpath);
		exit(1);
	}
	int src_len = -1;
	fseek(src, 0, SEEK_END);
	src_len = ftell(src);
	fseek(src,0, SEEK_SET);
	if(src_len <= 0){
		printf("Souce file is empty or invalid %s\n", srcpath);
		exit(1);
	}
	char *src_code = (char *)malloc(src_len);
	src_len = fread(src_code, 1, src_len, src);
	fclose(src);

	program = clCreateProgramWithSource(context, 1, (const char **)&src_code, (const size_t *)&src_len, &err_code);
    	free(src_code);
    	checkErr(err_code, "clCreateProgramWithSource");
    	
    	err_code = clBuildProgram(program, 1, &default_device, NULL, NULL, NULL);
    	char *log = (char*)malloc(8192);
    	if(log == NULL){
    		printf("Malloc Failed: log\n");
    		exit(1);
    	}
    	err_code |= clGetProgramBuildInfo(program, default_device, CL_PROGRAM_BUILD_LOG, 8192, log, NULL);
	printf("*****\nKerel Compilation Error Log:\n%s\n*****\n", log); 
    	free(log);
    	checkErr(err_code, "clBuildProgram");
    	
    	cl_uint compute_units;
    	if(compress) kernel = clCreateKernel(program, "lzo1x_1_15_compress", &err_code);
    	else kernel = clCreateKernel(program, "lzo1x_1_15_decompress", &err_code);
    	err_code = clGetDeviceInfo(default_device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
	checkErr(err_code, "clGetDeviceInfo");

	size_t szGlobalWorkSize[1];
    	size_t szLocalWorkSize[1];
    	
    	gettimeofday(&checkpoint[2], NULL);
	if(timing_info) printf("OpenCL Initializations = %f ms\n",(double) (checkpoint[2].tv_usec - checkpoint[1].tv_usec) / 1000 + (double) (checkpoint[2].tv_sec - checkpoint[1].tv_sec)*1000);
	
Label1:	for(i = 0; i < no_files; i++){
		printf("\n");
		gettimeofday(&checkpoint[1], NULL);
		FILE *fp = fopen(files[i], "r");
		if (fp == NULL){
      			printf("Couldn't open file %s;\n", files[i]);
      			continue;
      		}
		fseek(fp, 0, SEEK_END);
    		cl_uint in_size = ftell(fp);
    		fseek(fp, 0, SEEK_SET);
    		if(in_size <= 0){
        		printf("%s: is an empty file\n", files[i]);
        		fclose(fp);
        		continue;
    		}

    		unsigned char *in_ptr = (unsigned char *)malloc(in_size);
    		if(in_ptr == NULL){
        		printf("%s: out of memory (Read Buffer)\n", files[i]);
        		continue;
    		}
    		in_size = (cl_uint)fread(in_ptr, 1, in_size, fp);
		fclose(fp);
		
		gettimeofday(&checkpoint[2], NULL);
		if(timing_info) printf("Reading FIle %s to CPU Memory = %f ms\n", files[i], (double) (checkpoint[2].tv_usec - checkpoint[1].tv_usec)/1000 + (double)(checkpoint[2].tv_sec - checkpoint[1].tv_sec)*1000);
    		printf("Loaded file %s: %ld bytes\n", files[i], (long) in_size);
    		
    		if(compress){
    			in_memobj = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, in_size*sizeof(unsigned char), in_ptr, &err_code);
    			checkErr(err_code, "clCreateBuffer:in");
    			gettimeofday(&checkpoint[3], NULL);
			if(timing_info) printf("Reading file %s to GPU = %f ms\n",files[i], (double) (checkpoint[3].tv_usec - checkpoint[2].tv_usec)/1000 + (double) (checkpoint[3].tv_sec - checkpoint[2].tv_sec)*1000);	
    		
	    		int blocks = in_size%block_size == 0 ? in_size/block_size : in_size/block_size + 1;
			int blocksPerLaunch = blocks > compute_units ? compute_units : blocks;
			blocksPerLaunch *= pll;
		
			cl_uint out_size = -1;
	    		if(compress) out_size = (block_size + block_size/16 + 64 + 3)*blocks;
			out_memobj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, out_size*sizeof(unsigned char), NULL, &err_code);
	    		checkErr(err_code, "clCreateBuffer:out");
	    		
			szGlobalWorkSize[0] = blocksPerLaunch * NUM_THREADS;
	    		szLocalWorkSize[0] = NUM_THREADS;
	    		
	    		cl_mem Endpts = clCreateBuffer(context, CL_MEM_WRITE_ONLY, blocks*sizeof(cl_uint), NULL, &err_code);
	    		checkErr(err_code, "clCreateBuffer:Endpts");
	    		
	    		cl_mem Debug = clCreateBuffer(context, CL_MEM_WRITE_ONLY, szGlobalWorkSize[0]*sizeof(cl_uint), NULL, &err_code);
	    		checkErr(err_code, "clCreateBuffer:Debug");
	    		
	    		cl_mem HashTbl = clCreateBuffer(context, CL_MEM_READ_WRITE, 8192*blocks*sizeof(cl_ushort), NULL, &err_code);
	    		checkErr(err_code, "clCreateBuffer:HashTbl");
	    		
	    		err_code = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&in_memobj);
	    		err_code |= clSetKernelArg(kernel, 1, sizeof(cl_uint), (void *)&in_size);
	    		err_code |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&out_memobj);
	    		err_code |= clSetKernelArg(kernel, 3, sizeof(cl_uint), (void *)&out_size);
	    		err_code |= clSetKernelArg(kernel, 4, sizeof(cl_uint), (void *)&block_size);
	    		err_code |= clSetKernelArg(kernel, 5, sizeof(int), (void *)&blocksPerLaunch);
	    		err_code |= clSetKernelArg(kernel, 6, sizeof(cl_mem), (void *)&Endpts);
	    		err_code |= clSetKernelArg(kernel, 7, sizeof(cl_mem), (void *)&Debug);
	    		err_code |= clSetKernelArg(kernel, 8, sizeof(cl_mem), (void *)&HashTbl);
			checkErr(err_code, "clSetKernelArg");
			int j, iter;
			for(j = 0, iter = 0; j < blocks; j += blocksPerLaunch, iter++){
				err_code = clSetKernelArg(kernel, 9, sizeof(int), (void *)&(iter));
				checkErr(err_code, "clSetKernelArg");
				if(blocks - j < blocksPerLaunch)szGlobalWorkSize[0] = (blocks-j)*NUM_THREADS;
				err_code = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, szGlobalWorkSize, szLocalWorkSize, 0, NULL, NULL);
				checkErr(err_code, "clEnqueNDRangeKernel");
			}
	    		clFinish(command_queue);
	    		gettimeofday(&checkpoint[4], NULL);
			if(timing_info) printf("Time taken for Output Buffer creation + Kernel execution time %f ms\n",(double) (checkpoint[4].tv_usec - checkpoint[3].tv_usec) / 1000 + (double) (checkpoint[4].tv_sec - checkpoint[3].tv_sec)*1000);
	    		
	    		unsigned char *out_ptr = (unsigned char *)malloc(out_size);
	    		cl_uint *endpts = malloc(blocks*sizeof(cl_uint));
	    		unsigned int* Debug_vals = (unsigned int *)malloc(szGlobalWorkSize[0]*sizeof(unsigned int));
	    		if(out_ptr == NULL || endpts == NULL || Debug_vals == NULL){
				printf("%s: out of memory (Write Buffer)\n", files[i]);
				continue;
	    		}

	    		err_code = clEnqueueReadBuffer(command_queue, Endpts, CL_TRUE, 0, blocks*sizeof(cl_uint), endpts, 0, NULL, NULL);
			checkErr(err_code, "Pos1");
	    		err_code |= clEnqueueReadBuffer(command_queue, Debug, CL_TRUE, 0, szGlobalWorkSize[0]*sizeof(unsigned int), Debug_vals, 0, NULL, NULL);
			checkErr(err_code, "Pose2");
			err_code |= clEnqueueReadBuffer(command_queue, out_memobj, CL_TRUE, 0, out_size*sizeof(unsigned char), out_ptr, 0, NULL, NULL);
	    		checkErr(err_code, "clEnqueueReadBuffer");

			int lenbyts = 0;			//header construction for storing length of original file in compressed file
			unsigned int sz = in_size;
			while(sz != 0){
				sz = sz/256;
				lenbyts++;
			}
			sz = lenbyts;
			int loglenbyts = 0;
			while(sz > 255){
				sz -= 255;
				loglenbyts++;
			}
			loglenbyts++;
			unsigned char Header[lenbyts + loglenbyts];
			sz = lenbyts;
			for(j = 0; j < loglenbyts; j++){
				if(sz > 255){
					sz -= 255;
					Header[j] = 0;
				}
				else Header[j] = sz;
			}
			sz = in_size;
			for(j = 0; j < lenbyts; j++){
				Header[j + loglenbyts] = sz%256;
				sz = sz/256;
			}
	    		
			gettimeofday(&checkpoint[5], NULL);
	    		if(timing_info) printf("Time taken for Reading Output Buffer %f ms\n",(double) (checkpoint[5].tv_usec - checkpoint[4].tv_usec) / 1000 + (double) (checkpoint[5].tv_sec - checkpoint[4].tv_sec)*1000);
	    		

	    		fp = fopen(out_files[i], "w");
	    		if(fp == NULL){
	    			printf("Couldn't open file %s for writing output\n", out_files[i]);
	    			continue;
	    		}
	    		if(fwrite(Header,1,loglenbyts+lenbyts, fp) <=0 ) printf("Writing of Header failed for %s\n", out_files[i]);
			unsigned char* block_data;
	    		cl_uint blksz = out_size/blocks;
	    		int tmp;
	    		int sum = 0;
	    		for(j = 0; j < blocks; j++){
	    			block_data = out_ptr + j*blksz;
	    			tmp = fwrite(block_data, 1, endpts[j], fp);
	    			sum += endpts[j];
	    			if(tmp <= 0) printf("Writer of block %d failed for %s\n", j+1, out_files[i]);
	    		}
	    		printf("Written %d bytes to %s\n", sum, out_files[i]);
	    		
	    		gettimeofday(&checkpoint[6], NULL);
	    		if(timing_info) printf("Writing file %f ms\n",(double) (checkpoint[6].tv_usec - checkpoint[5].tv_usec) / 1000 + (double) (checkpoint[6].tv_sec - checkpoint[5].tv_sec)*1000);
	    		
	    		fclose(fp);
	    		free(out_ptr);
    			free(endpts);
    			free(Debug_vals);
    		
    			clReleaseMemObject(Endpts);
    			clReleaseMemObject(Debug);
			clReleaseMemObject(HashTbl);
    		}
    		else{
    			cl_uint out_size = 0;				//Decoding length of original file from header
			int j = 0;
			int bits = 0;
			while(in_ptr[j] == 0){
				bits += 255;
				j++;
			}
			bits += (unsigned char)in_ptr[j];
			j++;
			j += bits;
			int k = 0;
			while(k < bits){
				out_size = out_size*256 + (unsigned char)in_ptr[--j];
				k++;	
			}
			j += bits;
			in_size -= j;
			in_ptr += j;
			int headerlen = j;
    			
    			if(!useOpenCL) goto Label2;
    			in_memobj = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, in_size*sizeof(unsigned char), in_ptr, &err_code);
    			checkErr(err_code, "clCreateBuffer:in");
    			gettimeofday(&checkpoint[3], NULL);
			if(timing_info) printf("Reading file %s to GPU = %f ms\n",files[i], (double) (checkpoint[3].tv_usec - checkpoint[2].tv_usec) / 1000 + (double) (checkpoint[3].tv_sec - checkpoint[2].tv_sec)*1000);
    			
    			szGlobalWorkSize[0] = NUM_THREADS;
    			szLocalWorkSize[0] = NUM_THREADS;
    			
    			out_memobj = clCreateBuffer(context, CL_MEM_READ_WRITE, out_size*sizeof(unsigned char), NULL, &err_code);
	    		checkErr(err_code, "clCreateBuffer:out");
	    		
	    		unsigned char *out_ptr;
	    		//unsigned char * Debug_out = (unsigned char*)malloc(5000*sizeof(char));
	    		err_code = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&in_memobj);
	    		err_code |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&out_memobj);
	    		err_code |= clSetKernelArg(kernel, 2, sizeof(cl_ushort), (void *)&NUM_THREADS);
	    		checkErr(err_code, "clSetKernelArg");
	    		
Label2:			if(!useOpenCL) gettimeofday(&checkpoint[3], NULL);	    		
			FILE *fp = fopen(out_files[i], "w");
	    		if(fp == NULL){
	    			printf("Couldn't open file %s for writing output\n", out_files[i]);
	    			continue;
	    		}
	    		if(!useOpenCL)out_ptr = (unsigned char*)malloc(out_size*sizeof(char));
	    		
	    		unsigned int startidx = 0,endidx = 0, start_inp = 0;
			unsigned int temp_t = 0;
			while(start_inp < in_size){
	    			unsigned short m_len = 0, m_off = 0;
	    			unsigned short cpy_len = 0;
	    			unsigned int t = 0;
				if(temp_t == 0)t = (unsigned int)in_ptr[start_inp++];
	    			unsigned short tmp = 0;
	    			if(temp_t != 0){
	    				t = temp_t;
	    				cpy_len = t;
					endidx += t;
	    			}
	    			else if(t > 17 && !start_inp){
	    				t -= 17;
	    				cpy_len = t;
	    				endidx += t;
	    			}
	    			else if(t < 16){
	    				if(t == 0){
	    					while(in_ptr[start_inp] == 0){
	    						t += 255;
	    						start_inp++;
	    					}
	    					t += 15 + in_ptr[start_inp++];
	    				}
	    				t += 3;
	    				endidx += t;
	    				cpy_len += t;
	    			}
	    			else{
					cpy_len = 0;
	    				start_inp--;
	    			}
    				unsigned int tt;
    				tt = (unsigned int)in_ptr[start_inp + cpy_len];
    				if(tt >= 64){
    					m_off = 1;
    					m_off += (tt >> 2) & 7;
    					m_off += ((unsigned int)in_ptr[start_inp + cpy_len + 1]) << 3;
    					m_len = (tt >> 5) + 1;
    					endidx += m_len;
    					tmp = 2;
    					temp_t = tt & 3;
    				}
    				else if(tt >= 32){
    					tt &= 31;
    					tmp = 1;
    					if(tt == 0){
    						while(in_ptr[start_inp + cpy_len + tmp] == 0){
    							tt += 255;
    							tmp++;
    						}
    						tt += 31 + in_ptr[start_inp + cpy_len + tmp];
    						tmp++;
    					}
    					m_len = tt + 2;
    					m_off = 1;
    					m_off += ((unsigned short)in_ptr[start_inp + cpy_len + tmp] >> 2);
    					temp_t = ((unsigned short)in_ptr[start_inp + cpy_len + tmp] & 3);
    					m_off += ((unsigned short)in_ptr[start_inp + cpy_len + tmp + 1] << 6);
    					tmp += 2;
    					endidx += m_len;   					
    				}
    				else if(tt >= 16){
    					m_off = (tt & 8)<<11;
    					tt &= 7;
    					tmp = 1;
    					if(tt == 0){
    						while(in_ptr[start_inp + cpy_len + tmp] == 0){
    							tt += 255;
    							tmp++;
    						}
    						tt += 7 + in_ptr[start_inp + cpy_len + tmp];
    						tmp++;
    					}
    					m_len = tt + 2;
    					m_off += ((unsigned short)in_ptr[start_inp + cpy_len + tmp] >> 2);
    					temp_t = ((unsigned short)in_ptr[start_inp + cpy_len + tmp] & 3);
    					m_off += ((unsigned short)in_ptr[start_inp + cpy_len + tmp + 1] << 6);
    					tmp += 2;
    					if(m_off == 0) m_len = 0;
    					else m_off += 0x4000;
    					endidx += m_len;
    				}
    				else{
    					printf("Logical error encountered .. Location: %d %d!!\n", start_inp, cpy_len);
    					break;
    				}
	    			if(useOpenCL){
		    			err_code = clSetKernelArg(kernel, 3, sizeof(unsigned int), (void *)&startidx);
		    			err_code |= clSetKernelArg(kernel, 4, sizeof(unsigned int), (void *)&start_inp);
		    			err_code |= clSetKernelArg(kernel, 5, sizeof(unsigned short), (void *)&cpy_len);
		    			err_code |= clSetKernelArg(kernel, 6, sizeof(unsigned short), (void *)&m_len);
		    			err_code |= clSetKernelArg(kernel, 7, sizeof(unsigned short), (void *)&m_off);
		    			checkErr(err_code, "clSetKernelArg");
		    			err_code = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, szGlobalWorkSize, szLocalWorkSize, 0, NULL, NULL);
					checkErr(err_code, "clEnqueNDRangeKernel");
				}
				//printf("start_inp:%d startidx:%d cpy_len:%d tmp:%d m_len:%d m_off:%d endidx:%d\n", start_inp, startidx, cpy_len, tmp, m_len, m_off, endidx);
				/*printf("-->");
				int j;
				for(j = start_inp; j < start_inp + cpy_len; j++){
					printf("%c", in_ptr[j]);
					Debug_out[startidx + (j-start_inp)] = in_ptr[j];
				}
				for(j = startidx+cpy_len; j < endidx; j++){
					printf("%c", Debug_out[j-m_off]);
					Debug_out[j] = Debug_out[j-m_off];
				}
				printf("<--\n");*/
				else{
					memcpy(out_ptr+startidx, in_ptr+start_inp, cpy_len);
					int j;
					for(j = startidx+cpy_len; j < endidx; j++){
						out_ptr[j] = out_ptr[j-m_off];
					}
				}
	    			start_inp += cpy_len + tmp;
	    			startidx = endidx;
	    		}
	    		if(useOpenCL)clFinish(command_queue);
	    		
	    		gettimeofday(&checkpoint[4], NULL);
	    		if(timing_info){
			printf("Time taken for Output Buffer creation + Kernel execution time %f ms\n",(double) (checkpoint[4].tv_usec - checkpoint[3].tv_usec) / 1000 + (double) (checkpoint[4].tv_sec - checkpoint[3].tv_sec)*1000);
			}
	    		
	    		//printf("%s", Debug_out);
	    		if(!useOpenCL) goto Label3;
	    		out_ptr = (unsigned char *)malloc(endidx);
	    		err_code = clEnqueueReadBuffer(command_queue, out_memobj, CL_TRUE, 0, endidx, out_ptr, 0, NULL, NULL);
	    		checkErr(err_code, "clEnqueueReadBuffer");
	    		
	    		gettimeofday(&checkpoint[5], NULL);
	    		if(timing_info){
			printf("Time taken for Reading Output Buffer %f ms\n",(double) (checkpoint[5].tv_usec - checkpoint[4].tv_usec) / 1000 + (double) (checkpoint[5].tv_sec - checkpoint[4].tv_sec)*1000);
			}
	    		
Label3:	    		if(!useOpenCL) gettimeofday(&checkpoint[5], NULL);
			endidx = fwrite(out_ptr, 1, (unsigned int)endidx, fp);
	    		if(endidx <= 0) printf("Write to file %s failed\n", out_files[i]);
	    		else printf("Successfully decompressed file to %u bytes\n", endidx);
			fclose(fp);
	    		free(out_ptr);
	    		
	    		gettimeofday(&checkpoint[6], NULL);
	    		if(timing_info){
			printf("Writing file %f ms\n",(double) (checkpoint[6].tv_usec - checkpoint[5].tv_usec) / 1000 + (double) (checkpoint[6].tv_sec - checkpoint[5].tv_sec)*1000);
			}
			in_ptr -= headerlen;
    		}
    		free(in_ptr);
    		
    		if(useOpenCL){
    			clReleaseMemObject(in_memobj);
    			clReleaseMemObject(out_memobj);
    		}
	}
	
	if(recursive_compress){
		int i;
		for(i = 0; i < no_files; i++){
			free(files[i]);
			free(out_files[i]);
		}
	}
	free(files);
	free(out_files);	
		
	if(useOpenCL){
		clReleaseKernel(kernel);
	    	clReleaseProgram(program);
	    	clReleaseCommandQueue(command_queue);
	    	clReleaseContext(context);
    	}
    	
    	gettimeofday(&checkpoint[7], NULL);
	if(timing_info) printf("Freeing OpenCL Resources %f ms\n",(double) (checkpoint[7].tv_usec - checkpoint[6].tv_usec) / 1000 + (double) (checkpoint[7].tv_sec - checkpoint[6].tv_sec)*1000);
	
	return 0;
}
