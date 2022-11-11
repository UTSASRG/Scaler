
#include <util/tool/VMEmulator.h>
#include <asm/prctl.h>
#include <sys/syscall.h>
#include <exceptions/ScalerException.h>
#include <unistd.h>

static xed_state_t vm_xed_machine_state = {
        XED_MACHINE_MODE_LONG_64,
        XED_ADDRESS_WIDTH_64b
};

#define OPERAND_NUMBER 3

void scaler::VMEmulator::getInstrInfo(scaler::Breakpoint &bp) {
    xed_decoded_inst_zero_set_mode(&bp.xedDecodedInst, &vm_xed_machine_state);
    xed_error_enum_t xed_error;

    xed_error = xed_decode((xed_decoded_inst_t *) &bp.xedDecodedInst,
                           (const xed_uint8_t *) XED_REINTERPRET_CAST(const xed_uint8_t*, bp.addr),
                           (const unsigned int) XED_MAX_INSTRUCTION_BYTES);

    if (xed_error == XED_ERROR_NONE) {
        bp.instLen = xed_decoded_inst_get_length(&bp.xedDecodedInst);
        //Extract instruction type
        bp.xiclass = xed_decoded_inst_get_iclass(&bp.xedDecodedInst);

        // backup original data
//        for (int i = 0; i < bp.instLen; i++) {
//            bp.oriCode[i] = ((char *) bp.addr)[i];
//
//            DBG_LOGS("%d-%d,", bp.oriCode[i],bp.addr[i]);
//        }

        // TRY to avoid functions from other library
        //memcpy(bp->inst_data, bp->addr, bp->inst_length);

    } else {
        fatalError("Intel xed fail to decode instruction, meaning the instruction is unrecognizable");
    }
}

void *scaler::VMEmulator::executeInstr(scaler::Breakpoint &bp, ucontext_t *context) {

    return nullptr;

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
//                            // Move if less (SF ` OF)
//                            isexec = (sf_mask & flags) ^ (of_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_CMOVLE:
//                        {
//                            // Move if less or equal (ZF=1 or SF ` OF)
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
//                            // SF` OF
//                            isjump = (sf_mask & flags) ^ (of_mask & flags);
//                            break;
//                        }
//                        case XED_ICLASS_JLE:
//                        {
//                            // (ZF=1 or SF ` OF)
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
scaler::VMEmulator::parseOp(xed_decoded_inst_t &decodedInst, uint64_t rel_address, scaler::InstrOp *operands,
                            int real_size,
                            void *context) {
    const xed_inst_t *xi = xed_decoded_inst_inst(&decodedInst);
    int real_noperands = 0;
    // decode operands: register, memory address, immediate value
    unsigned int noperands = xed_inst_noperands(xi);
    for (int i = 0; i < noperands; i++) {
        const xed_operand_t *op = xed_inst_operand(xi, i);
        const xed_operand_values_t *ov = xed_decoded_inst_operands_const(&decodedInst);
        xed_operand_enum_t op_name = xed_operand_name(op);

        if (!chkOpVisibility(op, op_name, &decodedInst)) {
            continue;
        }

        if (real_noperands >= real_size) {
            fprintf(stderr, "Only support %d operands\n", real_size);
            abort();
        }

        switch (op_name) {
            case XED_OPERAND_AGEN:
            case XED_OPERAND_MEM0:
            case XED_OPERAND_MEM1: {
                int memop = -1;
                if (op_name == XED_OPERAND_AGEN) {
                    operands[real_noperands].instrType = InstrOp::OPERAND_TYPE_ADDRESS;
                    operands[real_noperands].name = InstrOp::OPERAND_TYPE_ADDRESS;
                    memop = 0;
                } else {
                    operands[real_noperands].instrType = InstrOp::OPERAND_TYPE_MEMORY;
                    operands[real_noperands].name = InstrOp::OPERAND_TYPE_MEMORY;
                    if (op_name == XED_OPERAND_MEM0) {
                        memop = 0;
                    } else if (op_name == XED_OPERAND_MEM1) {
                        memop = 1;
                    }
                }

                xed_int64_t addr = 0;
                xed_reg_enum_t base, index, seg;
                base = xed_decoded_inst_get_base_reg(&decodedInst, memop);
                index = xed_decoded_inst_get_index_reg(&decodedInst, memop);
                seg = xed_decoded_inst_get_seg_reg(&decodedInst, memop);
                unsigned long long base_r_val = 0;
                unsigned long long index_r_val = 0;

                xed_int64_t disp = xed_operand_values_get_memory_displacement_int64(ov);
                xed_uint_t bytes = xed_decoded_inst_operand_length_bits(&decodedInst, i) >> 3;

                // FIXME segment register
                //size_t segment_register;
                //asm volatile("mov %%fs:0x0, %[Var]" : [Var] "=r" (segment_register));
                if (seg == XED_REG_FS) {
                    unsigned long real_fs_addr;
                    syscall(SYS_arch_prctl, ARCH_GET_FS, &real_fs_addr);
                    addr += real_fs_addr;
                }

                if (base != XED_REG_INVALID) {
                    int reg_enum = regMapping[base];
                    if (reg_enum >= REG_R8 && reg_enum <= REG_RIP) {
                        if (reg_enum == REG_RIP) {
                            xed_uint64_t instruction_length = xed_decoded_inst_get_length(&decodedInst);
                            base_r_val = rel_address + instruction_length;
                        } else {
                            base_r_val = GET_REG_VALUE(context, reg_enum);
                        }
                        addr += base_r_val;
                    }
                }
                if (index != XED_REG_INVALID) {
                    int reg_enum = regMapping[index];
                    if (reg_enum >= REG_R8 && reg_enum <= REG_RIP) {
                        if (reg_enum == REG_RIP) {
                            xed_uint64_t instruction_length = xed_decoded_inst_get_length(&decodedInst);
                            index_r_val = rel_address + instruction_length;;
                        } else {
                            index_r_val = GET_REG_VALUE(context, reg_enum);
                        }
                        xed_bits_t scale = xed3_operand_get_scale(&decodedInst);
                        index_r_val *= XED_STATIC_CAST(xed_uint_t, scale);
                        addr += index_r_val;
                    }
                }

                xed_bool_t no_base_index = (base == XED_REG_INVALID) && (index == XED_REG_INVALID);
                if (xed_operand_values_has_memory_displacement(ov)) {
                    unsigned int disp_bits = xed_operand_values_get_memory_displacement_length_bits(ov);
                    if (disp_bits && (disp || no_base_index)) {
                        addr += disp;
                    }
                }

                operands[real_noperands].value = addr;
                operands[real_noperands].size = bytes;

                break;
            }
            case XED_OPERAND_IMM0: {
                xed_uint64_t imm = xed_operand_values_get_immediate_uint64(ov);

                operands[real_noperands].instrType = InstrOp::OPERAND_TYPE_IMM;
                operands[real_noperands].name = XED_OPERAND_IMM0;
                operands[real_noperands].value = imm;
                operands[real_noperands].size = 8;

                break;
            }
            case XED_OPERAND_IMM1: {
                xed_uint64_t imm = xed3_operand_get_uimm1(&decodedInst);

                operands[real_noperands].instrType = InstrOp::OPERAND_TYPE_IMM;
                operands[real_noperands].name = XED_OPERAND_IMM1;
                operands[real_noperands].value = imm;
                operands[real_noperands].size = 8;

                break;
            }
            case XED_OPERAND_PTR: {
                unsigned int disp = (unsigned int) xed_operand_values_get_branch_displacement_int32(ov);

                operands[real_noperands].instrType = InstrOp::OPERAND_TYPE_ADDRESS;
                operands[real_noperands].name = XED_OPERAND_PTR;
                operands[real_noperands].value = disp;
                operands[real_noperands].size = 8;

                break;
            }
            case XED_OPERAND_RELBR: {
                xed_int64_t disp = xed3_operand_get_disp(&decodedInst);
                xed_uint64_t instruction_length = xed_decoded_inst_get_length(&decodedInst);
                xed_uint64_t pc = rel_address + instruction_length;
                xed_uint64_t effective_addr = (xed_uint64_t) ((xed_int64_t) pc + disp);

                operands[real_noperands].instrType = InstrOp::OPERAND_TYPE_ADDRESS;
                operands[real_noperands].name = XED_OPERAND_RELBR;
                operands[real_noperands].value = effective_addr;
                operands[real_noperands].size = 8;

                break;
            }
            default: {
                bool is_register = xed_operand_is_register(op_name);
                bool is_memory_register = xed_operand_is_memory_addressing_register(op_name);
                if (is_register || is_memory_register) {
                    xed_reg_enum_t r = xed_decoded_inst_get_reg(&decodedInst, op_name);
                    int reg_enum = regMapping[r];
                    if (reg_enum >= REG_R8 && reg_enum <= REG_RIP) {
                        size_t val = GET_REG_VALUE(context, reg_enum);

                        if (is_register) {
                            operands[real_noperands].instrType = InstrOp::OPERAND_TYPE_REGISTER;
                        } else {
                            operands[real_noperands].instrType = InstrOp::OPERAND_TYPE_MEMORY;
                        }
                        operands[real_noperands].name = r;
                        operands[real_noperands].value = val;
                        operands[real_noperands].size = xed_decoded_inst_operand_length(&decodedInst, i);
                    }

                } else {
                    fprintf(stderr, "Can not support %d op_name\n", op_name);
                    abort();
                }
            }
        }
        ++real_noperands;
    }
}

scaler::VMEmulator::VMEmulator() {
    initRegMapping();
    xed_tables_init();
}

size_t scaler::VMEmulator::getOp(scaler::InstrOp &op) {
    size_t op_value = op.value;
    if (op.instrType == InstrOp::Type::OPERAND_TYPE_MEMORY) {
        //DBG_LOGS("Error scene=%p",op.value);
        // get memory value
        // if this is the last word of current mapping, it could be a SEGV
        op_value = *(size_t *) op.value;
        // get right value based on size
        // FIXME Little Endian !!!
        op_value &= (0xFFFFFFFFFFFFFFFF >> ((8 - op.size) * sizeof(size_t)));
    } else if (op.instrType == InstrOp::Type::OPERAND_TYPE_REGISTER) {
        size_t mask = getRegMask(op);
        op_value &= mask;
    }
    return op_value;
}

void scaler::VMEmulator::setMemory(void *address, int size, ssize_t value) {

}

size_t scaler::VMEmulator::getRegMask(scaler::InstrOp &op) {
    size_t mask = 0;
    if (op.name >= XED_REG_AX && op.name <= XED_REG_R15W) {
        mask = 0xFFFF;
    } else if (op.name >= XED_REG_EAX && op.name <= XED_REG_R15D) {
        mask = 0xFFFFFFFF;
    } else if (op.name >= XED_REG_RAX && op.name <= XED_REG_R15) {
        mask = 0xFFFFFFFFFFFFFFFF;
    } else if (op.name >= XED_REG_AL && op.name <= XED_REG_R15B) {
        mask = 0xFF;
    } else {
        fatalError("Not support (A, B, C, D)H register\n");
    }
    //else if (dest->name >= XED_REG_AH && dest->name <= XED_REG_BH) {
    //  mask = 0xFF00;
    //}
    return mask;
}

bool scaler::VMEmulator::is32BitReg(scaler::InstrOp &op) {
    return false;
}

bool
scaler::VMEmulator::chkOpVisibility(const xed_operand_t *op, xed_operand_enum_t op_name, xed_decoded_inst_t *xptr) {
    bool isvisibile = true;
    if (xed_operand_operand_visibility(op) == XED_OPVIS_SUPPRESSED) {
        /*   allow a fall through to print the memop for stringops to match dumpbin */
        xed_bool_t stringop = (xed_decoded_inst_get_category(xptr) == XED_CATEGORY_STRINGOP);
        if (stringop) {
            if (op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1) {
            } else {
                isvisibile = false;
            }
        } else {
            isvisibile = false;
        }
    }

    return isvisibile;
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

