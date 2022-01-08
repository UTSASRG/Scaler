#if !defined(DOUBLETAKE_CALLSITE_H)
#define DOUBLETAKE_CALLSITE_H

/*
 * @file   callsite.h
 * @brief  Management about callsite for heap objects.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <stdio.h>

#include "xdefines.hh"

class CallSite {
public:
  CallSite() : _depth(0) {
    for (auto i = 0; i < xdefines::CALLSITE_MAXIMUM_LENGTH; i++) {
      _callsite[i] = nullptr;
    }
  }

  unsigned long depth() { return _depth; }

  void print() {
    for(auto i = 0; i < (int) _depth; i++) {
      printf("%p\t", _callsite[i]);
    }
    printf("\n");
  }

  unsigned long get(int index) { return (unsigned long)_callsite[index]; }

  // Save callsite
  void save(int depth, void** addr) {
    _depth = depth;

    for(int i = 0; i < depth; i++) {
      _callsite[i] = addr[i];
    }
  }

  bool saveAndCheck(int depth, void** addr) {
    bool isSame = true;

    _depth = depth;

    for(int i = 0; i < depth; i++) {
      if(_callsite[i] != addr[i]) {
        isSame = false;
        _callsite[i] = addr[i];
      }
    }

    return isSame;
  }

  // Return the callsite
  void** getCallsite() { return &_callsite[0]; }

private:
  unsigned long _depth; // Actual callsite depth
  void* _callsite[xdefines::CALLSITE_MAXIMUM_LENGTH];
};

#endif
