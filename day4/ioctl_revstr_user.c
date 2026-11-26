#include<stdio.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<string.h>
#include"struct_string.h"

#define STRING_REV _IOWR('J',1,struct string_data)

int main()
{
	struct string_data data;
	int fd=open("/dev/ioctlrevdev",O_RDWR,0666);
	if(fd<0)
	{
		perror("open");
		return 0;
	}
	strcpy(data.str, "device drivers");
	ioctl(fd,STRING_REV,&data);
	printf("after revarse =%s\n",data.str);
}
	
