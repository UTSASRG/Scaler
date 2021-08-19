
#include "pt_manager.h" 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/mman.h>
#include <signal.h>
#include <asm/unistd.h>
#include <errno.h>

#include "real.hh"
#include "threadmap.hh"

static long perf_event_open(struct perf_event_attr* hw_event, 
    pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, (uintptr_t)pid, 
      (uintptr_t)cpu, (uintptr_t)group_fd, (uintptr_t)flags);
}

inline static void getCpuInfo(struct pt_cpu * cpuinfo) {
  FILE* f = Real::fopen("/proc/cpuinfo", "rb");
  if (!f) {
    fprintf(stderr, "Couldn't open '/proc/cpuinfo'\n");
    exit(-1);
  }

  for (;;) {
    char k[1024], t[1024], v[1024];
    int ret = fscanf(f, "%1023[^\t]%1023[\t]: %1023[^\n]\n", k, t, v);
    if (ret == EOF) {
      break;
    }
    if (ret != 3) {
      break;
    }
    if (strcmp(k, "vendor_id") == 0) {
      if (strcmp(v, "GenuineIntel") == 0) {
        cpuinfo->vendor = pcv_intel;
      }
    }
    if (strcmp(k, "cpu family") == 0) {
      cpuinfo->family = atoi(v);
    }
    if (strcmp(k, "model") == 0) {
      cpuinfo->model = atoi(v);
    }
    if (strcmp(k, "stepping") == 0) {
      cpuinfo->stepping = atoi(v);
    }
  }
  Real::fclose(f);
}

void PTManager::pt_Init() {
  static char const intel_pt_path[] = "/sys/bus/event_source/devices/intel_pt/type";
  if(Real::access(intel_pt_path, F_OK) != -1) {
    uint8_t buf[256];
    int fd = Real::open(intel_pt_path, O_RDONLY | O_CLOEXEC);
    int bytes = Real::read(fd, buf, 255);
    Real::close(fd);
    if(bytes == 0) intel_pt_type = 6; // default is 6;
    intel_pt_type = (int)strtol((char*)buf, NULL, 10); 
  } else {
    fprintf(stderr, "Can not support Intel PT\n");
    exit(-1);
  }

  //fprintf(stderr, "pt_type %d\n", intel_pt_type);

  cpuinfo.vendor = pcv_unknown;
  cpuinfo.family = 0;
  cpuinfo.model = 0;
  cpuinfo.stepping = 0;
  getCpuInfo(&cpuinfo);
}

bool PTManager::pt_OpenAll() {
  bool ret = true;
  threadmap::aliveThreadIterator ti;
  for(ti = threadmap::getInstance().begin(); ti != threadmap::getInstance().end(); ti++) {
    thread_t* thread = ti.getThread();
    // init first
    per_thread_decoder[thread->index].init(&cpuinfo, thread->tid);
    // start a pt event
    ret &= pt_OpenByThread(thread->index);
  }
  return ret;
}

void PTManager::pt_CloseAll() {
  threadmap::aliveThreadIterator ti;
  for(ti = threadmap::getInstance().begin(); ti != threadmap::getInstance().end(); ti++) {
    thread_t* thread = ti.getThread();
    pt_CloseByThread(thread->index);
  }
}

bool PTManager::pt_EnableAll() {
  bool ret = true;
  threadmap::aliveThreadIterator ti;
  for(ti = threadmap::getInstance().begin(); ti != threadmap::getInstance().end(); ti++) {
    thread_t* thread = ti.getThread();
    ret &= pt_EnableByThread(thread->index);
  }
  return ret;
}

bool PTManager::pt_DisableAll() {
  bool ret = true;
  threadmap::aliveThreadIterator ti;
  for(ti = threadmap::getInstance().begin(); ti != threadmap::getInstance().end(); ti++) {
    thread_t* thread = ti.getThread();
    ret &= pt_DisableByThread(thread->index);
  }
  return ret;
}

void PTManager::pt_ReadPTDataByThread(int tindex) {
  pt_DisableByThread(tindex);
  PerThreadDecoder* decoder = getDecoderByThread(tindex);
  uint8_t* perfMapping = (uint8_t *)decoder->getPerfMapping();
  uint8_t* perfPTMapping = (uint8_t *)decoder->getPerfPTMapping();
  // copy trace data to a buffer 
  struct perf_event_mmap_page* pemp = (struct perf_event_mmap_page*)perfMapping;
  uint64_t aux_tail = pemp->aux_tail;
  uint64_t aux_head = pemp->aux_head;
  //fprintf(stderr, "aux_head %lu, aux_tail %lu\n", aux_head, aux_tail);
  size_t bytes = aux_head - aux_tail;
  uint8_t* src = &perfPTMapping[aux_tail];
  decoder->pt_save_trace(bytes, src);
  pt_EnableByThread(tindex); 
}

bool PTManager::pt_OpenByThread(int tindex) {
  PerThreadDecoder* decoder = getDecoderByThread(tindex);
  pid_t tid = decoder->getTid();
  int perfFd = 0;
  uint8_t* perfMapping = NULL;
  uint8_t* perfPTMapping = NULL;

  struct perf_event_attr pe;
  memset(&pe, 0, sizeof(struct perf_event_attr));

  pe.size = sizeof(struct perf_event_attr);
  pe.type = intel_pt_type;
  pe.config = RTIT_CTL_DISRETC; 
  //pe.config = RTIT_CTL_MTC_EN | RTIT_CTL_DISRETC; 
  //pe.config = RTIT_CTL_TRACEEN | RTIT_CTL_DISRETC; 

  //**********Test Code*********
  //pe.type = PERF_TYPE_HARDWARE;
  //pe.config = PERF_COUNT_HW_INSTRUCTIONS; 

  /*
  pe.sample_type = PERF_SAMPLE_IP|PERF_SAMPLE_TID|PERF_SAMPLE_TIME|PERF_SAMPLE_CPU|PERF_SAMPLE_IDENTIFIER; 
  pe.sample_period = 1;
  pe.read_format = PERF_FORMAT_ID;
  pe.sample_id_all = 1;

  pe.type = 1;
  pe.config = 0x9;
  pe.sample_period = 1;
  pe.task = 1;

  // this only works for monitoring fd
  pe.watermark = 1;
  pe.wakeup_watermark = 1;

  *******************
  
  pe.watermark = 1;
  pe.wakeup_watermark = 1;
  //pe.sample_period = 1;
  pe.aux_watermark = UINT32_MAX;
  //pe.sample_id_all=1;
  */

  pe.disabled = 1;
  pe.enable_on_exec = 1;
  
  /*
  */
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.exclude_idle = 1;
  pe.exclude_guest = 1;

  perfFd = perf_event_open(&pe, tid, -1, -1, 0);
  //fprintf(stderr, "perf_event_open for pid %d, %d\n", tid, perfFd);
  if(perfFd == -1) {
    fprintf(stderr, "can not open perf\n");
    exit(-1);
  }

  perfMapping = (uint8_t*)Real::mmap(NULL, PERF_MAP_SIZE + PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, perfFd, 0);
  if(perfMapping == MAP_FAILED) {
    fprintf(stderr, "mmap failed for ring buffer\n");
    exit(-1);
  }
  
  /* 
  */
  struct perf_event_mmap_page*  pemp = (struct perf_event_mmap_page*)perfMapping;
  pemp->aux_offset = pemp->data_offset + pemp->data_size;
  pemp->aux_size = PERF_AUX_SIZE;
  //fprintf(stderr, "pemp->data_offset %llu, pemp->data_siz %llu, pemp->aux_size %llu\n", 
  //    pemp->data_offset, pemp->data_size, pemp->aux_size);
  
  perfPTMapping = (uint8_t*)Real::mmap(NULL, pemp->aux_size, PROT_READ | PROT_WRITE, MAP_SHARED, perfFd, pemp->aux_offset);
  if(perfPTMapping == MAP_FAILED) {
    fprintf(stderr, "mmap failed for PT ring buffer\n");
    exit(-1);
  }

  // configure perf event 
  //if(fcntl(perfFd, F_SETFL, O_RDWR | O_NONBLOCK | O_ASYNC) == -1) {
  if(Real::fcntl(perfFd, F_SETFL, fcntl(perfFd, F_GETFL) | O_NONBLOCK |  O_ASYNC) == -1) {
    fprintf(stderr, "Failed to set perf event file to ASYNC mode\n");
    exit(-1);
  }
  if(Real::fcntl(perfFd, F_SETSIG, SIGIO) == -1) {
    fprintf(stderr, "Failed to set perf event file's async signal: %d\n", SIGIO);
    exit(-1);
  }
  //struct f_owner_ex owner = {F_OWNER_TID, getpid()};
  struct f_owner_ex owner = {F_OWNER_TID, tid};
  if(Real::fcntl(perfFd, F_SETOWN_EX, &owner) == -1) {
    fprintf(stderr, "Failed to set the owner of the perf event file\n");
    exit(-1);
  }

  /*
  //if(ioctl(perfFd, PERF_EVENT_IOC_RESET, 0) == -1) {
  if(ioctl(perfFd, PERF_EVENT_IOC_REFRESH, 1) == -1) {
    fprintf(stderr, "Failed to reset perf: %d\n", errno); 
    exit(-1);
  }
  */

  /*
   * Address range filtering: limiting the data to certain
   * instruction address ranges. Filters are ioctl()ed to us from
   * userspace as ascii strings.
   *
   * Filter string format:
   *
   * ACTION RANGE_SPEC
   * where ACTION is one of the
   *  * "filter": limit the trace to this region
   *  * "start": start tracing from this address
   *  * "stop": stop tracing at this address/region;
   * RANGE_SPEC is
   *  * for kernel addresses: <start address>[/<size>]
   *  * for object files:     <start address>[/<size>]@</path/to/object/file>
   *
   * if <size> is not specified or is zero, the range is treated as a single
   * address; not valid for ACTION=="filter".
  char filter[] = "filter 0x4000000/0x10000000000@/home/hongyuliu/workspace/intelpt/mypt/test";
  if(ioctl(perfFd, PERF_EVENT_IOC_SET_FILTER, filter) == -1) {
    fprintf(stderr, "Failed to set filter\n"); 
    exit(-1);
  }
  char filter1[] = "start 0x4000000/0x10000000000@/home/hongyuliu/workspace/intelpt/mypt/test";
  if(ioctl(perfFd, PERF_EVENT_IOC_SET_FILTER, filter1) == -1) {
    fprintf(stderr, "Failed to set filter\n"); 
    exit(-1);
  }
   */

  decoder->setPerfFd(perfFd);
  decoder->setPerfMapping(perfMapping);
  decoder->setPerfPTMapping(perfPTMapping);

  //fprintf(stderr, "enable Intel PT for tid %d: perfFd %d\n", tid, perfFd);

  return true;
}

void PTManager::pt_CloseByThread(int tindex) {
  pt_DisableByThread(tindex);
  PerThreadDecoder* decoder = getDecoderByThread(tindex);
  Real::munmap(decoder->getPerfMapping(), PERF_MAP_SIZE + PAGE_SIZE);
  Real::munmap(decoder->getPerfPTMapping(), PERF_AUX_SIZE);
  Real::close(decoder->getPerfFd());
}

bool PTManager::pt_EnableByThread(int tindex) {
  PerThreadDecoder* decoder = getDecoderByThread(tindex);
  if(Real::ioctl(decoder->getPerfFd(), PERF_EVENT_IOC_ENABLE, 0) == -1) {
    fprintf(stderr, "Failed to enable perf event\n");
    exit(-1);
  }
  return true;
}

bool PTManager::pt_DisableByThread(int tindex) {
  PerThreadDecoder* decoder = getDecoderByThread(tindex);
  if(Real::ioctl(decoder->getPerfFd(), PERF_EVENT_IOC_DISABLE, 0) == -1) {
    //fprintf(stderr, "Failed to disable perf event, fd %d\n", perfFd);
	}
  return true;
}

void copy_from_ring_buffer(struct perf_event_mmap_page* mapping,
    ptrdiff_t index, void* dest, size_t bytes) {
  uintptr_t base = reinterpret_cast<uintptr_t>(mapping) + PAGE_SIZE;
  size_t start_index = index % PERF_MAP_SIZE;
  size_t end_index = start_index + bytes;

  if(end_index <= PERF_MAP_SIZE) {
    memcpy(dest, reinterpret_cast<void*>(base + start_index), bytes);
  } else {
    size_t chunk2_size = end_index - PERF_MAP_SIZE;
    size_t chunk1_size = bytes - chunk2_size;

    void* chunk2_dest = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(dest) + chunk1_size);

    memcpy(dest, reinterpret_cast<void*>(base + start_index), chunk1_size);
    memcpy(chunk2_dest, reinterpret_cast<void*>(base), chunk2_size);
  }
}

bool has_data(struct perf_event_mmap_page* mapping, size_t head, size_t index, size_t* recordsize) {
  // If there is no ring buffer, there is no data
  if(mapping == nullptr) {
    //fprintf(stderr, "mmap is null\n");
    return false;
  }

  // If there isn't enough data in the ring buffer to hold a header, there is no data
  if(index + sizeof(struct perf_event_header) >= head) {
    //fprintf(stderr, "no header, index %zu, head %zu\n", index, head);
    return false;
  }

  struct perf_event_header hdr;
  copy_from_ring_buffer(mapping, index, &hdr, sizeof(struct perf_event_header));

  // If the first record is larger than the available data, nothing can be read
  if(index + hdr.size > head) {
    //fprintf(stderr, "no data, index %zu, head %zu\n", index, head);
    return false;
  }

  // return size
  *recordsize = hdr.size;
  return true;
}

bool get_perf_record(struct perf_event_mmap_page* _mapping, uint8_t* _buf, size_t index, size_t size) {
  // get data
  copy_from_ring_buffer(_mapping, index, _buf, size);
  struct perf_event_header *event = (struct perf_event_header *)_buf;
  uintptr_t p = reinterpret_cast<uintptr_t>(event) + sizeof(struct perf_event_header);
  fprintf(stderr, "event->type %d\n", event->type);
  if(event->type == PERF_RECORD_AUX) { 
    uint64_t aux_offset = *(uint64_t*)p;
    p += sizeof(uint64_t);
    uint64_t aux_size = *(uint64_t*)p;
    p += sizeof(uint64_t);
    uint64_t flags = *(uint64_t*)p;
    fprintf(stderr, "aux_offset %lu, aux_size %lu, flags %lu\n", aux_offset, aux_size, flags);
    return true;
  } else if(event->type == PERF_RECORD_ITRACE_START) {
    uint32_t pid = *(uint32_t*)p;
    p += sizeof(uint32_t);
    uint32_t tid = *(uint32_t*)p;
    p += sizeof(uint32_t);
    fprintf(stderr, "pid: %d, tid: %d\n", pid, tid);
  }
  return false;
}

void PTManager::pt_PrintRingBufferByThread(int tindex) {
  PerThreadDecoder* decoder = getDecoderByThread(tindex);
  struct perf_event_mmap_page* _mapping = (struct perf_event_mmap_page*)decoder->getPerfMapping();
  if(_mapping == MAP_FAILED){
    fprintf(stderr, "mmap falied\n");
    exit(-1);
  }
  size_t head = _mapping->data_head;
  size_t index = _mapping->data_tail;
  uint8_t _buf[4096];

  fprintf(stderr, "====> Read DATA area:\n");
  size_t recordsize = 0;
  while(has_data(_mapping, head, index, &recordsize)) {
    get_perf_record(_mapping, _buf, index, recordsize);
    // bump to next
    index += recordsize;
  }
  _mapping->data_tail = index;
  fprintf(stderr, "=====================\n");
}

