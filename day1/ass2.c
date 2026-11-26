#include<stdio.h>
struct Packet
{
    unsigned ready : 1;
    unsigned error : 1;
    unsigned cmd   : 6;

    void (*handler)(void *);
};
void fun(struct Packet *q)
{
	printf("function called\n");
}
int main()
{
	int b=5;
	struct Packet packet;
	struct Packet *p=&packet;
	p->ready=b&1;
	p->error=(b>>1) &1; 
	p->cmd=(b>>2)&0x3f;
	if(p->ready && !p->error)
	{
		p->handler=fun;
		p->handler(p);
	}
	else
	{
		return 0;
	}		
}
