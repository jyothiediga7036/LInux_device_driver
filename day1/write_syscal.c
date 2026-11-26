/*#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
int main()
{
	char s='A';
	int fd = open("read.c", O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd == -1)
    	{
        	perror("open");
        	return 1;
    	}
	write(fd,&s,sizeof(s));
	
    	lseek(fd, 0, SEEK_SET);
	char c;
	read(fd,&c,sizeof(c));
	printf("after =%c\n",c);
	
	close(fd);
	
	
	
}*/

#include <stdio.h>
#include <unistd.h>

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        printf("Child Process\n");
    }
    else
    {
        printf("Parent Process\n");
    }

    return 0;
}




