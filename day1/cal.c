#include<stdio.h>
void add(int a, int b)
{
    printf("%d\n",a+b);
}
void sub(int a, int b)
{
    printf("%d\n",a-b);
}
void mul(int a, int b)
{
    printf("%d\n",a*b);
}
void division(int a, int b)
{
    printf("%d\n",a/b);
}
void callback_fun(int x,int y, void(*fptr)(int, int))
{
    fptr(x,y);
}
int main()
{
    int x=24;
    int y=12;
    callback_fun(x,y,add);
    callback_fun(x,y,sub);
    callback_fun(x,y,mul);
    callback_fun(x,y,division);
}

