/*
 * =====================================================================================
 *
 *       Filename:  vm_emulator.cpp
 *
 *    Description:  
 *
 *         Author:  Hongyu Liu 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "vm_emulator.h"

#include <asm/prctl.h>
#include <sys/syscall.h>

#define OPERAND_NUMBER 3

void* VmEmulator::ExecuteInstruction(breakpoint_t* bp, void* context) {
  void* next_instruction = NULL;
  xed_decoded_inst_t xedd;
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error;

  xed_decoded_inst_zero_set_mode(xptr, &vm_xed_machine_state);

  xed_error = xed_decode(xptr, XED_REINTERPRET_CAST(const xed_uint8_t*, bp->inst_data), bp->inst_length);

  if(xed_error == XED_ERROR_NONE){
    xed_iclass_enum_t xiclass = xed_decoded_inst_get_iclass(xptr);

    operand_t operands[OPERAND_NUMBER];
    xed_uint64_t rel_address =  (xed_uint64_t)bp->addr;
    bool isapp = selfmap::getInstance().isApplication((void*)bp->text_mapping->getBase());
    if(!isapp) { 
      rel_address =  (xed_uint64_t)((intptr_t)bp->addr - bp->text_mapping->getBase());
    }
    ParseOperands(xptr, rel_address, operands, OPERAND_NUMBER, context);

    next_instruction =  (void*)((intptr_t)bp->addr + bp->inst_length);

    switch(xiclass) {
      case XED_ICLASS_SAR:
      case XED_ICLASS_SHL:
        {
          // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
          size_t src_value = GetOperandValue(&operands[1]);

          operand_t* dest = &operands[0];
          size_t dest_value = GetOperandValue(dest);
          if(dest->type == OPERAND_TYPE_REGISTER) {
            size_t mask = GetRegisterMask(dest);

            int reg_enum = reg_mapping[dest->name];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){ 
              size_t orig_value;
              if(Is32BitRegister(dest)) {
                orig_value = dest_value;
              } else {
                orig_value = GET_REG_VALUE(context, reg_enum);
              }
              size_t new_value = ((dest_value << src_value) & mask) | (orig_value & ~mask);
              if(xiclass==XED_ICLASS_SAR) {
                new_value = ((dest_value >> src_value) & mask) | (orig_value & ~mask);
              }
              SET_REG_VALUE(context, reg_enum, new_value); 
              // set signed bits
            }

          } else if(dest->type == OPERAND_TYPE_MEMORY) {
            if(xiclass==XED_ICLASS_SAR) { 
              SetValueToMemory((void*)dest->value, dest->size, dest_value >> src_value);
            } else {
              SetValueToMemory((void*)dest->value, dest->size, dest_value << src_value);
            }
          }

          break;
        }
        /*  mathematical operations */
      case XED_ICLASS_ADD:
        {
          // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
          size_t src_value = GetOperandValue(&operands[1]);

          operand_t* dest = &operands[0];
          size_t dest_value = GetOperandValue(dest);
          if(dest->type == OPERAND_TYPE_REGISTER) {
            size_t mask = GetRegisterMask(dest);

            int reg_enum = reg_mapping[dest->name];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){ 
              size_t orig_value;
              if(Is32BitRegister(dest)) {
                orig_value = dest_value;
              } else {
                orig_value = GET_REG_VALUE(context, reg_enum);
              }
              size_t new_value = ((src_value + dest_value) & mask) | (orig_value & ~mask);
              SET_REG_VALUE(context, reg_enum, new_value); 
              // set signed bits
            }

          } else if(dest->type == OPERAND_TYPE_MEMORY) {
            SetValueToMemory((void*)dest->value, dest->size, src_value + dest_value);
          }

          break;
        }
      case XED_ICLASS_SUB:
        {
          // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
          size_t src_value = GetOperandValue(&operands[1]);

          operand_t* dest = &operands[0];
          size_t dest_value = GetOperandValue(dest);
          if(dest->type == OPERAND_TYPE_REGISTER) {
            size_t mask = GetRegisterMask(dest);

            int reg_enum = reg_mapping[dest->name];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){ 
              size_t orig_value;
              if(Is32BitRegister(dest)) {
                orig_value = dest_value;
              } else {
                orig_value = GET_REG_VALUE(context, reg_enum);
              }
              size_t new_value = ((dest_value - src_value) & mask) | (orig_value & ~mask);
              SET_REG_VALUE(context, reg_enum, new_value); 
              // set signed bits
            }

          } else if(dest->type == OPERAND_TYPE_MEMORY) {
            SetValueToMemory((void*)dest->value, dest->size, dest_value - src_value);
          }

          break;
        }
        /*  logic operations */
      case XED_ICLASS_XOR:
        {
          // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
          size_t src_value = GetOperandValue(&operands[1]);

          operand_t* dest = &operands[0];
          size_t dest_value = GetOperandValue(dest);
          if(dest->type == OPERAND_TYPE_REGISTER) {
            size_t mask = GetRegisterMask(dest);

            int reg_enum = reg_mapping[dest->name];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
              size_t orig_value;
              if(Is32BitRegister(dest)) {
                orig_value = dest_value;
              } else {
                orig_value = GET_REG_VALUE(context, reg_enum);
              }
              size_t new_value = ((src_value ^ dest_value) & mask) | (orig_value & ~mask);
              SET_REG_VALUE(context, reg_enum, new_value); 
              // set signed bits
              //fprintf(stderr, "reg_enum %d, mask %lx, new value is %lx, src %lx, dest %lx, orig_value %lx\n", reg_enum, mask, new_value, src_value, dest_value, orig_value);
            }

          } else if(dest->type == OPERAND_TYPE_MEMORY) {
            SetValueToMemory((void*)dest->value, dest->size, src_value ^ dest_value);
          }

          break;
        }
      case XED_ICLASS_OR:
        {
          // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
          size_t src_value = GetOperandValue(&operands[1]);

          operand_t* dest = &operands[0];
          size_t dest_value = GetOperandValue(dest);
          if(dest->type == OPERAND_TYPE_REGISTER) {
            size_t mask = GetRegisterMask(dest);

            int reg_enum = reg_mapping[dest->name];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
              size_t orig_value;
              if(Is32BitRegister(dest)) {
                orig_value = dest_value;
              } else {
                orig_value = GET_REG_VALUE(context, reg_enum);
              }
              size_t new_value = ((src_value | dest_value) & mask) | (orig_value & ~mask);
              SET_REG_VALUE(context, reg_enum, new_value); 
              // set signed bits
            }

          } else if(dest->type == OPERAND_TYPE_MEMORY) {
            SetValueToMemory((void*)dest->value, dest->size, src_value | dest_value);
          }

          break;
        }
      case XED_ICLASS_AND:
        {
          // The OF, SF, ZF, AF, CF, and PF flags are set according to the result.
          size_t src_value = GetOperandValue(&operands[1]);

          operand_t* dest = &operands[0];
          size_t dest_value = GetOperandValue(dest);
          if(dest->type == OPERAND_TYPE_REGISTER) {
            size_t mask = GetRegisterMask(dest);

            int reg_enum = reg_mapping[dest->name];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
              size_t orig_value;
              if(Is32BitRegister(dest)) {
                orig_value = dest_value;
              } else {
                orig_value = GET_REG_VALUE(context, reg_enum);
              }
              size_t new_value = ((src_value & dest_value) & mask) | (orig_value & ~mask);
              SET_REG_VALUE(context, reg_enum, new_value); 
              // set signed bits
            }

          } else if(dest->type == OPERAND_TYPE_MEMORY) {
            SetValueToMemory((void*)dest->value, dest->size, src_value & dest_value);
          }

          break;
        }
        /* assignment instruction  */
      case XED_ICLASS_LEA: 
      // Move with Zero-Extend
      case XED_ICLASS_MOVZX:
      case XED_ICLASS_MOV: 
        {
          // format: move dest src
          size_t src_value = GetOperandValue(&operands[1]);

          operand_t* dest = &operands[0];
          if(dest->type == OPERAND_TYPE_REGISTER) {
            size_t mask = GetRegisterMask(dest);

            int reg_enum = reg_mapping[dest->name];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){ 
              size_t orig_value;
              if(Is32BitRegister(dest)) {
                orig_value = dest->value & 0xFFFFFFFF;
              } else {
                orig_value = dest->value;
              }
              size_t new_value = (src_value & mask) | (orig_value & ~mask);
              SET_REG_VALUE(context, reg_enum, new_value); 
            }

          } else if(dest->type == OPERAND_TYPE_MEMORY) {
            SetValueToMemory((void*)dest->value, dest->size, src_value);
          }

          break;
        }
      // Move with Sign-Extension
      case XED_ICLASS_MOVSXD:
      case XED_ICLASS_MOVSX:
        {
          // format: move dest(register) src
          operand_t* src = &operands[1];
          size_t src_value = GetOperandValue(src);
          size_t sign_mask = 1UL << (src->size*sizeof(size_t) - 1);
          bool sign_flag = src_value & sign_mask;
          if(sign_flag) src_value &= 0xFFFFFFFFFFFFFFFF;

          operand_t* dest = &operands[0];
          if(dest->type == OPERAND_TYPE_REGISTER) {
            size_t dest_mask = GetRegisterMask(dest);

            int reg_enum = reg_mapping[dest->name];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){ 
              size_t orig_value;
              if(Is32BitRegister(dest)) {
                orig_value = dest->value & 0xFFFFFFFF;
              } else {
                orig_value = dest->value;
              }
              size_t new_value = (src_value & dest_mask) | (orig_value & ~dest_mask);
              SET_REG_VALUE(context, reg_enum, new_value); 
            }
          } 
          else {
            fprintf(stderr, "The operand of MOVSX is invalid!!!\n");
          }

          break;
        }
      case XED_ICLASS_POP:
        {
          operand_t* dest = &operands[0];
          if(dest->type == OPERAND_TYPE_REGISTER) {
            int reg_enum = reg_mapping[dest->name];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){ 
              size_t orig_value = dest->value;
              // the size of the stack pointer is always 64 bits.
              int register_size = 8;
              if(dest->name >= XED_REG_AX && dest->name <= XED_REG_R15W) {
                register_size = 2;
              }
              size_t rsp_pointer = GET_REG_VALUE(context, REG_RSP);
              size_t new_value = *(size_t*)rsp_pointer;
              if(register_size == 2) {
                new_value &= 0xFFFF;
                orig_value &= ~0xFFFF;
                new_value |= orig_value;
              }
              SET_REG_VALUE(context, reg_enum, new_value); 
              // set the new value
              rsp_pointer += register_size;
              SET_REG_VALUE(context, REG_RSP, rsp_pointer); 
            } else {
              fprintf(stderr, "POP %d is not supported currently\n", dest->name);
            }
          } else {
            fprintf(stderr, "POP m64/m16 is not supported currently\n");
          }
          break;
        }
        // Move Data After Swapping Bytes
      //case XED_ICLASS_MOVBE:
        // Move Doubleword/Move Quadword
      /*
      case XED_ICLASS_MOVD:
        {
          break;
        }
      case XED_ICLASS_MOVQ:
        {
          break;
        }
        */
        // Store Doubleword Using Non-Temporal Hint
      //case XED_ICLASS_MOVNTI:
        // Store of Quadword Using Non-Temporal Hint
      //case XED_ICLASS_MOVNTQ:
        // Move with Sign-Extension
      ///case XED_ICLASS_MOVSX:
      //case XED_ICLASS_MOVSXD:
        // Move with Zero-Extend
      //case XED_ICLASS_MOVZX:
      default:
        {
          // check condition moves
          if(xiclass>=XED_ICLASS_CMOVB && xiclass<=XED_ICLASS_CMOVZ) {
            long long flags = ((ucontext_t*)context)->uc_mcontext.gregs[REG_EFL]; 
            long cf_mask = 0x0001;
            long pf_mask = 0x0004;
            //long af_mask = 0x0010;
            long zf_mask = 0x0040;
            long sf_mask = 0x0080;
            long of_mask = 0x0800;

            bool isexec = false;
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
                  fprintf(stderr, "Condition move %d is not supported\n", xiclass);
                }
            }

            if(isexec) {

              // format: move dest src
              size_t src_value = GetOperandValue(&operands[1]);

              operand_t* dest = &operands[0];
              if(dest->type == OPERAND_TYPE_REGISTER) {
                size_t mask = GetRegisterMask(dest);

                int reg_enum = reg_mapping[dest->name];
                if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){ 
                  size_t orig_value;
                  if(Is32BitRegister(dest)) {
                    orig_value = dest->value & 0xFFFFFFFF;
                  } else {
                    orig_value = dest->value;
                  }
                  size_t new_value = (src_value & mask) | (orig_value & ~mask);
                  SET_REG_VALUE(context, reg_enum, new_value); 
                }

              } else if(dest->type == OPERAND_TYPE_MEMORY) {
                SetValueToMemory((void*)dest->value, dest->size, src_value);
              }

            }

          } 
          // check branch instructions
          else if (xiclass>=XED_ICLASS_JB && xiclass<=XED_ICLASS_JZ){
            long long flags = ((ucontext_t*)context)->uc_mcontext.gregs[REG_EFL]; 
            long cf_mask = 0x0001;
            long pf_mask = 0x0004;
            //long af_mask = 0x0010;
            long zf_mask = 0x0040;
            long sf_mask = 0x0080;
            long of_mask = 0x0800;

            bool isjump = false;
            switch(xiclass) {
              case XED_ICLASS_JB:
                {
                  // (CF=1)
                  isjump = cf_mask & flags;
                  break;
                }
              case XED_ICLASS_JBE:
                {
                  // CF=1 or ZF=1
                  isjump = (cf_mask & flags) || (zf_mask & flags);
                  break;
                }
              case XED_ICLASS_JCXZ:
                {
                  // CX register is 0
                  size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
                  size_t cx = rcx_value & 0xFFFF;
                  isjump = (cx == 0);
                  break;
                }
              case XED_ICLASS_JECXZ:
                {
                  // ECX register is 0
                  size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
                  size_t ecx = rcx_value & 0xFFFFFFFF;
                  isjump = (ecx == 0);
                  break;
                }
              case XED_ICLASS_JL:
                {
                  // SF≠ OF
                  isjump = (sf_mask & flags) ^ (of_mask & flags);
                  break;
                }
              case XED_ICLASS_JLE:
                {
                  // (ZF=1 or SF ≠ OF)
                  isjump = (zf_mask & flags) || ((sf_mask & flags) ^ (of_mask & flags));
                  break;
                }
              case XED_ICLASS_JNB:
                {
                  // (CF=0)
                  isjump = !(cf_mask & flags);
                  break;
                }
              case XED_ICLASS_JNBE:
                {
                  // (CF=0 and ZF=0)
                  isjump = !(cf_mask & flags) && !(zf_mask & flags);
                  break;
                }
              case XED_ICLASS_JNL:
                {
                  // (SF=OF)
                  isjump = !((sf_mask & flags) ^ (of_mask & flags));
                  break;
                }
              case XED_ICLASS_JNLE:
                {
                  // (ZF=0 and SF=OF)
                  isjump = !(zf_mask & flags) && !((sf_mask & flags) ^ (of_mask & flags));
                  break;
                }
              case XED_ICLASS_JNO:
                {
                  // (OF=0)
                  isjump = !(of_mask & flags);
                  break;
                }
              case XED_ICLASS_JNP:
                {
                  // (PF=0)
                  isjump = !(pf_mask & flags);
                  break;
                }
              case XED_ICLASS_JNS:
                {
                  // (SF=0)
                  isjump = !(sf_mask & flags);
                  break;
                }
              case XED_ICLASS_JNZ:
                {
                  // (ZF=0)
                  isjump = !(zf_mask & flags);
                  break;
                }
              case XED_ICLASS_JO:
                {
                  // (OF=1)
                  isjump = of_mask & flags;
                  break;
                }
              case XED_ICLASS_JP:
                {
                  // (PF=1)
                  isjump = pf_mask & flags;
                  break;
                }
              case XED_ICLASS_JRCXZ:
                {
                  // RCX register is 0
                  size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
                  isjump = (rcx_value == 0);
                  break;
                }
              case XED_ICLASS_JS:
                {
                  // (SF=1)
                  isjump = sf_mask & flags;
                  break;
                }
              case XED_ICLASS_JZ:
                {
                  // (ZF=1)
                  isjump = zf_mask & flags;
                  break;
                }
              case XED_ICLASS_JMP:
              case XED_ICLASS_JMP_FAR:
                {
                  isjump = true;
                  break;
                }
              default:{}
            }

            if(isjump) {
              size_t target = GetOperandValue(&operands[0]);
              // far jump
              next_instruction = (void*)target;
              if(xed3_operand_get_nominal_opcode(xptr) != 0xFF && 
                  !isapp) { 
                  next_instruction = (void*)(bp->text_mapping->getBase() + target);
              }
            }

          }
          else {
            fprintf(stderr, "Not support instruction %d\n", xiclass);
            exit(-1);
          }
        }
    }
  }

  return next_instruction;
}

void VmEmulator::SetValueToMemory(void* address, int size, size_t value) {
  switch(size) {
    case 1:
      {
        *(char*)address = (char)value;
        break;
      }
    case 2:
      {
        *(short*)address = (short)value;
        break;
      }
    case 4:
      {
        *(int*)address = (int)value;
        break;
      }
    case 8:
      {
        *(size_t*)address = value;
        break;
      }
    default:
      {
        fprintf(stderr, "size %d is not correct!!!\n", size);                 
      }
  }
}

// 32-bit operands generate a 32-bit result, 
// zero-extended to a 64-bit result in the destination general-purpose register.
bool VmEmulator::Is32BitRegister(operand_t* operand) {
  bool is_32bit = false;
  if(operand->name >= XED_REG_EAX && operand->name <= XED_REG_R15D) {
    is_32bit = true; 
  }
  
  return is_32bit;
}

size_t VmEmulator::GetRegisterMask(operand_t* operand) {
  size_t mask = 0;
  if(operand->name >= XED_REG_AX && operand->name <= XED_REG_R15W) {
    mask = 0xFFFF; 
  } else if(operand->name >= XED_REG_EAX && operand->name <= XED_REG_R15D) {
    mask = 0xFFFFFFFF; 
  } else if(operand->name >= XED_REG_RAX && operand->name <= XED_REG_R15) {
    mask = 0xFFFFFFFFFFFFFFFF; 
  } else if (operand->name >= XED_REG_AL && operand->name <= XED_REG_R15B) {
    mask = 0xFF; 
  } else {
    fprintf(stderr, "Not support (A, B, C, D)H register\n");
  }
  //else if (dest->name >= XED_REG_AH && dest->name <= XED_REG_BH) {
  //  mask = 0xFF00; 
  //}
  return mask;
}

size_t VmEmulator::GetOperandValue(operand_t* operand) {
  size_t op_value = operand->value;
  if(operand->type == OPERAND_TYPE_MEMORY) {
    // get memory value 
    // if this is the last word of current mapping, it could be a SEGV
    op_value = *(size_t*)operand->value;
    // get right value based on size
    // FIXME Little Endian !!! 
    op_value &= (0xFFFFFFFFFFFFFFFF >> ((8-operand->size)*sizeof(size_t)));
  } else if(operand->type == OPERAND_TYPE_REGISTER) {
    size_t mask = GetRegisterMask(operand);
    op_value &= mask;
  }
  return op_value;
}

void VmEmulator::ParseOperands(xed_decoded_inst_t *xptr, xed_uint64_t rel_address, operand_t* operands, int real_size, void* context) {
  const xed_inst_t *xi = xed_decoded_inst_inst(xptr);
  int real_noperands = 0;
  // decode operands: register, memory address, immediate value
  unsigned int noperands = xed_inst_noperands(xi);
  for(int i=0; i<noperands; i++) {
    const xed_operand_t* op = xed_inst_operand(xi, i);
    const xed_operand_values_t* ov = xed_decoded_inst_operands_const(xptr);
    xed_operand_enum_t op_name = xed_operand_name(op);

    if(!check_operand_visibility(op, op_name, xptr)){
      continue;
    }

    if(real_noperands >= real_size) {
      fprintf(stderr, "Only support %d operands\n", real_size);
      abort();
    }

    switch(op_name) {
      case XED_OPERAND_AGEN:
      case XED_OPERAND_MEM0: 
      case XED_OPERAND_MEM1:
        {
          int memop = -1;
          if(op_name == XED_OPERAND_AGEN) {
            operands[real_noperands].type = OPERAND_TYPE_ADDRESS;
            operands[real_noperands].name = OPERAND_TYPE_ADDRESS;
            memop = 0;
          } else {
            operands[real_noperands].type = OPERAND_TYPE_MEMORY;
            operands[real_noperands].name = OPERAND_TYPE_MEMORY;
            if(op_name == XED_OPERAND_MEM0) {
              memop = 0; 
            } else if(op_name == XED_OPERAND_MEM1) {
              memop = 1; 
            }
          }

          xed_int64_t addr = 0;
          xed_reg_enum_t base, index, seg;
          base = xed_decoded_inst_get_base_reg(xptr, memop);
          index = xed_decoded_inst_get_index_reg(xptr, memop);
          seg = xed_decoded_inst_get_seg_reg(xptr, memop);
          unsigned long long base_r_val = 0;
          unsigned long long index_r_val = 0;

          xed_int64_t disp = xed_operand_values_get_memory_displacement_int64(ov);
          xed_uint_t bytes = xed_decoded_inst_operand_length_bits(xptr, i)>>3;

          // FIXME segment register
          //size_t segment_register;
          //asm volatile("mov %%fs:0x0, %[Var]" : [Var] "=r" (segment_register));
          if(seg == XED_REG_FS) {
            unsigned long real_fs_addr;
            syscall(SYS_arch_prctl, ARCH_GET_FS, &real_fs_addr);
            addr += real_fs_addr;
          }

          if(base != XED_REG_INVALID) {
            int reg_enum = reg_mapping[base];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){ 
              if(reg_enum == REG_RIP) {
                xed_uint64_t instruction_length = xed_decoded_inst_get_length(xptr);
                base_r_val = rel_address + instruction_length; 
              } else {
                base_r_val = GET_REG_VALUE(context, reg_enum); 
              }
              addr += base_r_val;
            }
          } 
          if(index != XED_REG_INVALID) {
            int reg_enum = reg_mapping[index];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){
              if(reg_enum == REG_RIP) {
                xed_uint64_t instruction_length = xed_decoded_inst_get_length(xptr);
                index_r_val = rel_address + instruction_length;;
              } else {
                index_r_val = GET_REG_VALUE(context, reg_enum); 
              }
              xed_bits_t scale = xed3_operand_get_scale(xptr);
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
      case XED_OPERAND_IMM0:
        {
          xed_uint64_t imm = xed_operand_values_get_immediate_uint64(ov);

          operands[real_noperands].type = OPERAND_TYPE_IMM;
          operands[real_noperands].name = XED_OPERAND_IMM0;
          operands[real_noperands].value = imm;
          operands[real_noperands].size = 8;

          break;
        }
      case XED_OPERAND_IMM1:
        {
          xed_uint64_t imm = xed3_operand_get_uimm1(xptr);
          
          operands[real_noperands].type = OPERAND_TYPE_IMM;
          operands[real_noperands].name = XED_OPERAND_IMM1;
          operands[real_noperands].value = imm;
          operands[real_noperands].size = 8;
          
          break;
        }
      case XED_OPERAND_PTR:
        {
          unsigned int disp =(unsigned int) xed_operand_values_get_branch_displacement_int32(ov); 

          operands[real_noperands].type = OPERAND_TYPE_ADDRESS;
          operands[real_noperands].name = XED_OPERAND_PTR;
          operands[real_noperands].value = disp;
          operands[real_noperands].size = 8;

          break;
        }
      case XED_OPERAND_RELBR:
        {
          xed_int64_t disp = xed3_operand_get_disp(xptr);
          xed_uint64_t instruction_length = xed_decoded_inst_get_length(xptr);
          xed_uint64_t pc = rel_address + instruction_length;
          xed_uint64_t effective_addr = (xed_uint64_t) ((xed_int64_t)pc  + disp);

          operands[real_noperands].type = OPERAND_TYPE_ADDRESS;
          operands[real_noperands].name = XED_OPERAND_RELBR;
          operands[real_noperands].value = effective_addr;
          operands[real_noperands].size = 8;
          
          break;
        }
      default:
        {
          bool is_register = xed_operand_is_register(op_name);
          bool is_memory_register = xed_operand_is_memory_addressing_register(op_name);
          if(is_register || is_memory_register) {
            xed_reg_enum_t r = xed_decoded_inst_get_reg(xptr, op_name);
            int reg_enum = reg_mapping[r];
            if(reg_enum >= REG_R8 && reg_enum <= REG_RIP){ 
              size_t val = GET_REG_VALUE(context, reg_enum);

              if(is_register) {
                operands[real_noperands].type = OPERAND_TYPE_REGISTER;
              } else {
                operands[real_noperands].type = OPERAND_TYPE_MEMORY;
              }
              operands[real_noperands].name = r;
              operands[real_noperands].value = val;
              operands[real_noperands].size = xed_decoded_inst_operand_length(xptr, i);
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

bool VmEmulator::GetInstructionInformation(breakpoint_t* bp) {
  xed_decoded_inst_t xedd;
  xed_decoded_inst_t *xptr = &xedd;
  xed_error_enum_t xed_error;

  xed_decoded_inst_zero_set_mode(xptr, &vm_xed_machine_state);

  xed_error = xed_decode(xptr, XED_REINTERPRET_CAST(const xed_uint8_t*, bp->addr), XED_MAX_INSTRUCTION_BYTES);

  bool ret = false;
  if(xed_error == XED_ERROR_NONE){ 
    bp->inst_length = xed_decoded_inst_get_length(xptr);
    // backup original data
    int length = bp->inst_length;
    char* dest = bp->inst_data;
    char* src = (char*)(bp->addr);
    for(int i=0; i<length; i++) {
      dest[i] = src[i];
    }
    // TRY to avoid functions from other library 
    //memcpy(bp->inst_data, bp->addr, bp->inst_length);
    ret = true;
  }

  return ret;
}

bool VmEmulator::check_operand_visibility(const xed_operand_t* op, xed_operand_enum_t op_name, xed_decoded_inst_t *xptr) {
  bool isvisibile = true;
  if (xed_operand_operand_visibility(op) == XED_OPVIS_SUPPRESSED) {
    /*   allow a fall through to print the memop for stringops to match dumpbin */
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
