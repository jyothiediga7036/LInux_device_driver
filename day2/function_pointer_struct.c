#include<stdio.h>
#include <fcntl.h>
#include <unistd.h>
typedef struct student_details
{
	int roll;
	char name[20];
	float marks;
}student;
int my_open(char *path, int flag, mode_t m)
{
	return open(path,flag,m);
}
int my_write(int f, void*buf, int n)
{
	return write(f,buf,n);
}
int my_read(int f, void*buf, int n)
{
	return read(f, buf, n);
}
int my_close(int f)
{
	return close(f);
}
typedef struct function_pointer
{
	int (*open_sys)(char*,int,mode_t);
	int (*close_sys)(int);
	int (*write_sys)(int, void*,int);
	int (*read_sys)(int, void*,int);
}systemcalls;

systemcalls call={
	.open_sys=my_open,
	.close_sys=my_close,
	.write_sys=my_write,
	.read_sys=my_read,
};


int main()
{
	student s1={12,"jyothi",98.3};
	student s2;
	
	int fd = call.open_sys("student.txt", O_CREAT | O_RDWR | O_TRUNC, 0664);
	if(fd==-1)
	{
		perror("open");
		return 0;
	}
	int n=call.write_sys(fd,&s1,sizeof(s1));
	printf("write characters =%d\n",n);
	   lseek(fd, 0, SEEK_SET);
	n=call.read_sys(fd,&s2,sizeof(s2));
	printf("read character =%d\n",n);
	printf("%d\n %s\n%f\n",s2.roll,s2.name,s2.marks);
	
	call.close_sys(fd);
}



















/*typedef struct file_open
{

 	FILE* (*open_file)(const char *, const char*);
	int (*write_file)(void*,size_t, size_t, FILE*);
 	int (*read_file)(void *,size_t,size_t,FILE*);
 	int (close_file)(FILE*);
}files;
typedef struct student
{
	int roll;
	char name[20];
	float marks;
}details;
files={.open_file=fopen,.write=fwrite,.read_file=fread, .close_file=fclose}

int main()
{
	details d1={12,"jyothi",98.6}
	details d2;
	
}
*/



















/*#include<stdio.h>
typedef void (*fptr)(int, int);
typedef void (*fptr1)(int, int);
struct calculator
{
	fptr op_add;
	fptr1 op_sub;
	
};
void add(int a, int b)
{
	printf("add function\n");	
}
void sub(int a, int b)
{
	printf("sub function \n");
}
void callback_fun(void (*fp)(int,int),int a ,int b)
{
	fp(a,b);
}
int main()
{
	struct calculator cal;
	cal.op_add=add;
	cal.op_sub=sub;
	callback_fun(cal.op_add, 3,4);
	callback_fun(cal.op_sub,12,4);
	
		
}*/

