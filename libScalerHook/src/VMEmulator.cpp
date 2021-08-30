
#include <util/tool/VMEmulator.h>
#include <asm/prctl.h>
#include <sys/syscall.h>

static xed_state_t vm_xed_machine_state = {
        XED_MACHINE_MODE_LONG_64,
        XED_ADDRESS_WIDTH_64b
};

#define OPERAND_NUMBER 3

void scaler::VMEmulator::getInstrInfo(scaler::Breakpoint &bp) {
    xed_decoded_inst_t xedd;
    xed_decoded_inst_t *xptr = &xedd;
    xed_decoded_inst_zero_set_mode(xptr, &vm_xed_machine_state);
    xed_error_enum_t xed_error;

    xed_error = xed_decode((xed_decoded_inst_t *) xptr,
                           (const xed_uint8_t *) XED_REINTERPRET_CAST(const xed_uint8_t*, bp.addr),
                           (const unsigned int) XED_MAX_INSTRUCTION_BYTES);

    if (xed_error == XED_ERROR_NONE) {
        bp.instLen = xed_decoded_inst_get_length(xptr);
        // backup original data
        for (int i = 0; i < bp.instLen; i++) {
            bp.oriCode[i] = ((char *) bp.addr)[i];
        }
        // TRY to avoid functions from other library
        //memcpy(bp->inst_data, bp->addr, bp->inst_length);
    } else {
        throwScalerException(ErrCode::UNRECOGNIZSBLE_INSTR,
                             "Intel xed fail to decode instruction, meaning the instruction is unrecognizable");
    }
}

void *scaler::VMEmulator::executeInstr(scaler::Breakpoint &bp, ucontext_t *context) {
//    void* next_instruction = NULL;
//    xed_decoded_inst_t xedd;
//    xed_decoded_inst_t *xptr = &xedd;
//    xed_error_enum_t xed_error;
//
//    xed_decoded_inst_zero_set_mode(xptr, &vm_xed_machine_state);
//
//    xed_error = xed_decode(xptr, XED_REINTERPRET_CAST(const xed_uint8_t*, bp.oriCode), bp.instLen);
//
//    if(xed_error == XED_ERROR_NONE){
//        xed_iclass_enum_t xiclass = xed_decoded_inst_get_iclass(xptr);
//
//        InstrOp operands[OPERAND_NUMBER];
//        xed_uint64_t rel_address =  (xed_uint64_t)bp.addr;
//
//        parseOp(xptr, rel_address, operands, OPERAND_NUMBER, context);
//
//        next_instruction =  (void*)((intptr_t)bp->addr + bp.instLen);
//
//        switch(xiclass) {
//            case XED_ICLASS_SAR:
//            case XED_ICLASS_SHL:
//            {
//                // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
//                size_t src_value = getOp(&operands[1]);
//
//                operand_t* dest = &operands[0];
//                size_t dest_value = GetOperandValue(dest);
//                if(dest->type == OPERAND_TYPE_REGISTER) {
//                    size_t mask = GetRegisterMask(dest);
//
//                    int reg_enum = reg_mapping[dest->name];
//                    if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
//                        size_t orig_value;
//                        if(Is32BitRegister(dest)) {
//                            orig_value = dest_value;
//                        } else {
//                            orig_value = GET_REG_VALUE(context, reg_enum);
//                        }
//                        size_t new_value = ((dest_value << src_value) & mask) | (orig_value & ~mask);
//                        if(xiclass==XED_ICLASS_SAR) {
//                            new_value = ((dest_value >> src_value) & mask) | (orig_value & ~mask);
//                        }
//                        SET_REG_VALUE(context, reg_enum, new_value);
//                        // set signed bits
//                    }
//
//                } else if(dest->type == OPERAND_TYPE_MEMORY) {
//                    if(xiclass==XED_ICLASS_SAR) {
//                        SetValueToMemory((void*)dest->value, dest->size, dest_value >> src_value);
//                    } else {
//                        SetValueToMemory((void*)dest->value, dest->size, dest_value << src_value);
//                    }
//                }
//
//                break;
//            }
//                /*  mathematical operations */
//            case XED_ICLASS_ADD:
//            {
//                // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
//                size_t src_value = GetOperandValue(&operands[1]);
//
//                operand_t* dest = &operands[0];
//                size_t dest_value = GetOperandValue(dest);
//                if(dest->type == OPERAND_TYPE_REGISTER) {
//                    size_t mask = GetRegisterMask(dest);
//
//                    int reg_enum = reg_mapping[dest->name];
//                    if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
//                        size_t orig_value;
//                        if(Is32BitRegister(dest)) {
//                            orig_value = dest_value;
//                        } else {
//                            orig_value = GET_REG_VALUE(context, reg_enum);
//                        }
//                        size_t new_value = ((src_value + dest_value) & mask) | (orig_value & ~mask);
//                        SET_REG_VALUE(context, reg_enum, new_value);
//                        // set signed bits
//                    }
//
//                } else if(dest->type == OPERAND_TYPE_MEMORY) {
//                    SetValueToMemory((void*)dest->value, dest->size, src_value + dest_value);
//                }
//
//                break;
//            }
//            case XED_ICLASS_SUB:
//            {
//                // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
//                size_t src_value = GetOperandValue(&operands[1]);
//
//                operand_t* dest = &operands[0];
//                size_t dest_value = GetOperandValue(dest);
//                if(dest->type == OPERAND_TYPE_REGISTER) {
//                    size_t mask = GetRegisterMask(dest);
//
//                    int reg_enum = reg_mapping[dest->name];
//                    if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
//                        size_t orig_value;
//                        if(Is32BitRegister(dest)) {
//                            orig_value = dest_value;
//                        } else {
//                            orig_value = GET_REG_VALUE(context, reg_enum);
//                        }
//                        size_t new_value = ((dest_value - src_value) & mask) | (orig_value & ~mask);
//                        SET_REG_VALUE(context, reg_enum, new_value);
//                        // set signed bits
//                    }
//
//                } else if(dest->type == OPERAND_TYPE_MEMORY) {
//                    SetValueToMemory((void*)dest->value, dest->size, dest_value - src_value);
//                }
//
//                break;
//            }
//                /*  logic operations */
//            case XED_ICLASS_XOR:
//            {
//                // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
//                size_t src_value = GetOperandValue(&operands[1]);
//
//                operand_t* dest = &operands[0];
//                size_t dest_value = GetOperandValue(dest);
//                if(dest->type == OPERAND_TYPE_REGISTER) {
//                    size_t mask = GetRegisterMask(dest);
//
//                    int reg_enum = reg_mapping[dest->name];
//                    if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
//                        size_t orig_value;
//                        if(Is32BitRegister(dest)) {
//                            orig_value = dest_value;
//                        } else {
//                            orig_value = GET_REG_VALUE(context, reg_enum);
//                        }
//                        size_t new_value = ((src_value ^ dest_value) & mask) | (orig_value & ~mask);
//                        SET_REG_VALUE(context, reg_enum, new_value);
//                        // set signed bits
//                        //fprintf(stderr, "reg_enum %d, mask %lx, new value is %lx, src %lx, dest %lx, orig_value %lx\n", reg_enum, mask, new_value, src_value, dest_value, orig_value);
//                    }
//
//                } else if(dest->type == OPERAND_TYPE_MEMORY) {
//                    SetValueToMemory((void*)dest->value, dest->size, src_value ^ dest_value);
//                }
//
//                break;
//            }
//            case XED_ICLASS_OR:
//            {
//                // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
//                size_t src_value = GetOperandValue(&operands[1]);
//
//                operand_t* dest = &operands[0];
//                size_t dest_value = GetOperandValue(dest);
//                if(dest->type == OPERAND_TYPE_REGISTER) {
//                    size_t mask = GetRegisterMask(dest);
//
//                    int reg_enum = reg_mapping[dest->name];
//                    if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
//                        size_t orig_value;
//                        if(Is32BitRegister(dest)) {
//                            orig_value = dest_value;
//                        } else {
//                            orig_value = GET_REG_VALUE(context, reg_enum);
//                        }
//                        size_t new_value = ((src_value | dest_value) & mask) | (orig_value & ~mask);
//                        SET_REG_VALUE(context, reg_enum, new_value);
//                        // set signed bits
//                    }
//
//                } else if(dest->type == OPERAND_TYPE_MEMORY) {
//                    SetValueToMemory((void*)dest->value, dest->size, src_value | dest_value);
//                }
//
//                break;
//            }
//            case XED_ICLASS_AND:
//            {
//                // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
//                size_t src_value = GetOperandValue(&operands[1]);
//
//                operand_t* dest = &operands[0];
//                size_t dest_value = GetOperandValue(dest);
//                if(dest->type == OPERAND_TYPE_REGISTER) {
//                    size_t mask = GetRegisterMask(dest);
//
//                    int reg_enum = reg_mapping[dest->name];
//                    if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
//                        size_t orig_value;
//                        if(Is32BitRegister(dest)) {
//                            orig_value = dest_value;
//                        } else {
//                            orig_value = GET_REG_VALUE(context, reg_enum);
//                        }
//                        size_t new_value = ((src_value & dest_value) & mask) | (orig_value & ~mask);
//                        SET_REG_VALUE(context, reg_enum, new_value);
//                        // set signed bits
//                    }
//
//                } else if(dest->type == OPERAND_TYPE_MEMORY) {
//                    SetValueToMemory((void*)dest->value, dest->size, src_value & dest_value);
//                }
//
//                break;
//            }
//                /* assignment instruction  */
//            case XED_ICLASS_LEA:
//                // Move with Zero-Extend
//            case XED_ICLASS_MOVZX:
//            case XED_ICLASS_MOV:
//            {
//                // format: move dest src
//                size_t src_value = GetOperandValue(&operands[1]);
//
//                operand_t* dest = &operands[0];
//                if(dest->type == OPERAND_TYPE_REGISTER) {
//                    size_t mask = GetRegisterMask(dest);
//
//                    int reg_enum = reg_mapping[dest->name];
//                    if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
//                        size_t orig_value;
//                        if(Is32BitRegister(dest)) {
//                            orig_value = dest->value & 0xFFFFFFFF;
//                        } else {
//                            orig_value = dest->value;
//                        }
//                        size_t new_value = (src_value & mask) | (orig_value & ~mask);
//                        SET_REG_VALUE(context, reg_enum, new_value);
//                    }
//
//                } else if(dest->type == OPERAND_TYPE_MEMORY) {
//                    SetValueToMemory((void*)dest->value, dest->size, src_value);
//                }
//
//                break;
//            }
//                // Move with Sign-Extension
//            case XED_ICLASS_MOVSXD:
//            case XED_ICLASS_MOVSX:
//            {
//                // format: move dest(register) src
//                operand_t* src = &operands[1];
//                size_t src_value = GetOperandValue(src);
//                size_t sign_mask = 1UL << (src->size*sizeof(size_t) - 1);
//                bool sign_flag = src_value & sign_mask;
//                if(sign_flag) src_value &= 0xFFFFFFFFFFFFFFFF;
//
//                operand_t* dest = &operands[0];
//                if(dest->type == OPERAND_TYPE_REGISTER) {
//                    size_t dest_mask = GetRegisterMask(dest);
//
//                    int reg_enum = reg_mapping[dest->name];
//                    if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
//                        size_t orig_value;
//                        if(Is32BitRegister(dest)) {
//                            orig_value = dest->value & 0xFFFFFFFF;
//                        } else {
//                            orig_value = dest->value;
//                        }
//                        size_t new_value = (src_value & dest_mask) | (orig_value & ~dest_mask);
//                        SET_REG_VALUE(context, reg_enum, new_value);
//                    }
//                }
//                else {
//                    fprintf(stderr, "The operand of MOVSX is invalid!!!\n");
//                }
//
//                break;
//            }
//            case XED_ICLASS_POP:
//            {
//                operand_t* dest = &operands[0];
//                if(dest->type == OPERAND_TYPE_REGISTER) {
//                    int reg_enum = reg_mapping[dest->name];
//                    if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
//                        size_t orig_value = dest->value;
//                        // the size of the stack pointer is always 64 bits.
//                        int register_size = 8;
//                        if(dest->name >= XED_REG_AX && dest->name <= XED_REG_R15W) {
//                            register_size = 2;
//                        }
//                        size_t rsp_pointer = GET_REG_VALUE(context, REG_RSP);
//                        size_t new_value = *(size_t*)rsp_pointer;
//                        if(register_size == 2) {
//                            new_value &= 0xFFFF;
//                            orig_value &= ~0xFFFF;
//                            new_value |= orig_value;
//                        }
//                        SET_REG_VALUE(context, reg_enum, new_value);
//                        // set the new value
//                        rsp_pointer += register_size;
//                        SET_REG_VALUE(context, REG_RSP, rsp_pointer);
//                    } else {
//                        fprintf(stderr, "POP %d is not supported currently\n", dest->name);
//                    }
//                } else {
//                    fprintf(stderr, "POP m64/m16 is not supported currently\n");
//                }
//                break;
//            }
//                // Move Data After Swapping Bytes
//                //case XED_ICLASS_MOVBE:
//                // Move Doubleword/Move Quadword
//                /*
//                case XED_ICLASS_MOVD:
//                  {
//                    break;
//                  }
//                case XED_ICLASS_MOVQ:
//                  {
//                    break;
//                  }
//                  */
//                // Store Doubleword Using Non-Temporal Hint
//                //case XED_ICLASS_MOVNTI:
//                // Store of Quadword Using Non-Temporal Hint
//                //case XED_ICLASS_MOVNTQ:
//                // Move with Sign-Extension
//                ///case XED_ICLASS_MOVSX:
//                //case XED_ICLASS_MOVSXD:
//                // Move with Zero-Extend
//                //case XED_ICLASS_MOVZX:
//            default:
//            {
//                // check condition moves
//                if(xiclass>=XED_ICLASS_CMOVB && xiclass<=XED_ICLASS_CMOVZ) {
//                    long long flags = ((ucontext_t*)context)->uc_mcontext.gregs[REG_EFL];
//                    long cf_mask = 0x0001;
//                    long pf_mask = 0x0004;
//                    //long af_mask = 0x0010;
//                    long zf_mask = 0x0040;
//                    long sf_mask = 0x0080;
//                    long of_mask = 0x0800;
//
//                    bool isexec = false;
//                    switch(xiclass) {
//                        case XED_ICLASS_CMOVB:
//                        {
//                            // Move if below (CF=1)
//                            isexec = cf_mask & flags;
//                            break;
//                        }
//                        case XED_ICLASS_CMOVBE:
//                        {
//                            // Move if below or equal (CF=1 or ZF=1)
//                            isexec = (cf_mask & flags) || (zf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_CMOVL:
//                        {
//                            // Move if less (SF ≠ OF)
//                            isexec = (sf_mask & flags) ^ (of_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_CMOVLE:
//                        {
//                            // Move if less or equal (ZF=1 or SF ≠ OF)
//                            isexec = (zf_mask & flags) || ((sf_mask & flags) ^ (of_mask & flags));
//                            break;
//                        }
//                        case XED_ICLASS_CMOVNB:
//                        {
//                            // Move if not below (CF=0)
//                            isexec = !(cf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_CMOVNBE:
//                        {
//                            // Move if not below or equal (CF=0 and ZF=0)
//                            isexec = !(cf_mask & flags) && !(zf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_CMOVNL:
//                        {
//                            // Move if not less (SF=OF)
//                            isexec = !((sf_mask & flags) ^ (of_mask & flags));
//                            break;
//                        }
//                        case XED_ICLASS_CMOVNLE:
//                        {
//                            // Move if not less or equal (ZF=0 and SF=OF)
//                            isexec = !(zf_mask & flags) && !((sf_mask & flags) ^ (of_mask & flags));
//                            break;
//                        }
//                        case XED_ICLASS_CMOVNO:
//                        {
//                            // Move if not overflow (OF=0)
//                            isexec = !(of_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_CMOVNP:
//                        {
//                            // Move if not parity (PF=0)
//                            isexec = !(pf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_CMOVNS:
//                        {
//                            // Move if not sign (SF=0)
//                            isexec = !(sf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_CMOVNZ:
//                        {
//                            // Move if not zero (ZF=0)
//                            isexec = !(zf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_CMOVO:
//                        {
//                            // Move if overflow (OF=1)
//                            isexec = of_mask & flags;
//                            break;
//                        }
//                        case XED_ICLASS_CMOVP:
//                        {
//                            // Move if parity (PF=1)
//                            isexec = pf_mask & flags;
//                            break;
//                        }
//                        case XED_ICLASS_CMOVS:
//                        {
//                            // Move if sign (SF=1)
//                            isexec = sf_mask & flags;
//                            break;
//                        }
//                        case XED_ICLASS_CMOVZ:
//                        {
//                            // Move if zero (ZF=1)
//                            isexec = zf_mask & flags;
//                            break;
//                        }
//                        default:
//                        {
//                            fprintf(stderr, "Condition move %d is not supported\n", xiclass);
//                        }
//                    }
//
//                    if(isexec) {
//
//                        // format: move dest src
//                        size_t src_value = GetOperandValue(&operands[1]);
//
//                        operand_t* dest = &operands[0];
//                        if(dest->type == OPERAND_TYPE_REGISTER) {
//                            size_t mask = GetRegisterMask(dest);
//
//                            int reg_enum = reg_mapping[dest->name];
//                            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
//                                size_t orig_value;
//                                if(Is32BitRegister(dest)) {
//                                    orig_value = dest->value & 0xFFFFFFFF;
//                                } else {
//                                    orig_value = dest->value;
//                                }
//                                size_t new_value = (src_value & mask) | (orig_value & ~mask);
//                                SET_REG_VALUE(context, reg_enum, new_value);
//                            }
//
//                        } else if(dest->type == OPERAND_TYPE_MEMORY) {
//                            SetValueToMemory((void*)dest->value, dest->size, src_value);
//                        }
//
//                    }
//
//                }
//                    // check branch instructions
//                else if (xiclass>=XED_ICLASS_JB && xiclass<=XED_ICLASS_JZ){
//                    long long flags = ((ucontext_t*)context)->uc_mcontext.gregs[REG_EFL];
//                    long cf_mask = 0x0001;
//                    long pf_mask = 0x0004;
//                    //long af_mask = 0x0010;
//                    long zf_mask = 0x0040;
//                    long sf_mask = 0x0080;
//                    long of_mask = 0x0800;
//
//                    bool isjump = false;
//                    switch(xiclass) {
//                        case XED_ICLASS_JB:
//                        {
//                            // (CF=1)
//                            isjump = cf_mask & flags;
//                            break;
//                        }
//                        case XED_ICLASS_JBE:
//                        {
//                            // CF=1 or ZF=1
//                            isjump = (cf_mask & flags) || (zf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_JCXZ:
//                        {
//                            // CX register is 0
//                            size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
//                            size_t cx = rcx_value & 0xFFFF;
//                            isjump = (cx == 0);
//                            break;
//                        }
//                        case XED_ICLASS_JECXZ:
//                        {
//                            // ECX register is 0
//                            size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
//                            size_t ecx = rcx_value & 0xFFFFFFFF;
//                            isjump = (ecx == 0);
//                            break;
//                        }
//                        case XED_ICLASS_JL:
//                        {
//                            // SF≠ OF
//                            isjump = (sf_mask & flags) ^ (of_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_JLE:
//                        {
//                            // (ZF=1 or SF ≠ OF)
//                            isjump = (zf_mask & flags) || ((sf_mask & flags) ^ (of_mask & flags));
//                            break;
//                        }
//                        case XED_ICLASS_JNB:
//                        {
//                            // (CF=0)
//                            isjump = !(cf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_JNBE:
//                        {
//                            // (CF=0 and ZF=0)
//                            isjump = !(cf_mask & flags) && !(zf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_JNL:
//                        {
//                            // (SF=OF)
//                            isjump = !((sf_mask & flags) ^ (of_mask & flags));
//                            break;
//                        }
//                        case XED_ICLASS_JNLE:
//                        {
//                            // (ZF=0 and SF=OF)
//                            isjump = !(zf_mask & flags) && !((sf_mask & flags) ^ (of_mask & flags));
//                            break;
//                        }
//                        case XED_ICLASS_JNO:
//                        {
//                            // (OF=0)
//                            isjump = !(of_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_JNP:
//                        {
//                            // (PF=0)
//                            isjump = !(pf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_JNS:
//                        {
//                            // (SF=0)
//                            isjump = !(sf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_JNZ:
//                        {
//                            // (ZF=0)
//                            isjump = !(zf_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_JO:
//                        {
//                            // (OF=1)
//                            isjump = of_mask & flags;
//                            break;
//                        }
//                        case XED_ICLASS_JP:
//                        {
//                            // (PF=1)
//                            isjump = pf_mask & flags;
//                            break;
//                        }
//                        case XED_ICLASS_JRCXZ:
//                        {
//                            // RCX register is 0
//                            size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
//                            isjump = (rcx_value == 0);
//                            break;
//                        }
//                        case XED_ICLASS_JS:
//                        {
//                            // (SF=1)
//                            isjump = sf_mask & flags;
//                            break;
//                        }
//                        case XED_ICLASS_JZ:
//                        {
//                            // (ZF=1)
//                            isjump = zf_mask & flags;
//                            break;
//                        }
//                        case XED_ICLASS_JMP:
//                        case XED_ICLASS_JMP_FAR:
//                        {
//                            isjump = true;
//                            break;
//                        }
//                        default:{}
//                    }
//
//                    if(isjump) {
//                        size_t target = GetOperandValue(&operands[0]);
//                        // far jump
//                        next_instruction = (void*)target;
//                        if(xed3_operand_get_nominal_opcode(xptr) != 0xFF &&
//                           !isapp) {
//                            next_instruction = (void*)(bp->text_mapping->getBase() + target);
//                        }
//                    }
//
//                }
//                else {
//                    fprintf(stderr, "Not support instruction %d\n", xiclass);
//                    exit(-1);
//                }
//            }
//        }
//    }
//
//    return next_instruction;
//}
}

void
scaler::VMEmulator::parseOp(xed_decoded_inst_t *xptr, uint64_t rel_address, scaler::InstrOp *operands, int real_size,
                            void *context) {

}

scaler::VMEmulator::VMEmulator() {
    initRegMapping();
    xed_tables_init();
}

size_t scaler::VMEmulator::getOp(scaler::InstrOp *op) {
    return 0;
}

void scaler::VMEmulator::setMemory(void *address, int size, size_t value) {

}

size_t scaler::VMEmulator::getRegMask(scaler::InstrOp *op) {
    return 0;
}

bool scaler::VMEmulator::is32BitReg(scaler::InstrOp *op) {
    return false;
}

bool
scaler::VMEmulator::chkOpVisibility(const xed_operand_t *op, xed_operand_enum_t op_name, xed_decoded_inst_t *xptr) {
    return false;
}

void scaler::VMEmulator::initRegMapping() {
    regMapping[XED_REG_R8W] = REG_R8;
    regMapping[XED_REG_R9W] = REG_R9;
    regMapping[XED_REG_R10W] = REG_R10;
    regMapping[XED_REG_R11W] = REG_R11;
    regMapping[XED_REG_R12W] = REG_R12;
    regMapping[XED_REG_R13W] = REG_R13;
    regMapping[XED_REG_R14W] = REG_R14;
    regMapping[XED_REG_R15W] = REG_R15;
    regMapping[XED_REG_DI] = REG_RDI;
    regMapping[XED_REG_SI] = REG_RSI;
    regMapping[XED_REG_BP] = REG_RBP;
    regMapping[XED_REG_BX] = REG_RBX;
    regMapping[XED_REG_DX] = REG_RDX;
    regMapping[XED_REG_AX] = REG_RAX;
    regMapping[XED_REG_CX] = REG_RCX;
    regMapping[XED_REG_SP] = REG_RSP;

    regMapping[XED_REG_R8D] = REG_R8;
    regMapping[XED_REG_R9D] = REG_R9;
    regMapping[XED_REG_R10D] = REG_R10;
    regMapping[XED_REG_R11D] = REG_R11;
    regMapping[XED_REG_R12D] = REG_R12;
    regMapping[XED_REG_R13D] = REG_R13;
    regMapping[XED_REG_R14D] = REG_R14;
    regMapping[XED_REG_R15D] = REG_R15;
    regMapping[XED_REG_EDI] = REG_RDI;
    regMapping[XED_REG_ESI] = REG_RSI;
    regMapping[XED_REG_EBP] = REG_RBP;
    regMapping[XED_REG_EBX] = REG_RBX;
    regMapping[XED_REG_EDX] = REG_RDX;
    regMapping[XED_REG_EAX] = REG_RAX;
    regMapping[XED_REG_ECX] = REG_RCX;
    regMapping[XED_REG_ESP] = REG_RSP;

    regMapping[XED_REG_R8] = REG_R8;
    regMapping[XED_REG_R9] = REG_R9;
    regMapping[XED_REG_R10] = REG_R10;
    regMapping[XED_REG_R11] = REG_R11;
    regMapping[XED_REG_R12] = REG_R12;
    regMapping[XED_REG_R13] = REG_R13;
    regMapping[XED_REG_R14] = REG_R14;
    regMapping[XED_REG_R15] = REG_R15;
    regMapping[XED_REG_RDI] = REG_RDI;
    regMapping[XED_REG_RSI] = REG_RSI;
    regMapping[XED_REG_RBP] = REG_RBP;
    regMapping[XED_REG_RBX] = REG_RBX;
    regMapping[XED_REG_RDX] = REG_RDX;
    regMapping[XED_REG_RAX] = REG_RAX;
    regMapping[XED_REG_RCX] = REG_RCX;
    regMapping[XED_REG_RSP] = REG_RSP;
    regMapping[XED_REG_RIP] = REG_RIP;

    regMapping[XED_REG_R8B] = REG_R8;
    regMapping[XED_REG_R9B] = REG_R9;
    regMapping[XED_REG_R10B] = REG_R10;
    regMapping[XED_REG_R11B] = REG_R11;
    regMapping[XED_REG_R12B] = REG_R12;
    regMapping[XED_REG_R13B] = REG_R13;
    regMapping[XED_REG_R14B] = REG_R14;
    regMapping[XED_REG_R15B] = REG_R15;
    regMapping[XED_REG_DIL] = REG_RDI;
    regMapping[XED_REG_SIL] = REG_RSI;
    regMapping[XED_REG_BPL] = REG_RBP;
    regMapping[XED_REG_BL] = REG_RBX;
    regMapping[XED_REG_DL] = REG_RDX;
    regMapping[XED_REG_AL] = REG_RAX;
    regMapping[XED_REG_CL] = REG_RCX;
    regMapping[XED_REG_SPL] = REG_RSP;

    /*
     *  regMapping[XED_REG_EFL] = REG_EFL;
     *  regMapping[XED_REG_CSGSFS] = REG_CSGSFS;
     *  regMapping[XED_REG_ERR] = REG_ERR;
     *  regMapping[XED_REG_TRAPNO] = REG_TRAPNO;
     *  regMapping[XED_REG_OLDMASK] = REG_OLDMASK;
     *  regMapping[XED_REG_CR2] = REG_CR2;
     */

}
