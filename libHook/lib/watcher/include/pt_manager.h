#if !defined(PT_MANAGER_H)
#define PT_MANAGER_H

#include <stdlib.h>
#include <stdio.h>
#include <new>

#include "xdefines.hh"
#include "perthread_decoder.h"

#include <intel-pt.h>

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif

#define PAGE_SIZE 4096
#define PERF_MAP_SIZE (PAGE_SIZE * 16)
#define PERF_AUX_SIZE (PAGE_SIZE * 4096 * 4)

// IA32_RTIT_CTL MSRs can be set by config member of perf_event_attr
#define RTIT_CTL_TRACEEN BIT(0)
#define RTIT_CTL_CYCLEACC BIT(1)
#define RTIT_CTL_OS BIT(2)
#define RTIT_CTL_USR BIT(3)
#define RTIT_CTL_PWR_EVT_EN BIT(4)
#define RTIT_CTL_FUP_ON_PTW BIT(5)
#define RTIT_CTL_CR3EN BIT(7)
#define RTIT_CTL_TOPA BIT(8)
#define RTIT_CTL_MTC_EN BIT(9)
#define RTIT_CTL_TSC_EN BIT(10)
#define RTIT_CTL_DISRETC BIT(11)
#define RTIT_CTL_PTW_EN BIT(12)
#define RTIT_CTL_BRANCH_EN BIT(13)
#define RTIT_CTL_MTC_RANGE_OFFSET 14
#define RTIT_CTL_MTC_RANGE (0x0full << RTIT_CTL_MTC_RANGE_OFFSET)
#define RTIT_CTL_CYC_THRESH_OFFSET 19
#define RTIT_CTL_CYC_THRESH (0x0full << RTIT_CTL_CYC_THRESH_OFFSET)
#define RTIT_CTL_PSB_FREQ_OFFSET 24
#define RTIT_CTL_PSB_FREQ (0x0full << RTIT_CTL_PSB_FREQ_OFFSET)
#define RTIT_CTL_ADDR0_OFFSET 32
#define RTIT_CTL_ADDR0 (0x0full << RTIT_CTL_ADDR0_OFFSET)
#define RTIT_CTL_ADDR1_OFFSET 36
#define RTIT_CTL_ADDR1 (0x0full << RTIT_CTL_ADDR1_OFFSET)
#define RTIT_CTL_ADDR2_OFFSET 40
#define RTIT_CTL_ADDR2 (0x0full << RTIT_CTL_ADDR2_OFFSET)
#define RTIT_CTL_ADDR3_OFFSET 44
#define RTIT_CTL_ADDR3 (0x0full << RTIT_CTL_ADDR3_OFFSET)
// IA32_RTIT_STATUS MSRs
#define RTIT_STATUS_FILTEREN BIT(0)
#define RTIT_STATUS_CONTEXTEN BIT(1)
#define RTIT_STATUS_TRIGGEREN BIT(2)
#define RTIT_STATUS_BUFFOVF BIT(3)
#define RTIT_STATUS_ERROR BIT(4)
#define RTIT_STATUS_STOPPED BIT(5)

class PTManager {
  public:
    static PTManager& getInstance() {
      static char pt_buf[sizeof(PTManager)];
      static PTManager* theOneObject = new (pt_buf) PTManager();
      return *theOneObject;
    }
    
    bool pt_OpenAll();
    void pt_CloseAll();
    bool pt_EnableAll();
    bool pt_DisableAll();
    void pt_ReadPTDataByThread(int tindex);

    void pt_PrintRingBufferByThread(int tindex);
    void pt_Decode();

    PerThreadDecoder* getDecoderByThread(int tindex) { return &per_thread_decoder[tindex]; }
      
  private:
    PTManager() {
      pt_Init();
    }

    void pt_Init();

    bool pt_OpenByThread(int tindex);
    void pt_CloseByThread(int tindex);
    bool pt_EnableByThread(int tindex);
    bool pt_DisableByThread(int tindex);

    int intel_pt_type;
    struct pt_cpu cpuinfo;

    PerThreadDecoder per_thread_decoder[xdefines::MAX_ALIVE_THREADS];
};

#endif
