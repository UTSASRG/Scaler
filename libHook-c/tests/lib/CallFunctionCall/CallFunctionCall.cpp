/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <CallFunctionCall.h>
#include <FuncWithDiffParms.h>
#include <TenThousandFunc.h>

extern "C" {

void callFuncA() {
    printf("Inside callFuncA\n");
    funcA();
}

void callFunc1000() {
    printf("Inside callFunc1000\n");
    func1000();
}

void callSleepDelay(long long milliseconds) {
    printf("callSleepDelay\n");
    sleepDelay(milliseconds);
}

void callLoopDelay(long long times) {
    printf("callLoopDelay\n");
    loopDelay(times);
}


}