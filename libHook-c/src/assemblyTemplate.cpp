/*
 * This file resides template for assembly code. It can also be used to check discrepancies between different machine in
 * debug mode.
 */

void __attribute__((used, naked)) redzoneJumper() {
    //todo: Calculate values based on rsp rathe than saving them to registers
    __asm__ __volatile__ (
    "subq $128,%rsp\n\t" //Skip redzone
    "jmp asmHookHandler\n\t"
    );

}