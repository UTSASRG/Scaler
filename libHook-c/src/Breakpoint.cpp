#include <type/Breakpoint.h>
#include <cstring>

scaler::Breakpoint::Breakpoint(const scaler::Breakpoint &rho) {
    operator=(rho);
}

scaler::Breakpoint::Breakpoint(const scaler::Breakpoint &&rho) noexcept {
    operator=(rho);
}

scaler::Breakpoint &scaler::Breakpoint::operator=(const scaler::Breakpoint &rho) {
    if (this != &rho) {
        this->instLen = rho.instLen;
        this->addr = rho.addr;
        this->funcID = rho.funcID;
        this->fileID = rho.fileID;
        memcpy(this->oriCode, rho.oriCode, rho.instLen);
        this->xedDecodedInst = rho.xedDecodedInst;
        this->xiclass = rho.xiclass;
        memcpy(this->operands, rho.operands, sizeof(InstrOp) * OPERAND_NUMBER);

    }
    return *this;

}

scaler::Breakpoint &scaler::Breakpoint::operator=(const scaler::Breakpoint &&rho) noexcept {
    operator=(rho);
    return *this;
}

bool scaler::Breakpoint::operator==(const scaler::Breakpoint &rho) {
    return this->addr == rho.addr;
}