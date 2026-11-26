#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
int main()
{
	//int x=66;
	char s='A';
	int fd = open("read.c", O_WRONLY | 0644);
	if (fd == -1)
    	{
        	perror("open");
        	return 1;
    	}
	read(fd,&s,sizeof(s));
	//write(fd,&s,sizeof(s));
	close(fd);
}
