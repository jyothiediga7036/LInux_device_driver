#include<stdio.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<string.h>
#include"struct_string.h"

#define LED_ON  _IOWR('V', 1, int)
#define LED_OFF _IOWR('V', 2, int)
#define GET_LED  _IOWR('V', 3, int)
#define CALC_DIV  _IOWR('V', 4,int)


