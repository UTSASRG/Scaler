
#include "xdefines.hh"

#define UNW_LOCAL_ONY
//#include <libunwind.h>
#include <libunwind-x86_64.h>
//static int my_backtrace(sFrame* stack, int d) {
int my_backtrace(sFrame* stack, int d) {
  unw_cursor_t cursor; unw_context_t uc;
  unw_word_t ip, sp, bp;
  int r = 0;
  unw_getcontext(&uc);
  unw_init_local(&cursor, &uc);
  while ((unw_step(&cursor) > 0) && (d-- > 0)) {
    unw_get_reg(&cursor, UNW_REG_IP, &ip);
    unw_get_reg(&cursor, UNW_REG_SP, &sp);
    unw_get_reg(&cursor, UNW_X86_64_RBP, &bp);
    //printf ("ip = %lx, sp = %lx, bp = %lx\n", (long) ip, (long) sp, (long) bp);
    if(ip == 0) {
      break;
    }
   
    stack[r].regs[REG_RIP] = ip;
    stack[r].regs[REG_RSP] = sp;
    stack[r].regs[REG_RBP] = bp;
    r++;
  }
  return r;
}

