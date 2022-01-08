/* -*- C++ -*- */

#ifndef HL_ANSIWRAPPER_H
#define HL_ANSIWRAPPER_H

#include <assert.h>
#include <string.h>

#include "utility/gcd.h"
#include "utility/istrue.h"
#include "utility/sassert.h"
#include "mallocinfo.h"


/*
 * @class ANSIWrapper
 * @brief Provide ANSI C behavior for malloc & free.
 *
 * Implements all prescribed ANSI behavior, including zero-sized
 * requests & aligned request sizes to a double word (or long word).
 */

namespace HL {

  template <class SuperHeap>
  class ANSIWrapper : public SuperHeap {
  private:
    // The unique value we return for malloc(0).
    // See http://linux.die.net/man/3/malloc
    // "If size is 0, then malloc() returns either NULL,
    // or a unique pointer value that can later be successfully
    // passed to free()."
    // Note that is deliberately un-aligned and so will never collide
    // with a real pointer.
    enum { UNIQUE_VALUE = 0x81234567 };

  public:
  
    ANSIWrapper() {
      sassert<(gcd<SuperHeap::Alignment, HL::MallocInfo::Alignment>::value == HL::MallocInfo::Alignment)> checkAlignment;
      checkAlignment = checkAlignment;
    }

    inline void * malloc (size_t sz) {
      if (sz == 0) {
	return (void *) UNIQUE_VALUE;
      }
      // Prevent integer underflows. This maximum should (and
      // currently does) provide more than enough slack to compensate for any
      // rounding below (in the alignment section).
      if (sz > HL::MallocInfo::MaxSize) {
	return NULL;
      }
      if (sz < HL::MallocInfo::MinSize) {
      	sz = HL::MallocInfo::MinSize;
      }
      // Enforce alignment requirements: round up allocation sizes if needed.
      // NOTE: Alignment needs to be a power of two.
      sassert<(HL::MallocInfo::Alignment & (HL::MallocInfo::Alignment - 1)) == 0> powTwo;
      powTwo = powTwo;

      // Enforce alignment.
      sz = (sz + HL::MallocInfo::Alignment - 1) & ~(HL::MallocInfo::Alignment - 1);

      void * ptr = SuperHeap::malloc (sz);
      assert ((size_t) ptr % HL::MallocInfo::Alignment == 0);
      return ptr;
    }
 
    inline void free (void * ptr) {
      if (ptr != (void *) UNIQUE_VALUE) {
	if (ptr != 0) {
	  SuperHeap::free (ptr);
	}
      }
    }

    inline void * calloc (const size_t s1, const size_t s2) {
      char * ptr = (char *) malloc (s1 * s2);
      if (ptr) {
      	memset (ptr, 0, s1 * s2);
      }
      return (void *) ptr;
    }
  
    inline void * realloc (void * ptr, const size_t sz) {
      if (ptr == 0) {
      	return malloc (sz);
      }
      if (sz == 0) {
      	free (ptr);
      	return 0;
      }

      size_t objSize = getSize (ptr);
      if (objSize == sz) {
    	return ptr;
      }

      // Allocate a new block of size sz.
      void * buf = malloc (sz);

      // Copy the contents of the original object
      // up to the size of the new block.

      size_t minSize = (objSize < sz) ? objSize : sz;
      if (buf) {
	memcpy (buf, ptr, minSize);
      }

      // Free the old block.
      free (ptr);
      return buf;
    }
  
    inline size_t getSize (void * ptr) {
      if (ptr) {
	return SuperHeap::getSize (ptr);
      } else {
	return 0;
      }
    }
  };

}

#endif
