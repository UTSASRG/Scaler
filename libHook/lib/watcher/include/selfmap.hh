#if !defined(DOUBLETAKE_SELFMAP_H)
#define DOUBLETAKE_SELFMAP_H

/*
 * @file   selfmap.h
 * @brief  Process the /proc/self/map file.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <functional>
#include <map>
#include <new>
#include <string>
#include <utility>

#include "interval.hh"
#include "mm.hh"
#include "real.hh"
#include "instbuffer.h"

// From heaplayers
#include "wrappers/stlallocator.h"

using namespace std;

struct regioninfo {
  void* start;
  void* end;
};

/**
 * A single mapping parsed from the /proc/self/maps file
 */
class mapping {
public:
  mapping() : _valid(false) {}

  mapping(uintptr_t base, uintptr_t limit, char* perms, size_t offset, std::string file)
      : _valid(true), _base(base), _limit(limit), _readable(perms[0] == 'r'),
        _writable(perms[1] == 'w'), _executable(perms[2] == 'x'), _copy_on_write(perms[3] == 'p'),
        _offset(offset), _file(file) {}

  bool valid() const { return _valid; }

  bool isText() const { return _readable && !_writable && _executable; }

  //bool isStack() const { return _file == "[stack]"; }
  bool isStack() const { return _file.find("[stack") != std::string::npos; }

  bool isGlobals(std::string mainfile) const {
    // global mappings are RW_P, and either the heap, or the mapping is backed
    // by a file (and all files have absolute paths)
		// the file is the current executable file, with [heap], or with lib*.so
		// Actually, the mainfile can be longer if it has some parameters.
	 return (_readable && _writable && !_executable && _copy_on_write) &&
           (_file.size() > 0 && (_file == mainfile ||  _file == "[heap]" || _file.find(".so") != std::string::npos));
  }
  
  //maybe it is global area
  bool isGlobalsExt() const {
	 return _readable && _writable && !_executable && _copy_on_write && _file.size() == 0;
  }

  uintptr_t getBase() const { return _base; }

  uintptr_t getLimit() const { return _limit; }

  const std::string& getFile() const { return _file; }

  size_t getOffset() { return _offset; }

  InstBuffer* getInstBuffer() { return &ib; }

private:
  bool _valid;
  uintptr_t _base;
  uintptr_t _limit;
  bool _readable;
  bool _writable;
  bool _executable;
  bool _copy_on_write;
  size_t _offset;
  std::string _file;
  InstBuffer ib;
};

/// Read a mapping from a file input stream
static std::ifstream& operator>>(std::ifstream& f, mapping& m) {
  if(f.good() && !f.eof()) {
    uintptr_t base, limit;
    char perms[5];
    size_t offset;
    size_t dev_major, dev_minor;
    int inode;
    string path;

    // Skip over whitespace
    f >> std::skipws;

    // Read in "<base>-<limit> <perms> <offset> <dev_major>:<dev_minor> <inode>"
    f >> std::hex >> base;
    if(f.get() != '-')
      return f;
    f >> std::hex >> limit;

    if(f.get() != ' ')
      return f;
    f.get(perms, 5);

    f >> std::hex >> offset;
    f >> std::hex >> dev_major;
    if(f.get() != ':')
      return f;
    f >> std::hex >> dev_minor;
    f >> std::dec >> inode;

    // Skip over spaces and tabs
    while(f.peek() == ' ' || f.peek() == '\t') {
      f.ignore(1);
    }

    // Read out the mapped file's path
    getline(f, path);

    m = mapping(base, limit, perms, offset, path);
  }

  return f;
}

class selfmap {
public:
  static selfmap& getInstance() {
    static char buf[sizeof(selfmap)];
    static selfmap* theOneTrueObject = new (buf) selfmap();
    return *theOneTrueObject;
  }

  /// Check whether an address is inside the DoubleTake library itself.
  bool isDoubleTakeLibrary(void* pcaddr) {
    return ((pcaddr >= _doubletakeStart) && (pcaddr <= _doubletakeEnd));
  }

  bool isDlLibrary(void* pcaddr) {
    return ((pcaddr >= _dlStart) && (pcaddr <= _dlEnd));
  }

  bool isLibcLibrary(void* pcaddr) {
    return ((pcaddr >= _libcStart) && (pcaddr <= _libcEnd));
  }
  
	bool isPthreadLibrary(void* pcaddr) {
    return ((pcaddr >= _libthreadStart) && (pcaddr <= _libthreadEnd));
  }

  /// Check whether an address is inside the main application.
  bool isApplication(void* pcaddr) {
    return ((pcaddr >= _appTextStart) && (pcaddr <= _appTextEnd));
  }

  mapping* getMappingByAddress(void* ptr){
    auto found = _mappings.find(interval(ptr, ptr));
    if(found != _mappings.end()) {
      return &found->second;
    }
    return NULL;
  } 

  bool isGlobalAddress(void* ptr){
    auto found = _globalmappings.find(interval(ptr, ptr));
    if(found != _mappings.end()) {
      return true;
    }
    return false;
  }

  // Print out the code information about an eip address.
  // Also try to print out the stack trace of given pcaddr.
  void printCallStack();
  void printCallStack(int depth, void** array);
  void printCallStackByAddress(char* buf, int index, void* addr);
  static int getCallStack(void** array);

  void getStackInformation(void** stackBottom, void** stackTop) {
    for(const auto& entry : _mappings) {
      const mapping& m = entry.second;
      if(m.isStack()) {
        *stackBottom = (void*)m.getBase();
        *stackTop = (void*)m.getLimit();
        return;
      }
    }
    fprintf(stderr, "Couldn't find stack mapping. Giving up.\n");
    abort();
  }

  /// Get information about global regions.
  void getTextRegions() {
    for(const auto& entry : _mappings) {
      const mapping& m = entry.second;
      if(m.isText()) {
        if(m.getFile().find("/libwatcher") != std::string::npos) {
          _doubletakeStart = (void*)m.getBase();
          _doubletakeEnd = (void*)m.getLimit();
					_currentLibrary = std::string(m.getFile());
        } else if(m.getFile().find("/libpthread-") != std::string::npos) {
          _libthreadStart = (void*)m.getBase();
          _libthreadEnd = (void*)m.getLimit();
					_threadLibrary = std::string(m.getFile());
        } else if(m.getFile().find("/libc-") != std::string::npos) {
          _libcStart = (void*)m.getBase();
          _libcEnd = (void*)m.getLimit();
        } else if(m.getFile().find("/ld-") != std::string::npos) {
          _dlStart = (void*)m.getBase();
          _dlEnd = (void*)m.getLimit();
        } else if(m.getFile() == _main_exe) {
          _appTextStart = (void*)m.getBase();
          _appTextEnd = (void*)m.getLimit();
        }

      }
    }
  }

  /// Collect all global regions.
  void getGlobalRegions(regioninfo* regions, int* regionNumb) {
    size_t index = 0;

    for(const auto& entry : _mappings) {
      const mapping& m = entry.second;

      // skip libireplayer
      if(m.isGlobals(_main_exe) && m.getFile().find("/libwatcher") == std::string::npos
          && m.isGlobals(_main_exe) && m.getFile().find("/libxed") == std::string::npos) {
        //PRINT("getGlobalRegiions: m.getBase() %lx m.getLimit() %lx isglobals and added\n", m.getBase(), m.getLimit());
        regions[index].start = (void*)m.getBase();
        regions[index].end = (void*)m.getLimit();
        index++;
        _globalmappings[interval(m.getBase(), m.getLimit())] = m;
      }
      else if(index!=0 && m.isGlobalsExt() && regions[index-1].end==(void*)m.getBase()){
        //PRINT("regions[index-1] %lx\n",  m.getLimit());
        // We will record it if the current region is also ``rw-p''. However, we can't do this for
        //the first region like this "ffff7000-300000000", it is large and we don't know what it is 
        regions[index-1].end = (void*)m.getLimit();
        _globalmappings[interval(m.getBase(), m.getLimit())] = m;
      }
    }

    // We only need to return this.
    *regionNumb = index;
  }
  
  void FindGlobalRegion(void* dynstart, intptr_t* begin, intptr_t* end) {
    bool newmapping = false;
    bool findnew = false;
    ifstream maps_file("/proc/self/maps");
    while(maps_file.good() && !maps_file.eof()) {
      mapping m;
      maps_file >> m;
      if(m.valid()) {
        /*
        if(findnew && m.isGlobalsExt() && *end==m.getBase() ) {
          *end = m.getLimit();
          _globalmappings[interval(m.getBase(), m.getLimit())] = m;
        }
        */
        if(newmapping && m.isGlobals(_main_exe)) {
          *begin = m.getBase();
          *end = m.getLimit();
          _globalmappings[interval(m.getBase(), m.getLimit())] = m;
          findnew = true;
        } else {
          findnew = false;
        }
        if(m.getBase() <= (intptr_t)dynstart 
            && m.getLimit() >= (intptr_t)dynstart) {
          newmapping = true;
        } else {
          newmapping = false;
        }
      }
    }

    //fprintf(stderr, "find new region begin %lx, end %lx, dyn region is %p\n", *begin, *end, dynstart);

  }

  bool checkAddress(intptr_t ptr){
    bool isValid = false;
    ifstream maps_file("/proc/self/maps");
    mapping m;
    while(maps_file.good() && !maps_file.eof()) {
      maps_file >> m;
      if(m.valid()) {
        if(m.getBase() <= ptr && ptr <= m.getLimit()) {
          isValid = true;
          break; 
        }
      }
    }
    return isValid;
  }

private:
  selfmap() {
    LoadSelfMaps();
  }

  void LoadSelfMaps() {
    // Read the name of the main executable
    // char buffer[PATH_MAX];
    //Real::readlink("/proc/self/exe", buffer, PATH_MAX);
    //_main_exe = std::string(buffer);
    bool gotMainExe = false;
    // Build the mappings data structure
    ifstream maps_file("/proc/self/maps");

    while(maps_file.good() && !maps_file.eof()) {
      mapping m;
      maps_file >> m;
      // It is more clean that that of using readlink. 
      // readlink will have some additional bytes after the executable file 
      // if there are parameters.	
      if(!gotMainExe && m.getFile()!="") {
        _main_exe = std::string(m.getFile());
        gotMainExe = true;
      } 

      if(m.valid()) {
        //	fprintf(stderr, "Base %lx limit %lx\n", m.getBase(), m.getLimit()); 
        _mappings[interval(m.getBase(), m.getLimit())] = m;
      }
    }
  }

  std::map<interval, mapping, std::less<interval>,
           HL::STLAllocator<std::pair<interval, mapping>, InternalHeapAllocator>> _mappings;
  std::map<interval, mapping, std::less<interval>,
           HL::STLAllocator<std::pair<interval, mapping>, InternalHeapAllocator>> _globalmappings;

  std::string _main_exe;
  std::string _currentLibrary;
  std::string _threadLibrary;
  void* _appTextStart;
  void* _appTextEnd;
	void * _libthreadStart;
	void * _libthreadEnd;
  void* _doubletakeStart;
  void* _doubletakeEnd;
  void* _libcStart;
  void* _libcEnd;
  void* _dlStart;
  void* _dlEnd;
};

#endif
