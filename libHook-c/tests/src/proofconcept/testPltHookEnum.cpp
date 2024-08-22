/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <plthook.h>
#include <cstdio>

void test_plthook_enum(plthook_t *plthook) {
    unsigned int pos = 0;
    const char *name;
    void **addr;
    int i;

    while (plthook_enum(plthook, &pos, &name, &addr) == 0) {
        printf("   %s\n", name);
    }

}


//int main() {
//    plthook_t *myPltHook;
//    //Find plthook
//    plthook_open(&myPltHook, NULL);
//    test_plthook_enum(myPltHook);
//}