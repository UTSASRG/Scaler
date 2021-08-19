
#include "rca_disas.hh"

#include "xmemory.hh"
#include "log.hh"

/* 
 * TODO
 * 1. support "rep movsb %ds:(%rsi),%es:(%rdi)"
 * */

inline bool isDataAddress(void* addr) { return xmemory::getInstance().inRange((intptr_t)addr) || selfmap::getInstance().isGlobalAddress(addr); } 

inline bool compareRegister(xed_reg_enum_t sreg, xed_reg_enum_t treg) { 
  if(sreg == treg) return true;

  if(XED_REG_AX <= sreg && sreg <= XED_REG_R15B
      && XED_REG_AX <= treg && treg <= XED_REG_R15B) {
    if(sreg > treg) {
      return (sreg - treg) % 16 == 0;
    } else {
      return (treg - sreg) % 16 == 0;
    }
  }
  return false;
}

inline xed_reg_enum_t convertToNorm(xed_reg_enum_t reg) { 
  xed_reg_enum_t norm = reg;
  if(XED_REG_AX <= reg && reg <= XED_REG_BH) {
    switch(reg) {
      case XED_REG_AH: 
        {
          norm = XED_REG_AL;
          break;
        }
      case XED_REG_CH:
        {
          norm = XED_REG_CL;
          break;
        }
      case XED_REG_DH:
        {
          norm = XED_REG_DL;
          break;
        }
      case XED_REG_BH:
        {
          norm = XED_REG_BL;
          break;
        }
      default: 
        {
          norm = (xed_reg_enum_t)(XED_REG_R15B - (abs(XED_REG_R15B - reg) % 16)); 
        }
    }
  }
  return norm; 
}

inline RegStatus compareRegisters(trackctl* ctl, xed_reg_enum_t reg) { 
  xed_reg_enum_t norm = reg;
  if(XED_REG_AX <= reg && reg <= XED_REG_BH) {
    norm = convertToNorm(reg);
  }
  return ctl->current_regs.isMatch(norm);
}

pid_t RCADisas::getCurrentTid(trackctl* ctl) {
    pid_t tid;
    if(ctl->trace.cur==NULL) {
      tid = syscall(SYS_gettid);
    } else {
      tid = ctl->trace.cur->tid;
    }
  return tid;
}

void RCADisas::decode_section(InstBuffer* ib, void* start, void* end, Elf64_Off offset, Elf64_Word size, Elf64_Addr runtime_vaddr) {
  size_t length = (intptr_t)end - (intptr_t)start;
  unsigned char* hard_limit = (unsigned char*)end;

  // start of section
  unsigned char* sStart = (unsigned char*)start + offset;
  if(sStart > hard_limit) {
    RCA_PRDBG("section offset %lu is beyond the end of file\n", offset);
    return;
  }

  // end of section
  unsigned char* sEnd = sStart + size;
  if(sEnd > hard_limit) {
    sEnd = hard_limit;
  }

  unsigned char* z = sStart;
  unsigned int i;

  int skipping = 0;
  int last_all_zeros = 0;
  for(i=0; i<length; i++) {
    if(z >= sEnd) {
      RCA_PRDBG("decode %d instruction\n", i);
      break; 
    }

    int ilim,elim;
    /*  if we get near the end of the section, clip the itext length */
    ilim = XED_MAX_INSTRUCTION_BYTES;
    elim = sEnd - z;
    if (elim < ilim) 
      ilim = elim;

    if (all_zeros((xed_uint8_t*) z, ilim)) 
    {
      if (skipping) {
        z = z + ilim;
        continue;
      }
      else if (last_all_zeros) { 
        z = z + ilim;
        skipping = 1;
        continue;
      }
      else
        last_all_zeros = 1;
    }
    else
    {
      skipping = 0;
      last_all_zeros = 0;
    }

    int ilength = 0;
    xed_error_enum_t xed_error = XED_ERROR_NONE;
    xed_error = decode_inst(XED_REINTERPRET_CAST(const xed_uint8_t*, z), ilim, &ilength);

    if(xed_error == XED_ERROR_NONE) {
      if (ilength == 0){
        // something wrong
        break;  
      }
      
      ib->addNewInst(z, ilength);
    }else{
      // just give a length of 1B to see if we can restart decode...
      ilength = 1;
    } 

    z = z + ilength;
  }
}

xed_error_enum_t RCADisas::decode_inst(const xed_uint8_t* addr, int ilim, int* ilength) {
  //xed_decoded_inst_t xedd;
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error; 
  // init inst_t
  xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);
  // FIXME not handle prefix: REP, LOCK ...
  // decode one instruction
  xed_error = xed_decode(xptr, addr, ilim);
  // if no error
  if(xed_error == XED_ERROR_NONE){
    // get length of instruction
    *ilength = xed_decoded_inst_get_length(xptr);
  }
  return xed_error;
}

xed_iclass_enum_t RCADisas::getInstructionType(void* inst) {
  //xed_decoded_inst_t xedd;
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error; 
  xed_iclass_enum_t xiclass = XED_ICLASS_INVALID;
  // init inst_t
  xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);
  // FIXME not handle prefix: REP, LOCK ...
  // decode one instruction
  xed_error = xed_decode(xptr, XED_REINTERPRET_CAST(const xed_uint8_t*, inst), XED_MAX_INSTRUCTION_BYTES);
  // if no error
  if(xed_error == XED_ERROR_NONE){
    xiclass = xed_decoded_inst_get_iclass(xptr);
  }
  return xiclass;
}

void RCADisas::initTracking(trackctl* ctl, PerThreadDecoder* decoder) {

  if(ctl->type == FAULTY_TYPE_ABORT) {
    ctl->faultyip = getPrevInstruction(ctl->faultyip);
  }
  RCA_PRDBG("ctl->faultyip %p\n", ctl->faultyip);

  void* inst = decoder->pt_prev_instruction();
  // find faulty instruction 
  while(inst!=NULL && inst!=ctl->faultyip) {
    //RCA_PRDBG("[SKIP] addr %p, %lx\n", inst, *(size_t*)inst);
    inst = decoder->pt_prev_instruction();
  }

  // abort is a system call, we need to check conditions
  if(ctl->type == FAULTY_TYPE_ABORT) {
    int level = ctl->cslevel;
    int i = 0;
    void* callerInst = (void*)(ctl->callstack[i].regs[REG_RIP]);
    while(selfmap::getInstance().isLibcLibrary(callerInst) && i<level) {
      i++;
      callerInst = (void*)(ctl->callstack[i].regs[REG_RIP]);
    }
    ctl->current_level = i;
    callerInst = getPrevInstruction(callerInst);
    RCA_PRINF("Caller instruction is %p\n", callerInst);

    pid_t tid = getCurrentTid(ctl);
    ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
        ctl->trace.cur, NULL, callerInst, 0, SITE_TYPE_ROOT, 
        ctl->cslevel, ctl->current_level, ctl->callstack, 0,
        tid);
    ctl->trace.cur = &ctl->trace.si[ctl->trace.num_site-1];

    findAbortCondition(ctl, decoder, callerInst);
  } else if(ctl->type == FAULTY_TYPE_SEGV) {
    get_suspicious_reg(ctl);
  } else if(ctl->type == FAULTY_TYPE_FPE) {
    get_suspicious_reg_for_div(ctl);
  } else if(ctl->type == FAULTY_TYPE_PHONY) {
    get_phony_reg(ctl); 
  }
}

void RCADisas::get_suspicious_reg(trackctl* ctl) {
  //xed_decoded_inst_t xedd;
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error; 

  // init inst_t
  xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);

  // FIXME not handle prefix: REP, LOCK ...
  // decode one instruction
  xed_error = xed_decode(xptr, (uint8_t *)ctl->faultyip, XED_MAX_INSTRUCTION_BYTES);

  // if no error
  if(xed_error == XED_ERROR_NONE){

    pid_t tid = getCurrentTid(ctl);

    xed_iclass_enum_t xiclass = xed_decoded_inst_get_iclass(xptr);
    // This is a stack SEGV. 
    // Because it is a ret instruction, the faulty address is RBP address.
    if(xiclass == XED_ICLASS_RET_FAR || xiclass == XED_ICLASS_RET_NEAR) {
      void* address = (size_t*)(ctl->callstack[0].regs[REG_RSP]) - 1;
      ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
          ctl->trace.cur, address, ctl->faultyip, 0, SITE_TYPE_ROOT, 
          ctl->cslevel, ctl->current_level, ctl->callstack, 0,
          tid);
      ctl->trace.cur = &ctl->trace.si[ctl->trace.num_site-1];
      ctl->site.addSite(ctl->trace.cur);
      RCA_PRINF("Instruction %p is ret\n", address);
    } else {
      // Return the xed_inst_t structure for this instruction. 
      // This is the route to the basic operands form information.
      const xed_inst_t *xi = xed_decoded_inst_inst(xptr);
      // get operands number
      unsigned int noperands = xed_inst_noperands(xi);

      for(int i=0; i<noperands; i++){
        const xed_operand_t* op = xed_inst_operand(xi, i);
        //const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);
        xed_operand_enum_t op_name = xed_operand_name(op);

        if(!check_operand_visibility(op, op_name, xptr)){
          continue;
        }

        int memop = -1;
        if (op_name == XED_OPERAND_MEM0) 
          memop = 0;
        else if (op_name == XED_OPERAND_MEM1) 
          memop = 1;

        if (memop != -1) {

          ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
              ctl->trace.cur, NULL, ctl->faultyip, 0, SITE_TYPE_ROOT, 
              ctl->cslevel, ctl->current_level, ctl->callstack, 0,
              tid);
          ctl->trace.cur = &ctl->trace.si[ctl->trace.num_site-1];

          /*  get reads of base/index regs,  if any */
          xed_reg_enum_t base, index;

          base = xed_decoded_inst_get_base_reg(xptr, memop);
          index = xed_decoded_inst_get_index_reg(xptr, memop);

          if (base != XED_REG_INVALID){
            int reg_enum = reg_mapping[base];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
              ctl->current_regs.addRegister(convertToNorm(base));
              RCA_PRINF("#%d# inst %p, suspicious register is %s\n", ctl->trace.num_site, ctl->faultyip, xed_reg_enum_t2str(base));
            }
          }

          if (index != XED_REG_INVALID) {
            int reg_enum = reg_mapping[index];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
              ctl->current_regs.addRegister(convertToNorm(index));
              RCA_PRINF("#%d# inst %p, suspicious register is %s\n", ctl->trace.num_site, ctl->faultyip, xed_reg_enum_t2str(index));
            }
          }

          break;
        }
      }
    }
  }
}

void RCADisas::get_suspicious_reg_for_div(trackctl* ctl) {
  //xed_decoded_inst_t xedd;
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error; 

  // init inst_t
  xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);

  // FIXME not handle prefix: REP, LOCK ...
  // decode one instruction
  xed_error = xed_decode(xptr, (uint8_t *)ctl->faultyip, XED_MAX_INSTRUCTION_BYTES);

  // if no error
  if(xed_error == XED_ERROR_NONE){

    pid_t tid = getCurrentTid(ctl);
    ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
        ctl->trace.cur, NULL, ctl->faultyip, 0, SITE_TYPE_ROOT, 
        ctl->cslevel, ctl->current_level, ctl->callstack, 0,
        tid);
    ctl->trace.cur = &ctl->trace.si[ctl->trace.num_site-1];

    // Return the xed_inst_t structure for this instruction. This is the route to the basic operands form information.
    const xed_inst_t *xi = xed_decoded_inst_inst(xptr);
    // get operands number
    unsigned int noperands = xed_inst_noperands(xi);

    for(int i=0; i<noperands; i++){
      const xed_operand_t* op = xed_inst_operand(xi, i);
      //const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);
      xed_operand_enum_t op_name = xed_operand_name(op);

      if(!check_operand_visibility(op, op_name, xptr)){
        continue;
      }

      // if the operand is register
      if(xed_operand_is_register(op_name)) {
        xed_reg_enum_t r = xed_decoded_inst_get_reg(xptr, op_name);
        //int reg_enum = reg_mapping[r];
        //if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
          ctl->current_regs.addRegister(convertToNorm(r));
          RCA_PRINF("inst %p, suspicious register is %s\n", ctl->faultyip, xed_reg_enum_t2str(r));
          break;
        //}
      }

      // if it is not a register, it is a memory address
      const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);
      ctl->trace.si[ctl->trace.num_site-1].faultyAddr = computeTargetAddress(ctl->faultyip, xptr, op_name, op, ov, &ctl->callstack[ctl->current_level]);
      ctl->site.addSite(ctl->trace.cur);
      RCA_PRINF("#%d# inst %p, divide by a memory value\n", ctl->trace.num_site, ctl->faultyip);
    }
  }
}

void RCADisas::get_phony_reg(trackctl* ctl) {
  xed_reg_enum_t reg = get_memoryassignment_reg(ctl->faultyip);
  if(reg != XED_REG_INVALID) {
    ctl->current_regs.addRegister(convertToNorm(reg));
  }
}

xed_reg_enum_t RCADisas::get_memoryassignment_reg(void* addr) {
  xed_reg_enum_t reg = XED_REG_INVALID;
  //xed_decoded_inst_t xedd;
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error; 

  // init inst_t
  xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);

  // FIXME not handle prefix: REP, LOCK ...
  // decode one instruction
  xed_error = xed_decode(xptr, (uint8_t *)addr, XED_MAX_INSTRUCTION_BYTES);

  // if no error
  if(xed_error == XED_ERROR_NONE){

    xed_iclass_enum_t xiclass = xed_decoded_inst_get_iclass(xptr);

    // Return the xed_inst_t structure for this instruction. 
    // This is the route to the basic operands form information.
    const xed_inst_t *xi = xed_decoded_inst_inst(xptr);
    // get operands number
    unsigned int noperands = xed_inst_noperands(xi);

    for(int i=0; i<noperands; i++){
      const xed_operand_t* op = xed_inst_operand(xi, i);
      //const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);
      xed_operand_enum_t op_name = xed_operand_name(op);

      if(!check_operand_visibility(op, op_name, xptr)){
        continue;
      }

      if(xed_operand_is_register(op_name) && xed_operand_read(op)) {
        xed_reg_enum_t r = xed_decoded_inst_get_reg(xptr, op_name);
        //int reg_enum = reg_mapping[r];
        //if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
          RCA_PRINF("inst %p, suspicious register is %s\n", addr, xed_reg_enum_t2str(r));
          reg = r;
          break;
        //} 
      }

      if(op_name == XED_OPERAND_IMM0) {
        const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);
        unsigned int real_bits = xed_decoded_inst_operand_element_size_bits(xptr, 1);
        size_t mask = (0xFFFFFFFFFFFFFFFF >> (64-real_bits));
        xed_uint64_t imm = xed_operand_values_get_immediate_uint64(ov);
        imm &= mask;
        PRINT("\nMemory write at %p is %lx\n", addr, imm);
      }

      if(op_name == XED_OPERAND_IMM1) {
        xed_uint64_t imm = xed3_operand_get_uimm1(xptr);
        PRINT("\nMemory write at %p is %lx\n", addr, imm);
      }
    }
  }

  return reg;
}

void RCADisas::findAbortCondition(trackctl* ctl, PerThreadDecoder* decoder, void* callerinst) {
  void* inst = decoder->pt_prev_instruction();
  while(inst!=NULL && inst!=callerinst) {
    inst = decoder->pt_prev_instruction();
  }

  xed_iclass_enum_t xiclass = XED_ICLASS_INVALID;
  bool findBranchInst = false; 
  while(!findBranchInst) {
    //RCA_PRDBG("[CONDITION] addr %p, %lx\n", inst, *(size_t*)inst);
    inst = decoder->pt_prev_instruction();
    if(inst == NULL) {
      PRINT("can not obtain the intel PT trace!!! exit.");
      exit(-1);
    }
    xiclass = getInstructionType(inst);
    if((xiclass>=XED_ICLASS_JB && xiclass<XED_ICLASS_JMP) 
        || (xiclass>XED_ICLASS_JMP_FAR && xiclass<=XED_ICLASS_JZ)) {
      findBranchInst = true;
    }
  }

  while(inst!=NULL) {
    inst = decoder->pt_prev_instruction();
    xiclass = getInstructionType(inst);
    if(xiclass == XED_ICLASS_TEST
        || xiclass == XED_ICLASS_CMP
        || xiclass == XED_ICLASS_XOR) {
      decodeComparisonInst(ctl, inst);
      break;
    }
  }
}

void RCADisas::decodeComparisonInst(trackctl* ctl, void* address) {
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error; 

  // init inst_t
  xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);

  // FIXME not handle prefix: REP, LOCK ...
  // decode one instruction
  xed_error = xed_decode(xptr, (uint8_t *)address, XED_MAX_INSTRUCTION_BYTES);

  // if no error
  if(xed_error == XED_ERROR_NONE){

    // Return the xed_inst_t structure for this instruction. This is the route to the basic operands form information.
    const xed_inst_t *xi = xed_decoded_inst_inst(xptr);
    // get operands number
    unsigned int noperands = xed_inst_noperands(xi);

    pid_t tid = getCurrentTid(ctl);
    ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
        ctl->trace.cur, NULL, address, 0, SITE_TYPE_CHILD,
        ctl->cslevel, ctl->current_level, ctl->callstack, 0,
        tid);
    RCA_PRINF("#%d# CMP inst %p\n", ctl->trace.num_site, address);

    for(int i=0; i<noperands; i++){
      const xed_operand_t* op = xed_inst_operand(xi, i);
      const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);
      xed_operand_enum_t op_name = xed_operand_name(op);

      if(!check_operand_visibility(op, op_name, xptr)){
        continue;
      }

      if (xed_operand_is_register(op_name)) {
        xed_reg_enum_t r = xed_decoded_inst_get_reg(xptr, op_name);
        //int reg_enum = reg_mapping[r];
        //if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
          ctl->current_regs.addRegister(convertToNorm(r));
          RCA_PRINF("register %s in instruction %p\n", xed_reg_enum_t2str(r), address);
        //}
      }

      if(xed_operand_is_memory_addressing_register(op_name)) {
        xed_reg_enum_t r = xed_decoded_inst_get_reg(xptr, op_name);
        int reg_enum = reg_mapping[r];
        if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
          // find address, put it into instruction list
          ctl->site.addSite(&ctl->trace.si[ctl->trace.num_site-1]);
          RCA_PRINF("#%d# assignment instruction is %p\n", ctl->trace.num_site, address);
        }
      }

      int memop = -1;
      if (op_name == XED_OPERAND_MEM0) 
        memop = 0;
      else if (op_name == XED_OPERAND_MEM1) 
        memop = 1;

      if (memop != -1) {
        xed_reg_enum_t base, index;
        base = xed_decoded_inst_get_base_reg(xptr, memop);
        index = xed_decoded_inst_get_index_reg(xptr, memop);

        if(base != XED_REG_INVALID || index!=XED_REG_INVALID) {
          ctl->site.addSite(&ctl->trace.si[ctl->trace.num_site-1]);
          RCA_PRINF("#%d# assignment instruction is %p\n", ctl->trace.num_site, address);
        }
      }
    }
  }
}

void RCADisas::trackFaultyAddress(trackctl* ctl, PerThreadDecoder* decoder) {
  // find the target registers
  initTracking(ctl, decoder);

  // track register reversely
  // 1. put register into a working list
  // 2. decode every instruction
  // 3. find assignment instruction
  // 4. if a memory assignment instruction is found, 
  //    remove the register from working list.
  //    if a transitive instruction is found, remove old register,
  //    put new register into working list, goto step 2
  // 5. if working list is null, return 

  xed_iclass_enum_t xiclass = XED_ICLASS_INVALID;

  void* inst = decoder->pt_prev_instruction();
  while(inst!=NULL && ctl->current_regs.num_regs != 0) {
    RCA_PRDBG("[CHECK] addr %p, %lx\n", inst, *(size_t*)inst);
    // find assignment for a list
    xiclass = getInstructionType(inst);
    switch(xiclass) {
      case XED_ICLASS_CALL_FAR:
      case XED_ICLASS_CALL_NEAR:
        {
          ctl->current_regs.pop();
          break;
        }
      case XED_ICLASS_RET_FAR:
      case XED_ICLASS_RET_NEAR:
        {
          ctl->current_regs.push();
          break;
        }
      case XED_ICLASS_IRET:
      case XED_ICLASS_IRETD:
      case XED_ICLASS_IRETQ:
        {
          PRINT("DO NOT SUPPORT Interrupt Return\n");
          break;
        }
      default:
        {
          // filter plt operations
          if(ctl->current_regs.hasRegisters() 
              && !selfmap::getInstance().isDlLibrary(inst)) { 
            checkAssignInst(ctl, inst);
          } 
        }
    }
    inst = decoder->pt_prev_instruction();
  }
}

void RCADisas::checkAssignInst(trackctl* ctl, void* inst) {
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error; 

  // init inst_t
  xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);

  // FIXME not handle prefix: REP, LOCK ...
  // decode one instruction
  xed_error = xed_decode(xptr, (uint8_t *)inst, XED_MAX_INSTRUCTION_BYTES);

  // if no error
  if(xed_error == XED_ERROR_NONE){
    // Return the xed_inst_t structure for this instruction. This is the route to the basic operands form information.
    const xed_inst_t *xi = xed_decoded_inst_inst(xptr);
    // get operands number
    unsigned int noperands = xed_inst_noperands(xi);

    // TODO if it is a XOR instruction, we do not treat it as a assignment instruction
    // We should track both source and destination
    xed_iclass_enum_t xiclass = xed_decoded_inst_get_iclass(xptr);
    if(xiclass == XED_ICLASS_XOR || xiclass == XED_ICLASS_POP) {
      return;
    } 

    for(int i=0; i<noperands; i++){
      const xed_operand_t* op = xed_inst_operand(xi, i);
      const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);
      xed_operand_enum_t op_name = xed_operand_name(op);

      if(!check_operand_visibility(op, op_name, xptr)){
        continue;
      }

      if (xed_operand_is_register(op_name) && xed_operand_written(op)) {
        xed_reg_enum_t r = xed_decoded_inst_get_reg(xptr, op_name);
        RegStatus reg_stat = compareRegisters(ctl, r);
        if(reg_stat){
          const xed_operand_t* op1 = xed_inst_operand(xi, 1);
          xed_operand_enum_t op_name1 = xed_operand_name(op1);
          decodeTargetAddr(ctl, inst, xptr, reg_stat, r, op_name1, op1, ov);
          break;
        }
      }
    }
  }
}

void RCADisas::removeSpeculativeInst(trackctl* ctl) {
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error;

  int gen_num = 0, kill_num = 0;
  xed_reg_enum_t GenReg[8];
  xed_reg_enum_t KillReg[8];

  int num_inst = ctl->site.num_site;
  for(int i=0; i<num_inst; i++) {
    void* addr = ctl->site.info[i]->instruction_address;

    xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);
    xed_error = xed_decode(xptr, (uint8_t *)addr, XED_MAX_INSTRUCTION_BYTES);
    if(xed_error == XED_ERROR_NONE){
      xed_iclass_enum_t xiclass = xed_decoded_inst_get_iclass(xptr);
      const xed_inst_t *xi = xed_decoded_inst_inst(xptr);
      // operands
      const xed_operand_t* op = xed_inst_operand(xi, 0);
      xed_operand_enum_t op_name = xed_operand_name(op);
      xed_reg_enum_t target = xed_decoded_inst_get_reg(xptr, op_name);
      const xed_operand_t* op1 = xed_inst_operand(xi, 1);
      xed_operand_enum_t op_name1 = xed_operand_name(op1);

      if((xiclass>=XED_ICLASS_CMOVB && xiclass<=XED_ICLASS_CMOVZ)) {
        sFrame* frame = &ctl->site.context[i];
        if(isExecutedInstruction(addr, frame)) {
          if(xed_operand_is_register(op_name1)) {
            xed_reg_enum_t src = xed_decoded_inst_get_reg(xptr, op_name1);
            GenReg[gen_num++] = src;
          } else {
            ctl->site.info[i]->type = SITE_TYPE_CHILD;
          }
          KillReg[kill_num++] = target;
        } else {
          if(xed_operand_is_register(op_name1)) {
            xed_reg_enum_t src = xed_decoded_inst_get_reg(xptr, op_name1);
            KillReg[kill_num++] = src;
          }
          GenReg[gen_num++] = target;
        }
      } 
      else {
        int iset;
        if(kill_num != 0) {
          for(iset=0; iset<kill_num; iset++) {
            if(target == KillReg[iset]) {
              break;
            }
          }
          if(iset!=kill_num) {
            KillReg[iset] = KillReg[--kill_num];
            if(xed_operand_is_register(op_name1)) {
              xed_reg_enum_t src = xed_decoded_inst_get_reg(xptr, op_name1);
              KillReg[kill_num++] = src;
            }
          }
        }

        if(gen_num != 0) {
          for(iset=0; iset<gen_num; iset++) {
            if(target == GenReg[iset]) {
              break;
            }
          }
          if(iset!=gen_num) {
            GenReg[iset] = GenReg[--gen_num];
            if(ctl->site.info[i]->type == SITE_TYPE_SPECULATION) {
              if(xed_operand_is_register(op_name1)) {
                xed_reg_enum_t src = xed_decoded_inst_get_reg(xptr, op_name1);
                GenReg[gen_num++] = src;
              } else {
                ctl->site.info[i]->type = SITE_TYPE_CHILD;
              }
            } 
          }
        }
      }
    } 

    /* 
    PRINT("\n====================");
    PRINT("Instruction is %p, type is %d", addr, ctl->site.info[i]->type);
    PRINT("***GEN %d:", gen_num);
    for(int igen=0; igen<gen_num; igen++) {
      PRINT("reg: %d", GenReg[igen]);
    }
    PRINT("***Kill %d:", kill_num);
    for(int ikill=0; ikill<kill_num; ikill++) {
      PRINT("reg: %d", KillReg[ikill]);
    }
    PRINT("====================\n");
    */
  }
}

void RCADisas::computeFaultyAddresses(trackctl* ctl) {
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error;

  int num_inst = ctl->site.num_site;
  for(int i=0; i<num_inst; i++) {
    if(ctl->site.info[i]->faultyAddr == NULL
       && ctl->site.info[i]->type != SITE_TYPE_SPECULATION 
       && ctl->site.info[i]->total_triggered_times != 0) {
      void* addr = ctl->site.info[i]->instruction_address;

      xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);
      xed_error = xed_decode(xptr, (uint8_t *)addr, XED_MAX_INSTRUCTION_BYTES);
      if(xed_error == XED_ERROR_NONE){

        // should find the memory address in this instruction
        const xed_inst_t *xi = xed_decoded_inst_inst(xptr);
        unsigned int noperands = xed_inst_noperands(xi);

        for(int op_i=0; op_i<noperands; op_i++){
          const xed_operand_t* op = xed_inst_operand(xi, op_i);
          xed_operand_enum_t op_name = xed_operand_name(op);
          const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);

          if(!check_operand_visibility(op, op_name, xptr)){
            continue;
          }

          int memop = -1;
          if (op_name == XED_OPERAND_MEM0) 
            memop = 0;
          else if (op_name == XED_OPERAND_MEM1) 
            memop = 1;

          if(memop != -1) {
            /*
            mapping* curmap = selfmap::getInstance().getMappingByAddress(addr);
            if(curmap != NULL) {
              PRINT("addr %p, base is %lx, %s\n", addr, curmap->getBase(), curmap->getFile().c_str());
            }
            */

            sFrame* frame = &ctl->site.context[i];
            void* memoryaddr = computeTargetAddress(addr, xptr, op_name, op, ov, frame);
            ctl->site.info[i]->faultyAddr = memoryaddr;
          }
        }
      }
    } 
  }
}

void* RCADisas::computeTargetAddress(void* inst, xed_decoded_inst_t* xptr, xed_operand_enum_t op_name, 
    const xed_operand_t* op, const xed_operand_values_t* ov, sFrame* frame) {
  void* targetip = NULL;
  
  if(xed_operand_is_memory_addressing_register(op_name)
      && xed_operand_read(op)){
    xed_reg_enum_t r = xed_decoded_inst_get_reg(xptr, op_name);
    int reg_enum = reg_mapping[r];
    if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
      targetip = (void*)GET_REG(frame, reg_enum);
    }
  }
    
  int memop = -1;
  if (op_name == XED_OPERAND_MEM0 
      || op_name == XED_OPERAND_AGEN) 
    memop = 0;
  else if (op_name == XED_OPERAND_MEM1) 
    memop = 1; 

  if (memop != -1) {
    xed_int64_t addr = 0;
    /*  get reads of base/index regs,  if any */
    xed_reg_enum_t base, index;
    base = xed_decoded_inst_get_base_reg(xptr, memop);
    index = xed_decoded_inst_get_index_reg(xptr, memop);

    if(base != XED_REG_INVALID) {
      int reg_enum = reg_mapping[base];
      if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
        if(reg_enum == REG_RIP) {
          xed_uint64_t instruction_length = xed_decoded_inst_get_length(xptr);
          addr += (intptr_t)inst + instruction_length;
        } else {
          addr += GET_REG(frame, reg_enum);
        }
      }
    }

    if(index != XED_REG_INVALID) {
      int reg_enum = reg_mapping[index];
      if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
        xed_int64_t index_r_val = GET_REG(frame, reg_enum);
        xed_bits_t scale = xed3_operand_get_scale(xptr);
        index_r_val *= XED_STATIC_CAST(xed_uint_t, scale);
        addr += index_r_val;
      }
    }

    xed_int64_t disp = xed_operand_values_get_memory_displacement_int64(ov);
    xed_bool_t no_base_index = (base == XED_REG_INVALID) && (index == XED_REG_INVALID);
    if (xed_operand_values_has_memory_displacement(ov)) {
      unsigned int disp_bits = xed_operand_values_get_memory_displacement_length_bits(ov);
      if (disp_bits && (disp || no_base_index)) {
        addr += disp;
      }
    }
    targetip = (void*)addr;
  }

  //assert(targetip!=NULL);
  return targetip;
}

void RCADisas::decodeTargetAddr(trackctl* ctl, void* inst, xed_decoded_inst_t* xptr, RegStatus reg_stat, xed_reg_enum_t reg,
    xed_operand_enum_t op_name, const xed_operand_t* op, const xed_operand_values_t* ov) {

  int memop = -1;
  if (op_name == XED_OPERAND_MEM0 
      || op_name == XED_OPERAND_AGEN) 
    memop = 0;
  else if (op_name == XED_OPERAND_MEM1) 
    memop = 1; 

  xed_iclass_enum_t xiclass = xed_decoded_inst_get_iclass(xptr);

  pid_t tid = getCurrentTid(ctl);

  if((xiclass>=XED_ICLASS_CMOVB && xiclass<=XED_ICLASS_CMOVZ)) {

    ctl->current_regs.removeRegister(convertToNorm(reg));
    ctl->current_regs.addRegister(convertToNorm(reg) + XED_REG_LAST);

    // find address, put it into instruction list
    ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
        ctl->trace.cur, NULL, inst, 0, SITE_TYPE_SPECULATION, ctl->cslevel, 
        ctl->current_level, ctl->callstack, 0, tid);
    ctl->site.addSite(&ctl->trace.si[ctl->trace.num_site-1]);
    RCA_PRINF("#%d# SPECULATION assignment instruction is %p\n", ctl->trace.num_site, inst);

    if(xed_operand_is_register(op_name) && xed_operand_read(op)) {
      xed_reg_enum_t r = xed_decoded_inst_get_reg(xptr, op_name);
      //int reg_enum = reg_mapping[r];
      //if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
        // put it into register list
        ctl->current_regs.addRegister(convertToNorm(r) + XED_REG_LAST);
        RCA_PRINF("CMOVcc instruction %p has a register, tracking %s register\n", inst, xed_reg_enum_t2str(r));
      //}
      return;
    }

    if(op_name == XED_OPERAND_AGEN) {
      /*  get reads of base/index regs,  if any */
      xed_reg_enum_t base, index;

      base = xed_decoded_inst_get_base_reg(xptr, memop);
      index = xed_decoded_inst_get_index_reg(xptr, memop);

      if (base != XED_REG_INVALID){
        int reg_enum = reg_mapping[base];
        if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
          ctl->current_regs.addRegister(convertToNorm(base) + XED_REG_LAST);
          RCA_PRINF("#%d# SPECULATION LEA inst %p, register is %s\n", ctl->trace.num_site, inst, xed_reg_enum_t2str(base));
        }
      }

      if (index != XED_REG_INVALID) {
        int reg_enum = reg_mapping[index];
        if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
          ctl->current_regs.addRegister(convertToNorm(index) + XED_REG_LAST);
          RCA_PRINF("#%d# SPECULATION LEA inst %p, register is %s\n", ctl->trace.num_site, inst, xed_reg_enum_t2str(index));
        }
      }
    } 
    return;
  }
    
  switch(xiclass) {
    case XED_ICLASS_SHL:
    case XED_ICLASS_SHR:
    /* mathematical operations */
    case XED_ICLASS_ADD:
    case XED_ICLASS_SUB:
    case XED_ICLASS_SAR:
    /* logic operations */
    case XED_ICLASS_OR:
    case XED_ICLASS_XOR:
    case XED_ICLASS_ROR:
    case XED_ICLASS_RORX:
    case XED_ICLASS_AND:
      {
        if(xed_operand_is_register(op_name)) {
          xed_reg_enum_t r = xed_decoded_inst_get_reg(xptr, op_name);
          //int reg_enum = reg_mapping[r];
          //if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
            // put it into register list
            if(reg_stat == REG_SPECULATION) {
              ctl->current_regs.addRegister(convertToNorm(r) + XED_REG_LAST);
              RCA_PRINF("SPECULATION instruction %p has a register, tracking %s register\n", inst, xed_reg_enum_t2str(r));

              ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
                  ctl->trace.cur, NULL, inst, 0, SITE_TYPE_SPECULATION, ctl->cslevel, 
                  ctl->current_level, ctl->callstack, 0, tid);
              ctl->site.addSite(&ctl->trace.si[ctl->trace.num_site-1]);
              RCA_PRINF("#%d# SPECULATION assignment instruction is %p\n", ctl->trace.num_site, inst);
            } else {
              ctl->current_regs.addRegister(convertToNorm(r));
              RCA_PRINF("instruction %p has a register, tracking %s register\n", inst, xed_reg_enum_t2str(r));
            }
          //}
        }
        if(memop != -1){ 
          ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
              ctl->trace.cur, NULL, inst, 0, 
              reg_stat==REG_SPECULATION?SITE_TYPE_SPECULATION:SITE_TYPE_CHILD, ctl->cslevel, 
              ctl->current_level, ctl->callstack, 0, tid);
          ctl->site.addSite(&ctl->trace.si[ctl->trace.num_site-1]);
          RCA_PRINF("#%d# assignment instruction is %p\n", ctl->trace.num_site, inst);
        }
        //RCA_PRINF("instruction is add or sub, keep tracking...\n");
        return;
      }
    default:
      {
        if(xed_operand_is_register(op_name) && xed_operand_read(op)) {
          xed_reg_enum_t r = xed_decoded_inst_get_reg(xptr, op_name);
          //int reg_enum = reg_mapping[r];
          //if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
            // put it into register list
            if(reg_stat == REG_SPECULATION) {
              ctl->current_regs.removeRegister(convertToNorm(reg) + XED_REG_LAST);
              ctl->current_regs.addRegister(convertToNorm(r) + XED_REG_LAST);
              RCA_PRINF("SPECULATION instruction %p has a register, tracking %s register\n", inst, xed_reg_enum_t2str(r));
              ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
                  ctl->trace.cur, NULL, inst, 0, SITE_TYPE_SPECULATION, ctl->cslevel, 
                  ctl->current_level, ctl->callstack, 0, tid);
              ctl->site.addSite(&ctl->trace.si[ctl->trace.num_site-1]);
              RCA_PRINF("#%d# SPECULATION assignment instruction is %p\n", ctl->trace.num_site, inst);
            } else {
              ctl->current_regs.removeRegister(convertToNorm(reg));
              ctl->current_regs.addRegister(convertToNorm(r));
              RCA_PRINF("instruction %p has a register, tracking %s register\n", inst, xed_reg_enum_t2str(r));
            }
          //}
          return;
        }

        if(xed_operand_is_memory_addressing_register(op_name)
            && xed_operand_read(op)){
          xed_reg_enum_t r = xed_decoded_inst_get_reg(xptr, op_name);
          int reg_enum = reg_mapping[r];
          if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
            // find address, put it into instruction list
            if(reg_stat == REG_SPECULATION) {
              ctl->current_regs.removeRegister(convertToNorm(reg) + XED_REG_LAST);
            } else {
              ctl->current_regs.removeRegister(convertToNorm(reg));
            }

            ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
                ctl->trace.cur, NULL, inst, 0, 
                reg_stat==REG_SPECULATION?SITE_TYPE_SPECULATION:SITE_TYPE_CHILD, ctl->cslevel, 
                ctl->current_level, ctl->callstack, 0, tid);
            ctl->site.addSite(&ctl->trace.si[ctl->trace.num_site-1]);
            RCA_PRINF("#%d# assignment instruction is %p\n", ctl->trace.num_site, inst);
            return;
          }
        }

        if(memop != -1){ 
          if(reg_stat == REG_SPECULATION) {
            ctl->current_regs.removeRegister(convertToNorm(reg) + XED_REG_LAST);
          } else {
            ctl->current_regs.removeRegister(convertToNorm(reg));
          }

          if(op_name == XED_OPERAND_AGEN) {
            ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
                ctl->trace.cur, NULL, inst, 0, reg_stat==REG_SPECULATION?SITE_TYPE_SPECULATION:SITE_TYPE_CHILD, ctl->cslevel, 
                ctl->current_level, ctl->callstack, 0, tid);
            if(reg_stat == REG_SPECULATION) {
              ctl->site.addSite(&ctl->trace.si[ctl->trace.num_site-1]);
            }

            /*  get reads of base/index regs,  if any */
            xed_reg_enum_t base, index;

            base = xed_decoded_inst_get_base_reg(xptr, memop);
            index = xed_decoded_inst_get_index_reg(xptr, memop);

            if (base != XED_REG_INVALID){
              int reg_enum = reg_mapping[base];
              if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
                if(reg_stat == REG_SPECULATION) {
                  ctl->current_regs.addRegister(convertToNorm(base) + XED_REG_LAST);
                  RCA_PRINF("#%d# SPECULATION LEA inst %p, register is %s\n", ctl->trace.num_site, inst, xed_reg_enum_t2str(base));
                } else {
                  ctl->current_regs.addRegister(convertToNorm(base));
                  RCA_PRINF("#%d# LEA inst %p, register is %s\n", ctl->trace.num_site, inst, xed_reg_enum_t2str(base));
                }
              }
            }

            if (index != XED_REG_INVALID) {
              int reg_enum = reg_mapping[index];
              if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
                if(reg_stat == REG_SPECULATION) {
                  ctl->current_regs.addRegister(convertToNorm(index) + XED_REG_LAST);
                  RCA_PRINF("#%d# SPECULATION LEA inst %p, register is %s\n", ctl->trace.num_site, inst, xed_reg_enum_t2str(index));
                }{
                  ctl->current_regs.addRegister(convertToNorm(index));
                  RCA_PRINF("#%d# LEA inst %p, register is %s\n", ctl->trace.num_site, inst, xed_reg_enum_t2str(index));
                }
              }
            }
          } else {
            xed_reg_enum_t seg = xed_decoded_inst_get_seg_reg(xptr, memop);
            if(seg == XED_REG_INVALID) {
              // find address, put it into instruction list
              ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
                  ctl->trace.cur, NULL, inst, 0, reg_stat==REG_SPECULATION?SITE_TYPE_SPECULATION:SITE_TYPE_CHILD, ctl->cslevel, 
                  ctl->current_level, ctl->callstack, 0, tid);
              ctl->site.addSite(&ctl->trace.si[ctl->trace.num_site-1]);
              RCA_PRINF("#%d# assignment instruction is %p\n", ctl->trace.num_site, inst);
            }
          }
          return;
        }

        if(op_name == XED_OPERAND_PTR) {
          unsigned int disp =(unsigned int) xed_operand_values_get_branch_displacement_int32(ov);
          //xed_bool_t long_mode = xed_operand_values_get_long_mode(xed_decoded_inst_operands_const(xptr));
          xed_uint64_t ptr = disp;
          // put it into address list
          if(reg_stat == REG_SPECULATION) {
            ctl->current_regs.removeRegister(convertToNorm(reg) + XED_REG_LAST);
          } else {
            ctl->current_regs.removeRegister(convertToNorm(reg));
          }
          ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
              ctl->trace.cur, (void*) ptr, inst, 0, 
              reg_stat==REG_SPECULATION?SITE_TYPE_SPECULATION:SITE_TYPE_CHILD, ctl->cslevel, 
              ctl->current_level, ctl->callstack, 0, tid);
          ctl->site.addSite(&ctl->trace.si[ctl->trace.num_site-1]);
          RCA_PRINF("#%d# assignment instruction is %p\n", ctl->trace.num_site, inst);
          return;
        }

        if(op_name == XED_OPERAND_RELBR) {
          // we do not care whether it is a memory or branch displacement
          // they would call the same function final in most cases
          xed_int64_t disp = xed3_operand_get_disp(xptr);
          xed_uint64_t instruction_length = xed_decoded_inst_get_length(xptr);
          xed_uint64_t pc = (intptr_t)inst + instruction_length;
          xed_uint64_t effective_addr = (xed_uint64_t) ((xed_int64_t)pc  + disp);
          // put it into address list
          if(reg_stat == REG_SPECULATION) {
            ctl->current_regs.removeRegister(convertToNorm(reg) + XED_REG_LAST);
          } else {
            ctl->current_regs.removeRegister(convertToNorm(reg));
          }
          ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
              ctl->trace.cur, (void*) effective_addr, inst, 0,  
              reg_stat==REG_SPECULATION?SITE_TYPE_SPECULATION:SITE_TYPE_CHILD, ctl->cslevel, 
              ctl->current_level, ctl->callstack, 0, tid);
          ctl->site.addSite(&ctl->trace.si[ctl->trace.num_site-1]);
          RCA_PRINF("#%d# assignment instruction is %p\n", ctl->trace.num_site, inst);
          return;
        }

        if(op_name == XED_OPERAND_IMM0) {
          unsigned int real_bits = xed_decoded_inst_operand_element_size_bits(xptr, 1);
          size_t mask = (0xFFFFFFFFFFFFFFFF >> (64-real_bits));
          xed_uint64_t imm = xed_operand_values_get_immediate_uint64(ov);
          imm &= mask;
          // put it into address list
          if(reg_stat == REG_SPECULATION) {
            ctl->current_regs.removeRegister(convertToNorm(reg) + XED_REG_LAST);
          } else {
            ctl->current_regs.removeRegister(convertToNorm(reg));
          }
          ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
              ctl->trace.cur, (void*) imm, inst, 0, 
              reg_stat==REG_SPECULATION?SITE_TYPE_SPECULATION:SITE_TYPE_CHILD, ctl->cslevel, 
              ctl->current_level, ctl->callstack, 0, tid);
          PRINT("Memory assignment at %p is immediate value %lx\n", inst, imm);
          return;
        }

        if(op_name == XED_OPERAND_IMM1) {
          xed_uint64_t imm = xed3_operand_get_uimm1(xptr);
          // put it into address list
          if(reg_stat == REG_SPECULATION) {
            ctl->current_regs.removeRegister(convertToNorm(reg) + XED_REG_LAST);
          } else {
            ctl->current_regs.removeRegister(convertToNorm(reg));
          }
          ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
              ctl->trace.cur, (void*) imm, inst, 0, 
              reg_stat==REG_SPECULATION?SITE_TYPE_SPECULATION:SITE_TYPE_CHILD, ctl->cslevel, 
              ctl->current_level, ctl->callstack, 0, tid);
          PRINT("Memory assignment at %p is immediate value %lx\n", inst, imm);
          return;
        }
      }
  }
}

void RCADisas::saveMemoryAssignment(trackctl* ctl, siteInfo* parent, pid_t tid, CallStackInfo* saved_callstack) {
  sFrame* frame = saved_callstack->callstack;
  // find the memory assignment instruction
  void* access_ip = (void*)(frame[0].regs[REG_RIP]);
  access_ip = getPrevInstruction(access_ip);
  frame[0].regs[REG_RIP] = (intptr_t)access_ip;

  xed_reg_enum_t reg = get_memoryassignment_reg(access_ip);
  if(reg != XED_REG_INVALID) {
    ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
        parent, NULL, access_ip, reg, SITE_TYPE_ROOT_SUBLEVEL,
        saved_callstack->level, 0, frame, 0, tid);
    PRINF("add memory access at %p, tid %d\n", access_ip, tid);
    // save sync info
    ctl->trace.si[ctl->trace.num_site-1].sync_num = saved_callstack->sync_num;
    for(int isync=0; isync<saved_callstack->sync_num; isync++) { 
      ctl->trace.si[ctl->trace.num_site-1].sync_event[isync] = saved_callstack->sync_event[isync];
    }
  } else {
    ctl->trace.si[ctl->trace.num_site++].addSiteInfo(
        parent, NULL, access_ip, reg, SITE_TYPE_CHILD,
        saved_callstack->level, 0, frame, 0, tid);
  }
}

void* RCADisas::getPrevInstruction(void* current_ip) {
  uint8_t* prev_instruction = NULL;

  mapping* curmap = selfmap::getInstance().getMappingByAddress(current_ip);
  if(curmap != NULL) {
    InstPos* inst = curmap->getInstBuffer()->findInstByAddr((uint8_t *)current_ip, curmap);
    if(inst != NULL) {
      inst--;
      prev_instruction = inst->iaddr;
      //PRINT("prev instruction %p, base %lx (%s)\n", prev_instruction, curmap->getBase(), curmap->getFile().c_str());
    }
  }

  /*
  xed_decoded_inst_t cur_xedd;
  xed_decoded_inst_t *xptr = &cur_xedd;
  xed_error_enum_t xed_error;

  int offset = 1;
  while(offset<XED_MAX_INSTRUCTION_BYTES) {
    prev_instruction = (uint8_t *)((intptr_t)current_ip-offset++);
    xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);
    xed_error = xed_decode(xptr, prev_instruction, XED_MAX_INSTRUCTION_BYTES);
    if(xed_error == XED_ERROR_NONE){ 
      int length = xed_decoded_inst_get_length(xptr);
      if(prev_instruction + length == current_ip) {
        xed_iclass_enum_t xiclass = xed_decoded_inst_get_iclass(xptr);
        PRINT("prev instruction %p, type %d\n", prev_instruction, xiclass);
        break;
      }
    }
  }
  */

  return prev_instruction;
}

void RCADisas::getMemoryAddressByPrevInstruction(void* addr, void* context, MemAccess* mem) {
  xed_decoded_inst_t cur_xedd;
  xed_decoded_inst_t *xptr = &cur_xedd;
  xed_error_enum_t xed_error;

  void* memoryaddr = NULL;
  void* inst = getPrevInstruction(addr);

  xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);
  xed_error = xed_decode(xptr, (uint8_t *)inst, XED_MAX_INSTRUCTION_BYTES);
  if(xed_error == XED_ERROR_NONE){

    xed_iclass_enum_t xiclass = xed_decoded_inst_get_iclass(xptr);

    switch(xiclass) {
      case XED_ICLASS_VMOVDQA:
        {
          mem->length = 32;
          break;
        }
      case XED_ICLASS_MOVDQU:
        {
          mem->length = 16;
          break;
        }
      default:
        {
          mem->length = 1;
        }
    }

    switch(xiclass) {
      case XED_ICLASS_PUSH:
      case XED_ICLASS_PUSHA:
      case XED_ICLASS_PUSHAD:
      case XED_ICLASS_PUSHF:
      case XED_ICLASS_PUSHFD:
      case XED_ICLASS_PUSHFQ:
        {
          memoryaddr = (void*)((ucontext_t *)context)->uc_mcontext.gregs[REG_RSP];
          break;
        }
      default:
        {

          const xed_inst_t *xi = xed_decoded_inst_inst(xptr);
          unsigned int noperands = xed_inst_noperands(xi);

          for(int op_i=0; op_i<noperands; op_i++){
            const xed_operand_t* op = xed_inst_operand(xi, op_i);
            xed_operand_enum_t op_name = xed_operand_name(op);
            const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);

            if(!check_operand_visibility(op, op_name, xptr)){
              continue;
            }

            int memop = -1;
            if (op_name == XED_OPERAND_MEM0) 
              memop = 0;
            else if (op_name == XED_OPERAND_MEM1) 
              memop = 1;

            if(memop != -1) {
              sFrame frame;
              for(int reg=REG_R8; reg<=REG_CR2; reg++) {
                frame.regs[reg] = ((ucontext_t *)context)->uc_mcontext.gregs[reg];
              }
              memoryaddr = computeTargetAddress(inst, xptr, op_name, op, ov, &frame);
            }
          }
        }
    }
  }
  mem->addr = memoryaddr;
}

size_t RCADisas::appendBlockByIP(void** buffer, void* ip){
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error; 

  size_t inst_num = 0;
  xed_iclass_enum_t xiclass = XED_ICLASS_INVALID;
  do {
    unsigned int ilen = XED_MAX_INSTRUCTION_BYTES;
    // init inst_t
    xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);

    // FIXME not handle prefix: REP, LOCK ...
    // decode one instruction
    xed_error = xed_decode(xptr, (uint8_t *)ip, ilen);

    // if no error
    if(xed_error == XED_ERROR_NONE){
      xiclass = xed_decoded_inst_get_iclass(xptr);
      ilen = xed_decoded_inst_get_length(xptr);

      // put instruction into buffer
      buffer[inst_num++] = ip; 
      //if((intptr_t)ip < 0x10000000)
      //if(selfmap::getInstance().isApplication(ip) 
      //      || selfmap::getInstance().isDoubleTakeLibrary(ip))
      //RCA_PRDBG("[BB] %zu addr %p, ilen is %d, %lx\n", inst_num, ip, ilen, *(size_t*)ip);

      //if this is a function call or direct jump
      // and the target ip can be determind,
      // jump to the target ip
      if(xiclass == XED_ICLASS_CALL_FAR || xiclass == XED_ICLASS_CALL_NEAR
          || xiclass == XED_ICLASS_JMP || xiclass == XED_ICLASS_JMP_FAR) {
        // only care the first operand for CALL and JMP instruction 
        const xed_inst_t *xi = xed_decoded_inst_inst(xptr);
        const xed_operand_t* op = xed_inst_operand(xi, 0);
        const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);
        xed_operand_enum_t op_name = xed_operand_name(op); 

        void* tip = getBranchAddress(ip, xptr, op_name, op, ov);
        if(tip != NULL) {
          inst_num += appendBlockByIP(&buffer[inst_num], tip);
        }
        break;
      }
    }

    // move to next instruction
    ip = (void*)((intptr_t)ip + ilen);
  } while(((xiclass<XED_ICLASS_JB || xiclass>XED_ICLASS_JZ) 
      || (xiclass == XED_ICLASS_JMP || xiclass == XED_ICLASS_JMP_FAR)) 
      && (xiclass != XED_ICLASS_RET_FAR && xiclass != XED_ICLASS_RET_NEAR)
      && (xiclass != XED_ICLASS_HLT));

  return inst_num;
}

size_t RCADisas::checkBranchAndAppend(void** buffer, void* branch_inst, bool istaken) {
  xed_decoded_inst_t *xptr = &xedd;
  size_t inst_num = 0;
  int ilength = 0;
  xed_error_enum_t xed_error = decode_inst(XED_REINTERPRET_CAST(const xed_uint8_t*, branch_inst), XED_MAX_INSTRUCTION_BYTES, &ilength);
  if(xed_error == XED_ERROR_NONE) {
    void* next_addr = (void*)((intptr_t)branch_inst + ilength);
    if(istaken) {
      // only care the first operand for CALL and JMP instruction 
      const xed_inst_t *xi = xed_decoded_inst_inst(xptr);
      const xed_operand_t* op = xed_inst_operand(xi, 0);
      const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);
      xed_operand_enum_t op_name = xed_operand_name(op); 

      next_addr = getBranchAddress(branch_inst, xptr, op_name, op, ov);
    } 

    if(next_addr != NULL) {
      inst_num = appendBlockByIP(buffer, next_addr);
    } else {
      PRINT("TNT meets a non-determined branch %p!!!", branch_inst);
    }
  }
  return inst_num;
}

void* RCADisas::getBranchAddress(void* address, xed_decoded_inst_t* xptr, xed_operand_enum_t op_name, const xed_operand_t* op, const xed_operand_values_t* ov){
  void* target_address = NULL;

  if(op_name == XED_OPERAND_PTR) { 
    unsigned int disp =(unsigned int) xed_operand_values_get_branch_displacement_int32(ov); 
    //xed_bool_t long_mode = xed_operand_values_get_long_mode(xed_decoded_inst_operands_const(xptr));
    xed_uint64_t ptr = disp;
    target_address = (void*) ptr;
  }

  if(op_name == XED_OPERAND_RELBR) {
    // we do not care whether it is a memory or branch displacement
    // they would call the same function final in most cases
    xed_int64_t disp = xed3_operand_get_disp(xptr);
    xed_uint64_t instruction_length = xed_decoded_inst_get_length(xptr);
    xed_uint64_t pc = (intptr_t)address + instruction_length;
    xed_uint64_t effective_addr = (xed_uint64_t) ((xed_int64_t)pc  + disp);

    target_address = (void*) effective_addr;
  }

  if(op_name == XED_OPERAND_IMM0) {
    unsigned int real_bits = xed_decoded_inst_operand_element_size_bits(xptr, 1);
    size_t mask = (0xFFFFFFFFFFFFFFFF >> (64-real_bits));
    xed_uint64_t imm = xed_operand_values_get_immediate_uint64(ov);
    imm &= mask;

    target_address = (void*) imm;
  }
  
  if(op_name == XED_OPERAND_IMM1) {
    xed_uint64_t imm = xed3_operand_get_uimm1(xptr);

    target_address = (void*) imm;
  }

  return target_address;
}

// this function is used to check: 
// whether some conditional instruction is executed or not.
bool RCADisas::isExecutedInstruction(void* inst, sFrame* frame) {
  bool isexec = false;
  xed_decoded_inst_t cur_xedd;
  xed_decoded_inst_t *xptr = &cur_xedd;
  xed_error_enum_t xed_error; 

  //init inst_t
  xed_decoded_inst_zero_set_mode(xptr, &xed_machine_state);

  // FIXME not handle prefix: REP, LOCK ...
  // decode one instruction
  xed_error = xed_decode(xptr, (uint8_t *)inst, XED_MAX_INSTRUCTION_BYTES);

  if(xed_error == XED_ERROR_NONE){ 
    //ucontext_t* trapcontext = (ucontext_t*)context;
    //long long flags = trapcontext->uc_mcontext.gregs[REG_EFL]; 
    long long flags = frame->regs[REG_EFL]; 

    //The status flags (bits 0, 2, 4, 6, 7, and 11) of the EFLAGS register
    //                      CF PF AF ZF SF      OF
    long cf_mask = 0x0001;
    long pf_mask = 0x0004;
    long af_mask = 0x0010;
    long zf_mask = 0x0040;
    long sf_mask = 0x0080;
    long of_mask = 0x0800;

    xed_iclass_enum_t xiclass = xed_decoded_inst_get_iclass(xptr);
    switch(xiclass) {
      case XED_ICLASS_CMOVB:
        {
          // Move if below (CF=1)
          isexec = cf_mask & flags;
          break;
        }
      case XED_ICLASS_CMOVBE:
        {
          // Move if below or equal (CF=1 or ZF=1) 
          isexec = (cf_mask & flags) || (zf_mask & flags);
          break;
        }
      case XED_ICLASS_CMOVL:
        {
          // Move if less (SF ≠ OF)
          isexec = (sf_mask & flags) ^ (of_mask & flags);
          break;
        }
      case XED_ICLASS_CMOVLE:
        {
          // Move if less or equal (ZF=1 or SF ≠ OF)
          isexec = (zf_mask & flags) || ((sf_mask & flags) ^ (of_mask & flags));
          break;
        }
      case XED_ICLASS_CMOVNB:
        {
          // Move if not below (CF=0)
          isexec = !(cf_mask & flags);
          break;
        }
      case XED_ICLASS_CMOVNBE:
        {
          // Move if not below or equal (CF=0 and ZF=0)
          isexec = !(cf_mask & flags) && !(zf_mask & flags);
          break;
        }
      case XED_ICLASS_CMOVNL:
        {
          // Move if not less (SF=OF)
          isexec = !((sf_mask & flags) ^ (of_mask & flags));
          break;
        }
      case XED_ICLASS_CMOVNLE:
        {
          // Move if not less or equal (ZF=0 and SF=OF)
          isexec = !(zf_mask & flags) && !((sf_mask & flags) ^ (of_mask & flags));
          break;
        }
      case XED_ICLASS_CMOVNO:
        {
          // Move if not overflow (OF=0)
          isexec = !(of_mask & flags);
          break;
        }
      case XED_ICLASS_CMOVNP:
        {
          // Move if not parity (PF=0)
          isexec = !(pf_mask & flags);
          break;
        }
      case XED_ICLASS_CMOVNS:
        {
          // Move if not sign (SF=0)
          isexec = !(sf_mask & flags);
          break;
        }
      case XED_ICLASS_CMOVNZ:
        {
          // Move if not zero (ZF=0)
          isexec = !(zf_mask & flags);
          break;
        }
      case XED_ICLASS_CMOVO:
        {
          // Move if overflow (OF=1)
          isexec = of_mask & flags;
          break;
        }
      case XED_ICLASS_CMOVP:
        {
          // Move if parity (PF=1)
          isexec = pf_mask & flags;
          break;
        }
      case XED_ICLASS_CMOVS:
        {
          // Move if sign (SF=1)
          isexec = sf_mask & flags;
          break;
        }
      case XED_ICLASS_CMOVZ:
        {
          // Move if zero (ZF=1)
          isexec = zf_mask & flags;
          break;
        }
      default:
        {
          isexec = true; 
        }
    }
  }

  return isexec;
}

