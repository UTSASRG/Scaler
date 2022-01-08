
#include "perthread_decoder.h" 

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

#include "real.hh"
#include "rca_disas.hh"
#include "threadmap.hh"

#ifdef GET_REPLAY_RUNTIME
extern unsigned long trace_inst_count;
#endif

/* Sign-extend a uint64_t value. */
inline static uint64_t sext(uint64_t val, uint8_t sign) {
  uint64_t signbit, mask;
  signbit = 1ull << (sign - 1);
  mask = ~0ull << sign;
  return val & signbit ? val | mask : val & ~mask;
}

inline static size_t getTIP(struct pt_packet &packet, size_t last_ip) {
  size_t ip;
  switch (packet.payload.ip.ipc) {
    case pt_ipc_update_16:
      ip = (packet.payload.ip.ip & 0xFFFFull) | (last_ip & ~0xFFFFull);
      break;
    case pt_ipc_update_32:
      ip = (packet.payload.ip.ip & 0xFFFFFFFFull) | (last_ip & ~0xFFFFFFFFull);
      break;
    case pt_ipc_update_48:
      ip = (packet.payload.ip.ip & 0xFFFFFFFFFFFFull) | (last_ip & ~0xFFFFFFFFFFFFull);
      break;
    case pt_ipc_sext_48:
      //fprintf(stderr, "(FULL) ");
      ip = sext(packet.payload.ip.ip, 48);
      break;
    case pt_ipc_full:
      //fprintf(stderr, "(FULL) ");
      ip = packet.payload.ip.ip;
      break;
    default:
      ip = 0;
  }

  return ip;
}

void PerThreadDecoder::init(struct pt_cpu* cpuinfo, pid_t tid) {
  ptTrace = (uint8_t*)Real::mmap(NULL,  PT_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(ptTrace == MAP_FAILED) {
    fprintf(stderr, "mmap failed for PerThreadDecoder::ptTrace\n");
    exit(-1);
  }

  decode_buffer = (void**)Real::mmap(NULL,  PT_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(decode_buffer == MAP_FAILED) {
    fprintf(stderr, "mmap failed for PerThreadDecoder::decode_buffer\n");
    exit(-1);
  }
  
  tid_ = tid;
  cpuinfo_ = cpuinfo;
  reset();
}

void PerThreadDecoder::reset() {
  last_ip = 0;
  succeeding_offset = -1;
  total_inst = 0;
  last_pkt_inst = 0;
  memset(ptTrace, 0, traceBytes);
  traceBytes = 0;
} 

void PerThreadDecoder::pt_save_trace(size_t size, void* buffer) {
  if(traceBytes + size >  PT_BUFFER_SIZE) {
    fprintf(stderr, "pt buffer size is too small, please set a larger buffer!!!\n");
    exit(-1);
  }
  //fprintf(stderr, "save pt data for thread %d, size %zu\n", tid_, size);

  uint8_t* dest = ptTrace + traceBytes;
  // copy pt data to this buffer
  memcpy(dest, buffer, size);
  // update total bytes
  traceBytes += size;
}

void* PerThreadDecoder::pt_prev_instruction() { 
  if(ptd == NULL) {
    start_decode();
  }
  
  while(total_inst==0) {
    // finish the decoding
    if(succeeding_offset==0) {
      return NULL;
    }
    decode_prev_segment();
    //fprintf(stderr, "Total instructions are %zu\n", total_inst);
  }
  
#ifdef GET_REPLAY_RUNTIME
  trace_inst_count++;
#endif
  return decode_buffer[--total_inst];
}

void PerThreadDecoder::decode_prev_segment() {

  uint64_t offset;  
  //fprintf(stderr, "************decode one segment at %zu**********\n", succeeding_offset);

  // start a new segment
  total_inst = 0;
  last_ip = 0;
  last_pkt_inst = 0;

  bool ignore = false;
  bool flow_update = false;
  int errcode;
  for (;;) {
    struct pt_packet packet;
    // decode the next packet
    errcode = pt_pkt_next(ptd, &packet, sizeof(packet));
    if (errcode == -pte_eos) {
      //fprintf(stderr, "FINISH DECODING: %s\n", pt_errstr(pt_errcode(errcode)));
      break;
    }
    if (errcode < 0) {
      fprintf(stderr, "pt_pkt_next() failed: %s\n", pt_errstr(pt_errcode(errcode)));
      break;
    }

    // check if it is the end of this segment
    errcode = pt_pkt_get_offset(ptd, &offset);
    if (errcode < 0) {
      fprintf(stderr, "pt_pkt_get_offset() failed: %s\n", pt_errstr(pt_errcode(errcode)));
      break;
    }
    //fprintf(stderr, "offset is %lx, succeeding_offset %lx\n", offset, succeeding_offset);
    if(offset == succeeding_offset) {
      //fprintf(stderr, "FINISH CURRENT SEGMENT!!!\n");
      break;
    }

    uint64_t ip = 0;
    // FUP, TIP, TIP.PGE, and TIP.PGD packets can contain an instruction pointer (IP) payload.
    switch (packet.type) {
      // ignore status only trace 
      case ppt_psb:
        {
          ignore = true;
          break;
        }
      case ppt_psbend:
      case ppt_ovf:
        {
          ignore = false;
          break;
        }
        // decode ip packets
      case ppt_fup:
        {
          ip = getTIP(packet, last_ip);
          if(!ignore) {
            adjustTrace((void*)ip);
            flow_update = true;
          }

          //fprintf(stderr, "TIP.FUP is %p\n", (void*)ip);
          break;
        }
      case ppt_tip_pgd:
        {
          /* 
           *If there is an IP payload, then the TIP.PGD 
           should be applied to either the next direct 
           branch whose target matches the TIP.PGD payload, 
           or the next branch that would normally generate 
           a TIP or TNT packet. If there is no IP payload, 
           then the TIP.PGD should apply to the next branch 
           or MOV CR3 instruction.
           * */
          ip = getTIP(packet, last_ip);
          if(ip != 0 && flow_update) {
            appendBlockByTIP((void*)ip);
            flow_update = false;
          }
          //fprintf(stderr, "TIP.PGD is %p\n", (void*)ip);
          break;
        }
      case ppt_tip_pge:
        {
          /* 
           * The IP payload will be the Next IP 
           * of the instruction that changes 
           * context if it is not a branch, 
           * otherwise it will be the target of the branch.
           * */

          // check if it is the target IP is a branch,
          // if so, ip is the target of the branch,
          // otherwise, ignore
          ip = getTIP(packet, last_ip);
          //fprintf(stderr, "TIP.PGE is %p\n", (void*)ip);
          break;
        }
      case ppt_tip:
        {
          ip = getTIP(packet, last_ip);
          //fprintf(stderr, "TIP is %p\n", (void*)ip);
          if(!ignore) {
            if(flow_update) {
              flow_update = false;
            }
            appendBlockByTIP((void*)ip);
          }
          break;
        }
      case ppt_tnt_8:
      case ppt_tnt_64:
        {
          if(!ignore && !flow_update && total_inst!=0) {
            uint8_t bit_size = packet.payload.tnt.bit_size;
            uint64_t current_tnt = packet.payload.tnt.payload;
            //fprintf(stderr, "TNT is %lx\n", (packet.payload.tnt.payload & (0xFFFFFFFFFFFFFFFF >> (64-bit_size))));

            uint64_t index = 1ull << (bit_size - 1);
            while(index!=0) {
              bool taken = (current_tnt & index) != 0;
              checkBranchAndAppend(taken);
              index >>= 1;
            }
          }
          break;
        }
      default:
        {
          //fprintf(stderr, "packet.type %d\n", packet.type);
        }    
    }
    
    // save last ip
    if(ip!=0) {
      last_ip = ip;
    }
  }

  errcode = pt_pkt_get_sync_offset(ptd, &offset);
  if (errcode == 0) {
    succeeding_offset = offset;
  } else {
    succeeding_offset = -1;
  }

  // move backward to the previous segment
  pt_pkt_sync_set(ptd, succeeding_offset);

  errcode = pt_pkt_sync_backward(ptd);
  if (errcode < 0) {
    fprintf(stderr, "pt_pkt_sync_backward() failed: %s\n", pt_errstr(pt_errcode(errcode)));
  }
}

void PerThreadDecoder::start_decode() {
  struct pt_config pt_conf;
  pt_config_init(&pt_conf);
  pt_conf.begin = ptTrace;
  pt_conf.end = ptTrace + traceBytes;
  pt_conf.cpu = *cpuinfo_;

  //fprintf(stderr, "trace bytes is %zu\n", traceBytes);

  int errcode = pt_cpu_errata(&pt_conf.errata, &pt_conf.cpu);
  if (errcode < 0) {
    fprintf(stderr, "pt_errata() failed: %s\n", pt_errstr(pt_errcode(errcode)));
    exit(-1);
  }
  
  ptd = pt_pkt_alloc_decoder(&pt_conf);
  if (ptd == NULL) {
    fprintf(stderr, "pt_pkt_alloc_decoder() failed\n");
    exit(-1);
  }

  errcode = pt_pkt_sync_backward(ptd);
  if (errcode < 0) {
    fprintf(stderr, "pt_pkt_sync_backward() failed: %s\n", pt_errstr(pt_errcode(errcode)));
  }

  last_ip = 0;
  succeeding_offset = -1;
  total_inst = 0;
  last_pkt_inst = 0;
}

void PerThreadDecoder::end_decode() {
  //FIXME destroy all data
  pt_pkt_free_decoder(ptd);
  ptd = NULL;
}

void PerThreadDecoder::adjustTrace(void* ip) {
  size_t index = total_inst;
  while(index>last_pkt_inst) {
    if(decode_buffer[--index] == ip) {
      // index is already decremented
      total_inst = index + 1;
      break;
    }
  }
}

void PerThreadDecoder::appendBlockByTIP(void* ip) {
  // We disable the compression of return address
  last_pkt_inst = total_inst;
  total_inst += RCADisas::getInstance().appendBlockByIP(&decode_buffer[total_inst], ip);
}

void PerThreadDecoder::checkBranchAndAppend(bool istaken) {
  last_pkt_inst = total_inst;
  total_inst += RCADisas::getInstance().checkBranchAndAppend(&decode_buffer[total_inst], decode_buffer[total_inst-1], istaken);
}


