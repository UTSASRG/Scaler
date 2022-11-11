
#include <cstdint>
#include <cstddef>
#include "ExtSymInfo.h"
#include <signal.h>
extern "C" {
#include <xed/xed-interface.h>
}

#ifndef SCALER_BREAKPOINT_H
#define SCALER_BREAKPOINT_H

namespace scaler {
#define OPERAND_NUMBER 3


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


    typedef void (*callback_sa_sigaction)(int, siginfo_t *, void *);

    class Breakpoint {
    public:
        uint8_t *addr = 0;
        // now it support one thread,
        // we can change it to thread mask later
        uint8_t oriCode[15]{};
        int instLen = 0;
        xed_iclass_enum_t xiclass;
        FuncID funcID = 0; //funcID from scaler
        FileID fileID = -1;
        InstrOp operands[OPERAND_NUMBER];
        xed_decoded_inst_t xedDecodedInst;

        Breakpoint() = default;

        Breakpoint(const Breakpoint &);

        Breakpoint(const Breakpoint &&) noexcept;

        bool operator==(const Breakpoint &);

        Breakpoint &operator=(const Breakpoint &);

        Breakpoint &operator=(const Breakpoint &&) noexcept;

    };


}
#endif

