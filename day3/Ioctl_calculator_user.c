#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include"common.h"

#define CALC_ADD  _IOWR('C', 1, struct calculator)
#define CALC_SUB  _IOWR('C', 2, struct calculator)
#define CALC_MUL  _IOWR('C', 3, struct calculator)
#define CALC_DIV  _IOWR('C', 4, struct calculator)

int main()
{
	struct calculator data;
	int fd;
	fd=open("/dev/ioctlcaldev",O_RDWR,0666);
	if(fd<0)
	{
		perror("open");
		return 0;
	}
	
	data.a=10;
	data.b=20;
	data.op='*';
	ioctl(fd,CALC_ADD,&data);
	printf("result =%d\n",data.result);
	close(fd);
}
	

