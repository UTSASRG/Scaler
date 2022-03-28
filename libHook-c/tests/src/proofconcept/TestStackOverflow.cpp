#include <stdio.h>
#include <stdlib.h>
#include <cstring>

void *__stack_chk_guard = (void *)0xdeadbeef;

void __stack_chk_fail(void)
{
    while(true){
        continue;
    }
    fprintf(stderr, "Stack smaswefwefhing detected.\n");
//    fprintf(stderr, "Stack smashing detected.\n");
//    fprintf(stderr, "Stack smashing detected.\n");

    exit(1);
}

void get_input(char *data)
{
    strcpy(data, "01234567012345670123");
}

int main(void)
{
    char buffer[8];
    get_input(buffer);
    return buffer[0];
}