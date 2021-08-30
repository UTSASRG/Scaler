extern "C" {
    #include "xed/xed-interface.h"
}

#include <type/breakpoint.h>
#include <ucontext.h>

#ifndef SCALER_VMEMULATOR_H
#define SCALER_VMEMULATOR_H


namespace scaler {

    class InstrOp {
    public:
        enum Type {
            OPERAND_TYPE_NONE,
            OPERAND_TYPE_IMM,
            OPERAND_TYPE_ADDRESS,
            OPERAND_TYPE_MEMORY,
            OPERAND_TYPE_REGISTER
        };

        Type instrType;
        int name;
        int size;
        size_t value;
    };

    class VMEmulator {
    public:
        static VMEmulator &getInstance() {
            static char vmbuf[sizeof(VMEmulator)];
            static VMEmulator *theOneObject = new(vmbuf) VMEmulator();
            return *theOneObject;
        }

        void *executeInstr(scaler::Breakpoint &bp, ucontext_t *context);

        void getInstrInfo(Breakpoint &bp);

        void parseOp(xed_decoded_inst_t *xptr, xed_uint64_t rel_address, InstrOp *operands, int real_size,
                     void *context);

    private:
        VMEmulator();

        size_t getOp(InstrOp *op);

        void setMemory(void *address, int size, size_t value);

        size_t getRegMask(InstrOp *op);

        bool is32BitReg(InstrOp *op);

        bool chkOpVisibility(const xed_operand_t *op, xed_operand_enum_t opName, xed_decoded_inst_t *xptr);

        greg_t regMapping[XED_REG_LAST];

        void initRegMapping();
    };

}

#endif //SCALER_VMEMULATOR_H
