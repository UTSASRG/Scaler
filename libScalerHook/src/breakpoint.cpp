#include <type/breakpoint.h>

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
