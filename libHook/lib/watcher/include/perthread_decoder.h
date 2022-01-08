#if !defined(PERTHREAD_DECODER_H)
#define PERTHREAD_DECODER_H

#include <stdlib.h>
#include <stdio.h>
#include <new>

#include "xdefines.hh"
#include "real.hh"

#include <intel-pt.h>

#define PT_BUFFER_SIZE 1024*1024*1024

class PerThreadDecoder {
  public:

    void init(struct pt_cpu* cpuinfo, pid_t tid);

    pid_t getTid() { return tid_; }
    void setPerfFd(int perffd) { perfFd = perffd; }
    int getPerfFd() { return perfFd; }
    void setPerfMapping(void* mapping) { perfMapping = (uint8_t*)mapping; }
    void* getPerfMapping() { return perfMapping; }
    void setPerfPTMapping(void* ptmapping) { perfPTMapping = (uint8_t*)ptmapping; }
    void* getPerfPTMapping() { return perfPTMapping; }

    void reset();

    // save trace data
    void pt_save_trace(size_t size, void* buffer);

    //void* ptdata_next_instruction();
    void* pt_prev_instruction();
    
    // search the last segment
    void start_decode();
    // clearn up decoder
    void end_decode();

    void adjustTrace(void* ip);
    void appendBlockByTIP(void* ip);
    void checkBranchAndAppend(bool istaken);

  private:

    // iterate segment reversely and decode one segment into instructions
    void decode_prev_segment();

    struct pt_cpu* cpuinfo_;

    /* record pt race */
    pid_t tid_;
    int perfFd;
    uint8_t* perfMapping;
    uint8_t* perfPTMapping;

    /* decode pt race */
    struct pt_packet_decoder* ptd;
    // trace bytes
    size_t traceBytes;
    uint8_t* ptTrace; // 1G by default

    // save last ip
    size_t last_ip;
    // record succeeding sync point offset
    uint64_t succeeding_offset;
    // we break the trace into instructions
    size_t total_inst;
    size_t last_pkt_inst;
    void** decode_buffer; // 1G by default
};

#endif
