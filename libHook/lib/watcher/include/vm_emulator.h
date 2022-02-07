#if !defined(VM_EMULATOR_H)
#define VM_EMULATOR_H
/*
 * =====================================================================================
 *
 *       Filename:  vm_emulator.h
 *
 *    Description:  
 *
 *         Author: Hongyu Liu 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>

#include "xdefines.hh"
#include "threadstruct.hh"
#include "software_breakpoint.h"

#define GET_REG_VALUE(ctxt, reg) (((ucontext_t *)ctxt)->uc_mcontext.gregs[reg])
#define SET_REG_VALUE(ctxt, reg, new_value) (((ucontext_t *)ctxt)->uc_mcontext.gregs[reg] = new_value)

#ifdef __cplusplus
extern "C" {
#endif

#include "xed/xed-interface.h"

#ifdef __cplusplus
};
#endif

static xed_state_t vm_xed_machine_state = { 
  XED_MACHINE_MODE_LONG_64, 
  XED_ADDRESS_WIDTH_64b 
};

typedef enum {
  OPERAND_TYPE_NONE,
  OPERAND_TYPE_IMM,
  OPERAND_TYPE_ADDRESS,
  OPERAND_TYPE_MEMORY,
  OPERAND_TYPE_REGISTER
} operand_type;

typedef struct operand_i {
  operand_type type;  
  int name;  
  int size;  
  size_t value;  
} operand_t;

class VmEmulator {

  public:
    static VmEmulator& getInstance() {
      static char vmbuf[sizeof(VmEmulator)];
      static VmEmulator* theOneObject = new (vmbuf) VmEmulator();
      return *theOneObject;
    }

    void* ExecuteInstruction(breakpoint_t* bp, void* context);

    bool GetInstructionInformation(breakpoint_t* bp);

    void ParseOperands(xed_decoded_inst_t *xptr, xed_uint64_t rel_address, operand_t* operands, int real_size, void* context);
 
  private:
    VmEmulator() {
      InitRegMapping(); 
      //xed_tables_init();
    }

    size_t GetOperandValue(operand_t* operand);

    void SetValueToMemory(void* address, int size, size_t value);

    size_t GetRegisterMask(operand_t* operand);

    bool Is32BitRegister(operand_t* operand);

    bool check_operand_visibility(const xed_operand_t* op, xed_operand_enum_t op_name, xed_decoded_inst_t *xptr);

    greg_t reg_mapping[XED_REG_LAST];
    void InitRegMapping(){

      reg_mapping[XED_REG_R8W] = REG_R8;
      reg_mapping[XED_REG_R9W] = REG_R9;
      reg_mapping[XED_REG_R10W] = REG_R10;
      reg_mapping[XED_REG_R11W] = REG_R11;
      reg_mapping[XED_REG_R12W] = REG_R12;
      reg_mapping[XED_REG_R13W] = REG_R13;
      reg_mapping[XED_REG_R14W] = REG_R14;
      reg_mapping[XED_REG_R15W] = REG_R15;
      reg_mapping[XED_REG_DI] = REG_RDI;
      reg_mapping[XED_REG_SI] = REG_RSI;
      reg_mapping[XED_REG_BP] = REG_RBP;
      reg_mapping[XED_REG_BX] = REG_RBX;
      reg_mapping[XED_REG_DX] = REG_RDX;
      reg_mapping[XED_REG_AX] = REG_RAX;
      reg_mapping[XED_REG_CX] = REG_RCX;
      reg_mapping[XED_REG_SP] = REG_RSP;

      reg_mapping[XED_REG_R8D] = REG_R8;
      reg_mapping[XED_REG_R9D] = REG_R9;
      reg_mapping[XED_REG_R10D] = REG_R10;
      reg_mapping[XED_REG_R11D] = REG_R11;
      reg_mapping[XED_REG_R12D] = REG_R12;
      reg_mapping[XED_REG_R13D] = REG_R13;
      reg_mapping[XED_REG_R14D] = REG_R14;
      reg_mapping[XED_REG_R15D] = REG_R15;
      reg_mapping[XED_REG_EDI] = REG_RDI;
      reg_mapping[XED_REG_ESI] = REG_RSI;
      reg_mapping[XED_REG_EBP] = REG_RBP;
      reg_mapping[XED_REG_EBX] = REG_RBX;
      reg_mapping[XED_REG_EDX] = REG_RDX;
      reg_mapping[XED_REG_EAX] = REG_RAX;
      reg_mapping[XED_REG_ECX] = REG_RCX;
      reg_mapping[XED_REG_ESP] = REG_RSP;

      reg_mapping[XED_REG_R8] = REG_R8;
      reg_mapping[XED_REG_R9] = REG_R9;
      reg_mapping[XED_REG_R10] = REG_R10;
      reg_mapping[XED_REG_R11] = REG_R11;
      reg_mapping[XED_REG_R12] = REG_R12;
      reg_mapping[XED_REG_R13] = REG_R13;
      reg_mapping[XED_REG_R14] = REG_R14;
      reg_mapping[XED_REG_R15] = REG_R15;
      reg_mapping[XED_REG_RDI] = REG_RDI;
      reg_mapping[XED_REG_RSI] = REG_RSI;
      reg_mapping[XED_REG_RBP] = REG_RBP;
      reg_mapping[XED_REG_RBX] = REG_RBX;
      reg_mapping[XED_REG_RDX] = REG_RDX;
      reg_mapping[XED_REG_RAX] = REG_RAX;
      reg_mapping[XED_REG_RCX] = REG_RCX;
      reg_mapping[XED_REG_RSP] = REG_RSP;
      reg_mapping[XED_REG_RIP] = REG_RIP;

      reg_mapping[XED_REG_R8B] = REG_R8;
      reg_mapping[XED_REG_R9B] = REG_R9;
      reg_mapping[XED_REG_R10B] = REG_R10;
      reg_mapping[XED_REG_R11B] = REG_R11;
      reg_mapping[XED_REG_R12B] = REG_R12;
      reg_mapping[XED_REG_R13B] = REG_R13;
      reg_mapping[XED_REG_R14B] = REG_R14;
      reg_mapping[XED_REG_R15B] = REG_R15;
      reg_mapping[XED_REG_DIL] = REG_RDI;
      reg_mapping[XED_REG_SIL] = REG_RSI;
      reg_mapping[XED_REG_BPL] = REG_RBP;
      reg_mapping[XED_REG_BL] = REG_RBX;
      reg_mapping[XED_REG_DL] = REG_RDX;
      reg_mapping[XED_REG_AL] = REG_RAX;
      reg_mapping[XED_REG_CL] = REG_RCX;
      reg_mapping[XED_REG_SPL] = REG_RSP;

      /*
       *  reg_mapping[XED_REG_EFL] = REG_EFL;
       *  reg_mapping[XED_REG_CSGSFS] = REG_CSGSFS;
       *  reg_mapping[XED_REG_ERR] = REG_ERR;
       *  reg_mapping[XED_REG_TRAPNO] = REG_TRAPNO;
       *  reg_mapping[XED_REG_OLDMASK] = REG_OLDMASK;
       *  reg_mapping[XED_REG_CR2] = REG_CR2;
       */
    }
};

#endif
