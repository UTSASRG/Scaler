/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <cstdio>
#include <iostream>

using namespace std;
extern "C" {
void funcA() {
    printf("funcAdddddddddddddddddddddd\n");
}

void funcB(int a) {
}

void funcC(int a, int b) {
}

void funcD(int a, int b, int c) {
}

int funcE(int a, int b, int c) {
    int j = a + b + c;
    return j;
}

int funcF(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
    int k = a + b + c + d + e + f + g + h + i + j;
    return k;
}

}

void *funcPtr = (void *) (funcE);


void *method = nullptr;

void test() {
    cout << "test function invoke" << endl;
}

void functionPtrTest() {
    method = {(void *) test};
    //调用的时候，把void*类型的指针强制类型转换成函数指针
    //然后以函数指针调用函数的方式调用这个函数
    //m.method 是函数的入口地址
    (*((void (*)()) (method)))();
}

#include "elf.h"
#include "link.h"
#include <dlfcn.h>

void stackTrace() {
    ElfW(Addr) *rbp;
    asm volatile("mov %%rbp,%0" : "=rm" (rbp));

    ElfW(Addr) *rbp1 = rbp;

    while (*rbp != 0) {
        printf("rbp: %p *rbp: %p *(rbp+1): %p: %p \n", rbp, *rbp, *(rbp + 1));
        rbp = reinterpret_cast<Elf64_Addr *>(*rbp);
    }

    printf("*rbp1: %p\n", *rbp1);
    printf("*(rbp1+1): %p\n", *(rbp1 + 1));
    printf("*(rbp1+2): %p\n", *(rbp1 + 2));
    printf("*(rbp1+3): %p\n", *(rbp1 + 3));
    printf("*(rbp1+4): %p\n", *(rbp1 + 4));
    printf("*(rbp1+5): %p\n", *(rbp1 + 5));

}

void call1() {
    stackTrace();
}

void call2() {
    call1();
}

void call3() {
    call2();
}

void call4() {
    call3();
}

void call5() {
    call4();
}


int main() {
    funcA();
    funcB(1);
    funcC(1, 2);
    funcD(1, 2, 3);
    funcE(1, 2, 3);
    funcF(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    functionPtrTest();

    printf("Call5: %p\n", call5);
    printf("Call4: %p\n", call4);
    printf("Call3: %p\n", call3);
    printf("Call2: %p\n", call2);
    printf("Call1: %p\n", call1);
    printf("Stacktrace: %p\n", stackTrace);

    call5();
    return 0;
}