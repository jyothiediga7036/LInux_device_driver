#include<stdio.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<unistd.h>

#define IOCTL_MAGIC 'A'
#define IOCTL_SET_VALUE _IOWR(IOCTL_MAGIC,1,int)

int main()
{
	int fd;
	int value=20;
	fd=open("/dev/ioctldev",O_RDWR);
	if(fd<0)
	{
		perror("open");
		return 1;
	}
	printf("usser :sending %d to kernel \n",value);
	
	ioctl(fd,IOCTL_SET_VALUE,&value);
	printf("user :go back %d from kernel \n",value);
	
	close(fd);
	return 0;
}
