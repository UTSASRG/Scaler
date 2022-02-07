#if !defined(DOUBLETAKE_XMAPPING_H)
#define DOUBLETAKE_XMAPPING_H

/*
 * @file   xmapping.h
 * @brief  Manage all about mappings, such as changing the protection and unprotection,
 *         managing the relation between shared and private pages.
 *         Adopt from sheiff framework, but there are massive changes.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <stddef.h>
#include <string.h>
#include <unistd.h>

#ifdef SSE_SUPPORT
#include <xmmintrin.h>
#endif

#include "log.hh"
#include "mm.hh"
#include "xdefines.hh"

class xmapping {
  public:
    xmapping() : _startaddr(NULL), _startsize(0) {}

    // Initialize the map and corresponding part.
    void initialize(void* startaddr = 0, size_t size = 0, void* heapstart = NULL) {
      REQUIRE(size % xdefines::PageSize == 0, "Wrong size %lx, should be page aligned", size);
      PRINF("xmapping starts at %p, size %lx", startaddr, size);

      // Establish two maps to the backing file.
      // The persistent map is shared.
      _backupMemory = (char*)MM::mmapAllocatePrivate(size);

      // If we specified a start address (globals), copy the contents into the
      // persistent area now because the transient memory mmap call is going
      // to squash it.
      _heapStart = heapstart;

      _userMemory = (char*)startaddr;

      // The transient map is private and optionally fixed at the
      // desired start address.
      _startsize = size;
      _startaddr = (void*)_userMemory;
      _endaddr = (void*)((intptr_t)_userMemory + _startsize);

#ifdef SSE_SUPPORT
      // A string of one bits.
      allones = _mm_setzero_si128();
      allones = _mm_cmpeq_epi32(allones, allones);
#endif
    }

    // return backup memory
    void finalize() { MM::mmapDeallocate(_backupMemory, _startsize); }

    /// @return the start of the memory region being managed.
    inline char* base() const { return _userMemory; }

    /// @return the size in bytes of the underlying object.
    inline size_t size() const { return _startsize; }

    inline bool checkHeapOverflow(void* end) {
      assert(_heapStart != NULL);

      bool hasOverflow = false;

      //		PRINT("xmapping: calling checkHeapIntegrity _heapStart %p end %p\n", _heapStart, end);
      // We only need to check those allocated heap.
      hasOverflow = sentinelmap::getInstance().checkHeapIntegrity(_heapStart, end);
      //		PRINT("xmapping: calling checkHeapIntegrity hasOverflow %d\n", hasOverflow);

      return hasOverflow;
    }

    // For the page
    void backup(void* end) {
      size_t sz;

      if(_heapStart) {
        sz = (intptr_t)end - (intptr_t)base();
      } else {
        // Commit all pages
        sz = size();
      }

      char* toMem = _backupMemory;
      char* fromMem = _userMemory;
#ifdef SSE_SUPPORT
      if((((intptr_t)toMem & 15) | ((intptr_t)fromMem & 15)) == 0) {
        int remaining = sz & 31;
        //int remaining = sz & 15;

        sz -= remaining;

        CopyWithSSESmall((uint8_t *)toMem, (uint8_t *)fromMem, sz);
        //CopyWithSSENoCache((uint8_t *)toMem, (uint8_t *)fromMem, sz);
        //CopyDiffWithSSE(toMem, fromMem, sz);

        //size_t realsize = sz / sizeof(uint64_t);
        //__asm__ __volatile__("rep movsq" : "+D"(toMem), "+S"(fromMem), "+c"(realsize) : : "memory");

        toMem += sz;
        fromMem += sz;
        sz = remaining;
      }
#endif
      // Copy everything to _backupMemory From _userMemory
      if(sz > 0) {
        memcpy(toMem, fromMem, sz);
      }
      //    PRINT("Backup memory: from %p to %p size %lx\n", _userMemory, _backupMemory, sz);
    }

    // How to commit some memory
    void commit(void* start, size_t size) {
      size_t offset = (intptr_t)start - (intptr_t)base();

      void* dest = (void*)((intptr_t)_backupMemory + offset);
      memcpy(dest, start, size);
    }

    // Release all temporary pages.
    void recoverMemory(void* end) {
      size_t sz;

      if(_heapStart) {
        sz = (intptr_t)end - (intptr_t)base();
      } else {
        // Commit all pages
        sz = size();
      }

      //   PRINT("Recover memory from %p to %p size %lx\n", _backupMemory, _userMemory, sz);
      memcpy(_userMemory, _backupMemory, sz);
    }

  private:

#ifdef SSE_SUPPORT
    // dst and src must be 16-byte aligned
    // size must be multiple of 16*2 = 32 bytes
    void CopyWithSSENoCache(uint8_t* dst, uint8_t* src, size_t size) {
      size_t stride = 2 * sizeof(__m128);
      while (size) {
        __m128 a = _mm_load_ps((float*)(src + 0*sizeof(__m128)));
        __m128 b = _mm_load_ps((float*)(src + 1*sizeof(__m128)));
        _mm_stream_ps((float*)(dst + 0*sizeof(__m128)), a);
        _mm_stream_ps((float*)(dst + 1*sizeof(__m128)), b);

        size -= stride;
        src += stride;
        dst += stride;
      }
    }

    // dst and src must be 16-byte aligned
    // size must be multiple of 16*2 = 32 bytes
    void CopyWithSSESmall(uint8_t* dst, uint8_t* src, size_t size) {
      size_t stride = 2 * sizeof(__m128);
      while (size) {
        __m128 a = _mm_load_ps((float*)(src + 0*sizeof(__m128)));
        __m128 b = _mm_load_ps((float*)(src + 1*sizeof(__m128)));
        _mm_store_ps((float*)(dst + 0*sizeof(__m128)), a);
        _mm_store_ps((float*)(dst + 1*sizeof(__m128)), b);

        size -= stride;
        src += stride;
        dst += stride;
      }
    }

    // size must be multiple of 16*1 = 16 bytes
    void CopyDiffWithSSE(void* dst, void* src, size_t size) {
      __m128i * srcbuf = (__m128i *) src;
      __m128i * dstbuf  = (__m128i *) dst;

      int number = size / sizeof(__m128i);
      for (int i = 0; i < number; i++) {

        __m128i srcChunk, dstChunk;

        srcChunk = _mm_load_si128 (&srcbuf[i]);
        dstChunk  = _mm_load_si128 (&dstbuf[i]);

        __m128i eqChunk = _mm_cmpeq_epi8 (srcChunk, dstChunk);
        __m128i neqChunk = _mm_xor_si128 (allones, eqChunk);

        _mm_maskmoveu_si128 (srcChunk, neqChunk, (char *) &dstbuf[i]);
      }
    }
#endif

    /// The starting address of the region.
    void* _startaddr;

    void* _heapStart;

    /// The size of the region.
    size_t _startsize;

    /// The starting address of the region.
    void* _endaddr;

    /// The transient (not yet backed) memory.
    char* _userMemory;

    /// The persistent (backed to disk) memory.
    char* _backupMemory;

#ifdef SSE_SUPPORT
    __m128i allones;
#endif
};

#endif
