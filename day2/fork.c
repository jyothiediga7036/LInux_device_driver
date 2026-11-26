#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
int main()
{
	printf("program started\n");
	pid_t pid;
	pid=fork();
	printf("child =%d\n parent =%d\n",getpid(),getppid()); 
	if(pid==0)
	{
		printf("child process\n");
		//execl("/bin/ls","ls",NULL);
		/*if(execl("/usr/bin/date","date",NULL)==-1){
			printf("execl funtion fails\n");
		}
	*/
		execl("/bin/cat","cat","/home/mirafra/linux/exec.c",NULL);
		printf("child finish\n");
	}
	else
	{
		printf("parent process\n");
		wait(NULL);
	}
	printf("program finish\n");
}
