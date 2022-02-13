#ifndef SCALER_MEMORYHEAP_H
#define SCALER_MEMORYHEAP_H


#include <cstdio>
#include <sys/mman.h>
#include <exceptions/ScalerException.h>
#include <cstring>
#include <cassert>

namespace scaler {
    template<class OBJTYPE>
    class MemoryHeap {

        /**
         * Actual storage format
         */
        union InternalObjType {
            InternalObjType *addr;
            OBJTYPE obj;
        };

        /**
         * A pre-allocated memory segment that stores a group of objects
         */
        class Chunk {


        public:
            //The starting address of memory segment
            InternalObjType *startAddr;
            //Point to the next chunk. (Only used to free all chunks)
            Chunk *nextChunk;
            //The number of objects in this chunk
            ssize_t totalSlots;
            //Slots tracked by free list
            ssize_t managedSlots;
            size_t objSize;

            Chunk(ssize_t objNum) : nextChunk(nullptr), totalSlots(objNum), managedSlots(0) {
                startAddr = (InternalObjType *) mmap(NULL, totalSlots * sizeof(InternalObjType), PROT_READ | PROT_WRITE,
                                                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if (startAddr == MAP_FAILED) {
                    fatalErrorS("Failed to allocate memory for MemoryHeap::Chunk at %p because: %s", startAddr,
                                strerror(errno));
                    exit(-1);
                }
            }

            inline InternalObjType *getNextUnmanagedSlot() {
                if (managedSlots == totalSlots) {
                    return nullptr;
                }
                InternalObjType *slotAddr = startAddr + managedSlots;
                managedSlots += 1;
                slotAddr->addr = 0;
                return slotAddr;
            }

            inline bool isFull() {
                return managedSlots == totalSlots;
            }

            ~Chunk() {
                if (munmap(startAddr, totalSlots * sizeof(InternalObjType)) == -1) {
                    fatalErrorS("Failed to deallocate memory for MemoryHeap::Chunk at %p because: %s", startAddr,
                                strerror(errno));
                    exit(-1);
                }
                startAddr = nullptr;
                nextChunk = nullptr;
                totalSlots = 0;
            }
        };

    public:
        MemoryHeap(size_t initialChunkSize = 32) : freelists(nullptr), largestChunkSize(initialChunkSize) {
            chunkHead = new Chunk(largestChunkSize);
        }

        ~MemoryHeap() {
            //Release all chunks
            while (chunkHead) {
                Chunk *cur = chunkHead;
                chunkHead = chunkHead->nextChunk;
                delete cur;
            }
            chunkHead = nullptr;
        }


        void free(OBJTYPE *&obj) {
            //Point free list to the chunkHead of this object
            ((InternalObjType *) (obj))->addr = freelists;
            freelists = (InternalObjType *) obj;
#ifndef NDEBUG
            obj = nullptr;
#endif
        }

        OBJTYPE *malloc() {
            if (!freelists) {
                if (chunkHead->isFull()) {
                    //Need a larger chunk
                    //The free list is used up, allocate more chunks
                    largestChunkSize *= 2;
                    auto *newChunk = new Chunk(largestChunkSize);
                    newChunk->nextChunk = chunkHead;
                    chunkHead = newChunk;
                }
                //Use the next free slot in this chunk
                freelists = chunkHead->getNextUnmanagedSlot();
            }

            InternalObjType *obj = freelists;
            freelists = freelists->addr;

            return (OBJTYPE *) obj;
        }

    protected:
        size_t largestChunkSize;
        Chunk *chunkHead;
        InternalObjType *freelists;
    };

}

#endif
