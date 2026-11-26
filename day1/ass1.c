#include<stdio.h>
enum symbol {'&','|'}
void op_and(int a, int b)
{
	printf("%d\n",a&b);
}
void op_or(int a, int b)
{
	printf("%d\n",a|b);
}
void op_xor(int a, int b)
{
	printf("%d\n",a^b);
}
void op_lshift(int a, int b)
{
	printf("%d\n",a<<b);
}
typedef void (*fptr)(int,int);
fptr table[]=
{
	op_and;
	op_or;
	op_xor;
	op_lshift;
};
void callback_fun(int a, int b, void (*p)(int,int))
{
	p(a,b);
}
void execute(int a, int b, char op)
{
	symbol index=op;
//table[(int)index];
	callback_fun(a,b,table[(int)index]);
}

int main()
{
	int x=10;
	int y=2;
	execute(x,y,'&');
	////callback_fun(x,y,table[1]);
	//callback_fun(x,y,table[1]);
}
