/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <cstdio>
#include <thread>
//=========================================================================
// Thread Functions
//=========================================================================

inline int funcInline(int &num) {
    std::this_thread::sleep_for (std::chrono::milliseconds(1));
    return num += 15;
}

void funcNormal(int num) {
    num+=1;
    funcInline(num);
    std::this_thread::sleep_for (std::chrono::milliseconds(1));
}

int main() {

    for (int i = 0; i < 999; ++i) {
        if (i % 10) {
            funcNormal(i);
        }
    }
    return 0;
}

