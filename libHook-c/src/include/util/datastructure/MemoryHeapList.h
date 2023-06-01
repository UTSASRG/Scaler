#ifndef SCALER_MEMORYHEAPLIST_H
#define SCALER_MEMORYHEAPLIST_H


#include <cstdio>
#include <sys/mman.h>
#include <util/datastructure/MemoryHeap.h>
#include <exceptions/ScalerException.h>
#include <cstring>
#include <cassert>

namespace scaler {
    template<class T>
    class MemoryHeapList : public MemoryHeap<T> {

        /**
         * Actual storage format
         */
        union InternalObjType {
            InternalObjType *addr;
            T obj;
        };

        /**
         * A pre-allocated memory segment that stores a group of objects
         */
        class Chunk {

        public:
            //The starting address of memory segment
            InternalObjType *internalArr;
            //Point to the next chunk. (Only used to free all chunks)
            Chunk *nextChunk;
            //The number of objects in this chunk
            ssize_t internalArrSize;
            //Slots tracked by free list
            ssize_t size;
            size_t objSize;

            Chunk(ssize_t objNum) : nextChunk(nullptr), internalArrSize(objNum), size(0) {
                internalArr = (InternalObjType *) mmap(NULL, internalArrSize * sizeof(InternalObjType), PROT_READ | PROT_WRITE,
                                                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if (internalArr == MAP_FAILED) {
                    fatalErrorS("Failed to allocate memory for MemoryHeap::Chunk at %p because: %s", internalArr,
                                strerror(errno));
                    exit(-1);
                }
            }

            

            inline bool isFull() {
                return size == internalArrSize;
            }

            ~Chunk() {
                if (munmap(internalArr, internalArrSize * sizeof(InternalObjType)) == -1) {
                    fatalErrorS("Failed to deallocate memory for MemoryHeap::Chunk at %p because: %s", internalArr,
                                strerror(errno));
                    exit(-1);
                }
                internalArr = nullptr;
                nextChunk = nullptr;
                internalArrSize = 0;
            }
        };

    public:
        MemoryHeapList(size_t initialChunkSize = 32) : freelists(nullptr), largestChunkSize(initialChunkSize) {
            chunkHead = new Chunk(largestChunkSize);
        }

        ~MemoryHeapList() {
            //Release all chunks
            while (chunkHead) {
                Chunk *cur = chunkHead;
                chunkHead = chunkHead->nextChunk;
                delete cur;
            }
            chunkHead = nullptr;
        }


        void free(T *&obj) {
            //Add this object back to freelist
            ((InternalObjType *) (obj))->addr = freelists;
            freelists = (InternalObjType *) obj;
#ifndef NDEBUG
            obj = nullptr;
#endif
        }

        T *malloc() {
            if (!freelists) {
                //If there are freed slots, use unallocated entry
                if (chunkHead->isFull()) {
                    //No free entries in the current slot either.
                    largestChunkSize *= 2;
                    //Allocate a chunk that is larger
                    expand(largestChunkSize);
                }
                //Use the next free slot in this chunk
                InternalObjType *slotAddr = chunkHead->internalArr + chunkHead->size;
                chunkHead->size += 1;
                return (T*) (slotAddr);
            }else{
                //If there are freed slots, use free slots
                InternalObjType *obj = freelists;
                freelists = freelists->addr; //Return the first slot from the freeList. Move freelist to the next slot.
                return (T *) obj;
            }
           
        }

    protected:
        inline void expand(ssize_t tgtSize) {
            //Need a larger chunk
            //The free list is used up, allocate more chunks
            auto *newChunk = new Chunk(tgtSize);
            newChunk->nextChunk = chunkHead;
            chunkHead = newChunk;
        }

        size_t largestChunkSize;
        Chunk *chunkHead;
        InternalObjType *freelists;
    };

}

#endif
