#include <type/ExtSymInfo.h>

bool scaler::ExtSymInfo::operator==(const scaler::ExtSymInfo &rho) const {
    if (&rho != this) {
        return fileId == rho.fileId && symIdInFile == rho.symIdInFile;
    } else {
        return true;
    }
}
