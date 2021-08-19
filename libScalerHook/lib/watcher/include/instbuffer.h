#if !defined(INSTBUFFER_H)
#define INSTBUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct instpos {
  uint8_t* iaddr;
  unsigned int ilength;
} InstPos;

class InstBuffer {

  public:

    InstBuffer() {
      isinit = false;
    }
    
    bool isInit() { return isinit; }

    void init(const char* path, void* mappingStart, void* mappingEnd);

    void addNewInst(uint8_t* iaddr, unsigned int ilength);

    InstPos* findInstByAddr(uint8_t* iaddr, void* m);

  private:

    void read_elf(const char* path);

    void open_library(const char* path, void** start, int* len);

    bool isinit;
    InstPos* start;
    size_t max_inst;
    size_t buffer_size;
    size_t cur;
    void* mStart;
    void* mEnd;
};

#endif
