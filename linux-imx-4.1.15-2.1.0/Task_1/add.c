#include<stdio.h>
#include<stdlib.h>
#include<time.h>
void add(void)
{
    srand((unsigned)time(NULL));
    int a = rand() % 100+1;
    int b = rand() % 100+1;
    printf("a + b = %d\n", a+b);
}