/**
 * This demo is used to observe stack behavior when using different stacks
 * @return
 */
int funcD(int a, int b, int c) {
    return 0xff;
}

void funcC(int a, int b, int c) {
    funcD(a, b, c);
}

void funcB(int a, int b) {
    funcC(a, b, 3);
    return;
}

void funcA() {
    funcB(1, 2);
    return;
}


int main() {
//    unsigned char out;
//    asm volatile("movb %%fs:0, %[Var]" : [Var] "=r" (out));
    funcA();
}
