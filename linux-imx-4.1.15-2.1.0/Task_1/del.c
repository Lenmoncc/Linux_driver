#include<stdio.h>
#include<stdlib.h>
#include<time.h>
void del(void)
{
    srand((unsigned)time(NULL));
    int a = rand()%100+1;
    int b = rand()%100+1;
    if(a-b<0){
        printf("a - b = -%d\n", b-a);
        return;
    }
    else
    printf("a - b = %d\n", a-b);
}