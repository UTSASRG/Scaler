
#include "instbuffer.h"

#include <string>
#include <sys/mman.h>
#include <elf.h>

#include "real.hh"
#include "rca_disas.hh"
#include "selfmap.hh"

void InstBuffer::open_library(const char* path, void** start, int* len){
  int ilen,fd;
  fd = Real::open(path, O_RDONLY); 
  if (fd == -1) {
    fprintf(stderr, "library can not be access: %s\n" , path);
    exit(1);
  }
  ilen = Real::lseek(fd, 0, SEEK_END); // find the size.
  if (ilen == -1) {
    fprintf(stderr, "lseek failed\n");
    exit(1);
  }
  *len = ilen;
  Real::lseek(fd, 0, SEEK_SET); // go to the beginning
  *start = Real::mmap(0, ilen, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
  if(*start == MAP_FAILED){
    fprintf(stderr, "mmap failed\n");
    exit(1);
  }
}

void InstBuffer::read_elf(const char* path){
  void* start;
  int len = 0; 
  open_library(path, &start, &len);

  //fprintf(stderr, "read file %s\n", path);

  Elf64_Ehdr* elf_hdr = (Elf64_Ehdr*) start;

  if (elf_hdr->e_machine != 0x3E){ 
    fprintf(stderr, "library is not EM_X86_64\n");
    exit(1);
  }

  // find section 
  Elf64_Off shoff = elf_hdr->e_shoff;  // section hdr table offset
  Elf64_Shdr* shp = (Elf64_Shdr*) ((char*)start + shoff);
  //Elf64_Half sect_strings  = elf_hdr->e_shstrndx;
  Elf64_Half nsect = elf_hdr->e_shnum;

  unsigned char* hard_limit = (unsigned char*)start + len;

  unsigned int i;
  // FIXME sort section first
  for( i=0;i<nsect;i++) {
    // section is in the end of file, check whether it is the last one 
    if ((unsigned char*) (shp+i) + sizeof(Elf64_Shdr) > hard_limit)
      break;

    if (shp[i].sh_type == SHT_PROGBITS
        && shp[i].sh_flags & SHF_EXECINSTR) {
      RCADisas::getInstance().decode_section(this, mStart, mEnd, shp[i].sh_offset, shp[i].sh_size, shp[i].sh_addr);
    }
  }

  Real::munmap(start, len);
}


void InstBuffer::init(const char* path, void* mappingStart, void* mappingEnd) {
  mStart = mappingStart;
  mEnd = mappingEnd;
  max_inst = (intptr_t)mEnd - (intptr_t)mStart;
  buffer_size = max_inst * sizeof(InstPos);
  // aligned up to pagesize
  int protInfo = PROT_READ | PROT_WRITE;
  int sharedInfo = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;
  start = (InstPos*)Real::mmap(NULL, buffer_size, protInfo, sharedInfo, -1, 0); 
  cur = 0;

  read_elf(path);

  isinit = true;
}

void InstBuffer::addNewInst(uint8_t* iaddr, unsigned int length){
  InstPos* newinst = &start[cur++];
  newinst->iaddr = iaddr;
  newinst->ilength = length;
}

InstPos* InstBuffer::findInstByAddr(uint8_t* iaddr, void* m){

  mapping* mapInfo = (mapping*)m;
  if(!isinit) {
    init(mapInfo->getFile().c_str(), (void*)mapInfo->getBase(), (void*)mapInfo->getLimit());
  }

  InstPos* ret = NULL;
  int low = 0;
  int high = cur-1;
  int mid = 0;
  while(low <= high) {
    mid = (low + high) / 2; 
    if(start[mid].iaddr == iaddr) {
      ret = &start[mid];
      break;
    } else if(start[mid].iaddr < iaddr) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }

  return ret;
}

