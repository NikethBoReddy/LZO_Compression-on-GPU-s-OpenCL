/* Author: Niketh Boreddy
   email: nikethcse@gmail.com
   
  Date: 10th June 2016
  The follwing file is a part of the LZO algortihm implementation for GPU's
*/

#include "OpenCL_functions.h"
#include "General_functions.h"

inline void checkErr(cl_int err, const char * name){
	if (err != CL_SUCCESS) {
		if(err == CL_MEM_OBJECT_ALLOCATION_FAILURE)printf("CL_MEM_OBJECT_ALLOCATION_FAILURE\n");
		/*else if(err == CL_OUT_OF_HOST_MEMORY)printf("CL_OUT_OF_HOST_MEMORY\n");
		else if(err == CL_INVALID_COMMAND_QUEUE)printf("CL_INVALID_COMMAND_QUEUE\n");
		else if(err == CL_INVALID_CONTEXT)printf("CL_INVALID_CONTEXT\n");
		else if(err == CL_INVALID_MEM_OBJECT)printf("CL_INVALID_MEM_OBJECT\n");
		else if(err == CL_INVALID_VALUE)printf("CL_INVALID_VALUE\n");
		else if(err == CL_INVALID_EVENT_WAIT_LIST)printf("CL_INVALID_EVENT_WAIT_LIST\n");
		else if(err == CL_OUT_OF_RESOURCES)printf("CL_OUT_OF_RESOURCES\n");
		else if(err == CL_INVALID_KERNEL_ARGS)printf("CL_INVALID_KERNEL_ARGS\n");
		else if(err == CL_INVALID_BUFFER_SIZE)printf("CL_INVALID_BUFFER_SIZE\n");*/
		printf("ERROR: %s (%d)\n", name, (int)err);
		exit(1);
	}
}

void Display_PlatformInfo(cl_platform_id default_platform, char *buffer){
	cl_int err_code;
	err_code = clGetPlatformInfo(default_platform, CL_PLATFORM_PROFILE, 10240, buffer, NULL);
	checkErr(err_code, "clGetPlatformIInfo:PlatformProfile");
	printf("\n  PROFILE = %s\n", buffer);
	err_code = clGetPlatformInfo(default_platform, CL_PLATFORM_VERSION, 10240, buffer, NULL);
	checkErr(err_code, "clGetPlatformIInfo:PlatformVersion");
	printf("  VERSION = %s\n", buffer);
	err_code = clGetPlatformInfo(default_platform, CL_PLATFORM_NAME, 10240, buffer, NULL);
	checkErr(err_code, "clGetPlatformIInfo:PlatformName");
	printf("  NAME = %s\n", buffer);
	err_code = clGetPlatformInfo(default_platform, CL_PLATFORM_VENDOR, 10240, buffer, NULL);
	checkErr(err_code, "clGetPlatformIInfo:PlatformVendor");
	printf("  VENDOR = %s\n", buffer);
	err_code = clGetPlatformInfo(default_platform, CL_PLATFORM_EXTENSIONS, 10240, buffer, NULL);
	checkErr(err_code, "clGetPlatformIInfo:PlatformExtensions");
	printf("  EXTENSIONS = %s\n\n", buffer);
}

void Display_DeviceInfo(cl_device_id default_device, char *buffer){
	cl_int err_code;
	cl_uint buf_uint;
	cl_ulong buf_ulong;
	size_t buf_size;
	err_code = clGetDeviceInfo(default_device, CL_DEVICE_NAME, 10240, buffer, NULL);
	checkErr(err_code,"clGetDeviceInfo:DeviceName");
	printf("\n  DEVICE_NAME = %s\n", buffer);
	err_code = clGetDeviceInfo(default_device, CL_DEVICE_VENDOR, 10240, buffer, NULL);
	checkErr(err_code,"clGetDeviceInfo:DeviceVendor");
	printf("  DEVICE_VENDOR = %s\n", buffer);
	err_code = clGetDeviceInfo(default_device, CL_DEVICE_VERSION, 10240, buffer, NULL);
	checkErr(err_code,"clGetDeviceInfo:Deviceversion");
	printf("  DEVICE_VERSION = %s\n", buffer);
	err_code = clGetDeviceInfo(default_device, CL_DRIVER_VERSION, 10240, buffer, NULL);
	checkErr(err_code,"clGetDeviceInfo:DeviceDriverVersion");
	printf("  DRIVER_VERSION = %s\n", buffer);
	err_code = clGetDeviceInfo(default_device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(buf_uint), &buf_uint, NULL);
	checkErr(err_code,"clGetDeviceInfo:DeviceMaxComputeUnits");
	printf("  DEVICE_MAX_COMPUTE_UNITS = %u\n", (unsigned int)buf_uint);
	err_code = clGetDeviceInfo(default_device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(buf_size), &buf_size, NULL);
	checkErr(err_code,"clGetDeviceInfo:DeviceMaxWorkGroupSizes");
	printf("  DEVICE_MAX_WORK_GROUP_SIZES = %u\n", (unsigned int)buf_size);
	err_code = clGetDeviceInfo(default_device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, sizeof(buf_uint), &buf_uint, NULL);
	checkErr(err_code,"clGetDeviceInfo:DevicePreferredVectorWidthChar");
	printf("  DEVICE_PREFERRED_VECTOR_WIDTH_CHAR = %u\n", (unsigned int)buf_uint);
	err_code = clGetDeviceInfo(default_device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(buf_uint), &buf_uint, NULL);
	checkErr(err_code,"clGetDeviceInfo:DeviceMaxClockFrequency");
	printf("  DEVICE_MAX_CLOCK_FREQUENCY = %u\n", (unsigned int)buf_uint);
	err_code = clGetDeviceInfo(default_device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(buf_ulong), &buf_ulong, NULL);
	checkErr(err_code,"clGetDeviceInfo:DeviceGlobalMemory");
	printf("  DEVICE_GLOBAL_MEM_SIZE = %llu\n", (unsigned long long)buf_ulong);
	err_code = clGetDeviceInfo(default_device, CL_DEVICE_MAX_CONSTANT_ARGS, sizeof(buf_uint), &buf_uint, NULL);
	checkErr(err_code,"clGetDeviceInfo:DeviceMaxConstantArgs");
	printf("  DEVICE_MAX_CONSTANT_ARGS = %u\n", (unsigned int)buf_uint);
	err_code = clGetDeviceInfo(default_device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(buf_ulong), &buf_ulong, NULL);
	checkErr(err_code,"clGetDeviceInfo:DeviceMaxConstantBufferSize");
	printf("  DEVICE_MAX_CONSTANT_BUFFER_SIZE = %llu\n", (unsigned long long)buf_ulong);
	err_code = clGetDeviceInfo(default_device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(buf_ulong), &buf_ulong, NULL);
	checkErr(err_code,"clGetDeviceInfo:DeviceLocalMemorySize");
	printf("  DEVICE_LOCAL_MEM_SIZE = %llu\n\n", (unsigned long long)buf_ulong);
}
