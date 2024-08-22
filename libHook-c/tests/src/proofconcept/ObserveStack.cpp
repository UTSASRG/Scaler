/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
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
