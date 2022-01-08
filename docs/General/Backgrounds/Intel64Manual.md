[Intel manual link](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.pdf)

The documentation is pretty lengthy. We only need to understand part of it. To better understand the content, we first need to be familiar with the notation.

## Notation Convention

### Bit and Byte Order

All tables in this brochure follows this convention. Pay attention to the byte order and the address direction.

![image-20210520135140254](https://user-images.githubusercontent.com/19838874/148663883-d44d3a91-f259-485e-aa71-b75ebfe64c0d.png)


### Instruction format

Check Page 104 for a complete description.

To understand how to use an instruction, understanding instruction and description is enough.

**Instruction column:**

- rel8: Relative address in the rage from 128 bytes before the end of the instruction to 127 bytes after the end of the instruction.
- rel16, rel32: Relative address within the same code segment as the instruction assembled. rel16 means operators is 16 bit, and rel32 means operator size is 32 bit. Compared to rel8m it has to be within the same code segment.
- ptr16:16, ptr16:32: A far pointer, typically ptr16 specifies the base address of a different code segment than the current one. The second 16 and 32 means the offset within that segment. The difference is whether the offset is 16bit-long or 32bit-long.
- r8: One byte GPRs (AL,CL,DL,BL,AH,CH,DH,BH,BPL,SPL,DIL,SIL) or one of the byte registers (R8L-R15L)
- r16: One of the world GPRs (AX,CX,DX,BX,SP,BP,SI,DI) or one of the word registers (R8-R15)
- r32: One of the doubleword GPRs (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI); Or one of the doubleword registers (R8D-R15D)
- r64: One of the quadword GPRs (RAX, RBX, RCX, RDX, RDI, RSI, RBP, RSP, R8-R15)
- imm8: Immedite byte
- imm16: immediate word (2 bytes)
- imm32: immediate double word (4 bytes)
- imm64: immediate quad word (8 bytes)
- r/m8: Either r8 or imm8
- r/m16: Either r16 or imm16
- r/m32: Either r32 or imm32
- r/m64: Either r64 or imm64
- m: Either 16, 32, 64 bit operand in memory
- m8: One byte operand in memory
- m16: A word in memory
- m32: A DWORD in memory
- m64: A QWORD in memory
- m128: A DOUBLE QWORD in memory
- m16:16, m16:32 & m16:64: Left number is pointer's segment selector.
- m16&32, m16&16, m32&32, m16&64: A memory operand consisting of data item pairs whose sizes are indicated on the left and the right side of '&'
- moffs8, moffs16, moffs32, moffs64: Memory offset of type byte, word, doubleword and qword used by some variants of mov. The actual addr is given by simple offset relative to segment base.
- Sreg: Segment register (ES,CS,SS,DS,FS,GS)
- m32fp, m64fp, m80fp: Single precision, double precision, and double extended-precision floating-point operand in memory. These symbols  designate floating-point values that are used as operands for x87 FPU ops
- ST or ST(0): Top element of the FPU register **stack**
- ST(i): The $i^{th}$ element from the top of the FPU register stack ($i\leftarrow 0\ through\ 7$)
- mm: The MMX register. (MM0 to MM7)
- mm/m32: The lower 32 bits of an MMX register or a 32-bit **memory** operand. 
- mm/m64: An MMX register or a 64-bit **memory** operand. (MM0-MM7)
- xmm: An XMM register. (XMM0-XMM7) (XMM8-XMM15) are available using REX.R in 64-bit mode
- xmm/m32: An XMM register. or 32 bit **memory** operand.
- xmm/m64: 64-bit memory operand. (XMM0-XMM7) (XMM8-XMM15) The contents of memory are found at the address provided by the effective address computation.
- xmm/m128: XMM register or 64-bit memory operand. (XMM0-XMM7) (XMM8-XMM15)
- \<XMM0\> The implied use of XMM0. xmm1 means the first source op and xmm2 stands for the second op
- ymm: YMM registers (YMM0-YMM7) (YMM8-YMM15)
- m256: 32-byte operand in memory.
- ymm/m256: ymm or m256
- \<YMM0\>: Implicit use of YMM0 
- bnd: 128-bit bounds register (BND0-BND3)
- mib: Memory operand using SIB addressing form.
- m512: 64-byte operand in memory
- zmm/m512: zmm or m512
- {k1}{z}: Mask register used as instruction writemask.
- {k1}: Mask register used as instruction writemask. (Has slight difference than {k1}{z})
- k1: Mask register used as a regular operand (k0-k7)
- mV: Vector memory operand. (size depend on the instruction)
- vm32{x,y,z}: A vector array of memory operands, specified using VSIB memory addressing. Vector index register is either XMM.YMM or ZMM.
- vm64{x,y,z}: Similar to vm32 but in 64-bits
- zmm/m512/m32bcst: zmm or m512 loaded from 32-bit memory location
- zmm/m512/m64bcst: zmm or m512 loaded from 64-bit memory location
- \<ZMM0\>: Implicit use of ZMM0
- {er}: Indicates support for embedded rounding control and also support SAE(suppress all exceptions).
- {sae}: Indicates support for SAE(suppress all exceptions), but doesn't support embedded rounding control.
- SRC1: The first source operand in the instruction syntax encoded with the VEX/EVEX prefix and have two or more source operands.
- SRC2: The second ...
- SRC3: The third ...
- SRC: The source in single-source instruction
- DST: The destination register

# Intel and AT&T

Previously I wrote in ATT format. I'll suggest to use intel. Because ATT doesn't have lots of documentations.

### Switch to intel

#### In GAS

```
.intel_syntax 
```
#### In GCC

```
-masm=intel
```

#### In GDB

```
set disassembly-flavor intel
```

### Switch to AT&T

#### In GAS

```
.att_syntax
```

#### In GCC

```
-masm=intel
```

#### In GDB

```
set disassembly-flavor att
```
