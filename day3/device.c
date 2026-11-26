
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int fd;
    char buffer[1024];
    int bytes;

    if (argc != 2)
    {
        printf("Usage: %s \"string\"\n", argv[0]);
        return 1;
    }

    fd = open("/dev/revchar", O_RDWR,0666);

    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    if (write(fd, argv[1], strlen(argv[1])) < 0)
    {
        perror("write");
        close(fd);
        return 1;
    }
    bytes = read(fd, buffer, sizeof(buffer) - 1);

    if (bytes < 0)
    {
        perror("read");
        close(fd);
        return 1;
    }
    buffer[bytes] = '\0';
    printf("Reversed string: %s\n", buffer);

    close(fd);

    return 0;
}










/*#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
int main()
{
	int fd=open("/dev/mychardev",O_RDWR,0666);
	if(fd<0)
	{
		perror("open");
		return 0;
	}
	char buffer[100]="learning device drivers";
	write(fd,&buffer,sizeof(buffer));
	lseek(fd,0,SEEK_SET);
	char str[100];
	read(fd,&str,sizeof(str));
	
	printf("recived :%s\n",str);
	close(fd);
}*/


#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
struct calculator

int main()
{
	int fd=open("/dev/mychardev",O_RDWR,0666);
	if(fd<0)
	{
		perror("open");
		return 0;
	}
	char buffer[100]="learning device drivers";
	write(fd,&buffer,sizeof(buffer));
	lseek(fd,0,SEEK_SET);
	char str[100];
	read(fd,&str,sizeof(str));
	
	printf("recived :%s\n",str);
	close(fd);
}
	
	
