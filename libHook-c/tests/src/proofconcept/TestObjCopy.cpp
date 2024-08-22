/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <stdio.h>
struct ObjData{
    int parm1=-1;
    int parm2=-1;
};

class A{
    ObjData objData;
    int parm1=1;
    int parm2=1;

public:
    A(){
        objData.parm1=-1;
        objData.parm1=-1;
        objData.parm2=-1;
    }

    A(int testP){
        objData.parm1=testP;
        objData.parm2=testP;
        parm1=testP;
        parm2=testP;
    }
};



int main ()
{
    A a;
    a=A(2);
    printf("Hello world!\n");
    return 0;
}