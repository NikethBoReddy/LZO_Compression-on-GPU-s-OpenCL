/* Author: Niketh Boreddy
   email: nikethcse@gmail.com
   
  Date: 10th June 2016
  The follwing file is a part of the LZO algortihm implementation for GPU's
*/

#include <CL/cl.h>

inline void checkErr(cl_int err, const char * name);

void Display_PlatformInfo(cl_platform_id default_platform, char *buffer);
void Display_DeviceInfo(cl_device_id default_device, char *buffer);
