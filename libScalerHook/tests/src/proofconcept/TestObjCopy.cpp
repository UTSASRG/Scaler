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