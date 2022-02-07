#if !defined(RCA_DISAS_H)
#define RCA_DISAS_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <elf.h>

#include "xdefines.hh"
#include "log.hh"
#include "xthread.hh"
#include "perthread_decoder.h"
#include "instbuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xed/xed-interface.h"

#ifdef __cplusplus
};
#endif

#define GET_REG(ctxt, reg) (ctxt->regs[reg])
#define NO_SIGN_EXTEND 0

static xed_state_t xed_machine_state = { 
  XED_MACHINE_MODE_LONG_64, 
  XED_ADDRESS_WIDTH_64b 
};

class RCADisas {

  public:
    
    static RCADisas& getInstance() {
      static char buf[sizeof(RCADisas)];
      static RCADisas* theOneTrueObject = new (buf) RCADisas();
      return *theOneTrueObject;
    }

    xed_iclass_enum_t getInstructionType(void* inst);

    xed_error_enum_t decode_inst(const xed_uint8_t* addr, int ilim, int* ilength);

    pid_t getCurrentTid(trackctl* ctl);

    void get_suspicious_reg(trackctl* ctl);

    void get_suspicious_reg_for_div(trackctl* ctl);

    void get_phony_reg(trackctl* ctl);

    xed_reg_enum_t get_memoryassignment_reg(void* addr);
      
    void findAbortCondition(trackctl* ctl, PerThreadDecoder* decoder, void* callerinst);

    void* getBranchAddress(void* address, xed_decoded_inst_t* xptr, 
        xed_operand_enum_t op_name, const xed_operand_t* op, const xed_operand_values_t* ov);

    void* getPrevInstruction(void* current_ip);

    void getMemoryAddressByPrevInstruction(void* addr, void* context, MemAccess* mem);

    void initTracking(trackctl* ctl, PerThreadDecoder* decoder);

    void trackFaultyAddress(trackctl* ctl, PerThreadDecoder* decoder);

    void checkAssignInst(trackctl* ctl, void* inst);

    void decodeTargetAddr(trackctl* ctl, void* inst, xed_decoded_inst_t* xptr, RegStatus reg_stat, xed_reg_enum_t reg,
        xed_operand_enum_t op_name, const xed_operand_t* op, const xed_operand_values_t* ov);

    void removeSpeculativeInst(trackctl* ctl);

    void computeFaultyAddresses(trackctl* ctl);

    void* computeTargetAddress(void* inst, xed_decoded_inst_t* xptr, xed_operand_enum_t op_name, 
        const xed_operand_t* op, const xed_operand_values_t* ov, sFrame* frame);

    size_t appendBlockByIP(void** buffer, void* ip);

    size_t checkBranchAndAppend(void** buffer, void* prev_inst, bool istaken);

    void decodeComparisonInst(trackctl* ctl, void* address);
      
    void saveMemoryAssignment(trackctl* ctl, siteInfo* parent, pid_t tid, CallStackInfo* saved_callstack);

    bool findAssignedAddr(trackctl* ctl, xed_decoded_inst_t* xptr, const xed_inst_t *xi);
      
    bool isExecutedInstruction(void* inst, sFrame* frame);

    void decode_section(InstBuffer* ib, void* start, void* end, Elf64_Off offset, Elf64_Word size, Elf64_Addr runtime_vaddr);

    void* getInstructionRealAddress(mapping* m, uint64_t rel_address) {
      void* address = (void*)rel_address;
      bool isapp = selfmap::getInstance().isApplication((void*)m->getBase());
      if(!isapp) {
        address = (void*)(m->getBase() + rel_address);
      }
      return address;
    }

    bool isConditionInst(xed_iclass_enum_t xiclass) {
      //fprintf(stderr, "class is %d\n", xiclass);
      return xiclass == XED_ICLASS_TEST
        || xiclass == XED_ICLASS_CMP
        || xiclass == XED_ICLASS_XOR;
    }

  private:
    RCADisas() { 
      xed_tables_init(); // one time per process
      initRegMapping();
    }

    int all_zeros(xed_uint8_t* p, unsigned int len) {
      unsigned int i;
      for( i=0;i<len;i++) 
        if (p[i]) 
          return 0;
      return 1;
    }

    bool check_operand_visibility(const xed_operand_t* op, xed_operand_enum_t op_name, xed_decoded_inst_t *xptr) {
      bool isvisibile = true;
      if (xed_operand_operand_visibility(op) == XED_OPVIS_SUPPRESSED) {
        /*  allow a fall through to print the memop for stringops to match dumpbin */
        xed_bool_t stringop = (xed_decoded_inst_get_category(xptr) == XED_CATEGORY_STRINGOP);
        if (stringop) {
          if (op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1) {
          }else{
            isvisibile = false;
          }
        } else{
          isvisibile = false;
        }
      }

      return isvisibile;
    }

    xed_decoded_inst_t xedd;

    greg_t reg_mapping[XED_REG_LAST];
    void initRegMapping(){

      for(int i=0; i<XED_REG_LAST; i++) {
        reg_mapping[i] = NGREG; 
      }
      
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

      reg_mapping[XED_REG_AH] = REG_RAX;
      reg_mapping[XED_REG_CH] = REG_RCX;
      reg_mapping[XED_REG_DH] = REG_RDX;
      reg_mapping[XED_REG_BH] = REG_RBX;

      /*
         reg_mapping[XED_REG_EFL] = REG_EFL;
         reg_mapping[XED_REG_CSGSFS] = REG_CSGSFS;
         reg_mapping[XED_REG_ERR] = REG_ERR;
         reg_mapping[XED_REG_TRAPNO] = REG_TRAPNO;
         reg_mapping[XED_REG_OLDMASK] = REG_OLDMASK;
         reg_mapping[XED_REG_CR2] = REG_CR2;
         */
    }

};
#endif
