/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <sys/mman.h>
#include <cstdio>

int main() {
    //calling break point
    static unsigned char c[] = {0xcc, 0xc3, 0x12, 0x34, 0x45};
    static void (*pfunc)() =(void (*)()) c;
    static int i = mprotect(reinterpret_cast<void *>((unsigned long int) c & 0xfffffffffffff000), sizeof(c), PROT_EXEC | PROT_READ | PROT_WRITE);
    fprintf(stderr, "Call pfunc"); //put some printf , so that this function is not inlined
    pfunc();
    fprintf(stderr, "Exit pFunc"); //put some printf , so that this function is not inlined
    return 0;
}