/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
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