extern "C" {
#include "xed/xed-interface.h"
}

#include <type/breakpoint.h>
#include <ucontext.h>

#ifndef SCALER_VMEMULATOR_H
#define SCALER_VMEMULATOR_H


namespace scaler {
#define GET_REG_VALUE(ctxt, reg) (((ucontext_t *)ctxt)->uc_mcontext.gregs[reg])
#define SET_REG_VALUE(ctxt, reg, new_value) (((ucontext_t *)ctxt)->uc_mcontext.gregs[reg] = new_value)
#define OPERAND_NUMBER 3

    typedef void (*callback_sa_sigaction)(int, siginfo_t *, void *);


    class VMEmulator {
    public:
        static VMEmulator &getInstance() {
            static char vmbuf[sizeof(VMEmulator)];
            static VMEmulator *theOneObject = new(vmbuf) VMEmulator();
            return *theOneObject;
        }

        void *executeInstr(scaler::Breakpoint &bp, ucontext_t *context);

        void getInstrInfo(Breakpoint &bp);

        void parseOp(xed_decoded_inst_t &decodedInst, xed_uint64_t rel_address, InstrOp *operands, int real_size,
                     void *context);

        size_t getOp(InstrOp &op);

    private:
        VMEmulator();

        void setMemory(void *address, int size, ssize_t value);

        size_t getRegMask(InstrOp &op);

        bool is32BitReg(InstrOp &op);

        bool chkOpVisibility(const xed_operand_t *op, xed_operand_enum_t opName, xed_decoded_inst_t *xptr);

        greg_t regMapping[XED_REG_LAST];

        void initRegMapping();
    };

}

#endif //SCALER_VMEMULATOR_H
