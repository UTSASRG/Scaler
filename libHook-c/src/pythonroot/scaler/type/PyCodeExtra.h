#ifndef PYCODEEXTRA_H
#define PYCODEEXTRA_H
#include <type/ExtSymInfo.h>
#include <util/datastructure/Array.h>

namespace scaler{
    class PyCodeExtra{
    public:
        Array<FuncID> pyModuleRecArrMap;//Map python module to fileId in the recording array
        FileID pyModuleFileId=0;
    };

    void freePyCodeExtra(void* obj){

    }
}
#endif