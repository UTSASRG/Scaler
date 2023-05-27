#ifndef SCALER_ARRAY_H
#define SCALER_ARRAY_H

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <util/datastructure/ForwardIterator.h>
#include <sys/mman.h>
#include <exceptions/ScalerException.h>
#include <util/tool/Math.h>

namespace scaler {

    /**
  * Auto expansion array. Won't call
  * Won't call any external function for read-only operation
  * @tparam T Value type
  */
    /**
  * Auto expansion array. Won't call
  * Won't call any external function for read-only operation
  * @tparam T Value type
  */
    template<typename T,typename... Args>
    class Array {
    public:

        //todo: Check initialized
        explicit Array() : internalArr(nullptr),internalArrSize(0), size(0) {
            //INFO_LOGS("Check initialized %zd\n",getSize());

        }

        explicit Array(const ssize_t &initialSize,Args... restOfArgs) : internalArr(nullptr),internalArrSize(initialSize), size(0) {
            //INFO_LOGS("Check initialized1 %zd\n",getSize());

            if(initialSize>0){
                internalArr = (T *) malloc(internalArrSize * sizeof(T));
//                INFO_LOGS("Internal array %d bytes",internalArrSize * sizeof(T));
                assert(internalArr != nullptr);
                memset(internalArr, 0, internalArrSize * sizeof(T));
            }
        }

        Array(const Array &rho) : Array() {
            operator=(rho);
        }

        virtual ~Array() {
            if (internalArr)
                free(internalArr);
        }

        Array &operator=(const Array &rho) {
            if (&rho != this) {
                if (internalArr)
                    free(internalArr);

                internalArr = (T *) malloc(rho.internalArrSize * sizeof(T));
                size = rho.size;
                internalArrSize = rho.internalArrSize;
                memcpy(internalArr, rho.internalArr, rho.internalArrSize * sizeof(T));
            }
            return *this;
        };

        virtual bool isEmpty() const {
            return size == 0;
        }

        inline T &operator[](const ssize_t &index) {
            if(!(0 <= index && index < size)){
                INFO_LOGS("%zd,%zd",index,size);
            }
            assert(0 <= index && index < size);
            assert(internalArr != nullptr);
            return internalArr[index];
        }

        inline T &get(const ssize_t &index) {
            assert(0 <= index && index < size);
            assert(internalArr != nullptr);
            return internalArr[index];
        }

        virtual inline void erase(const ssize_t &index) {
            assert(0 <= index && index < size);
            size -= 1;
            memmove(internalArr + index, internalArr + index + 1, (size - index) * sizeof(T));
        }


        virtual T *insertAt(ssize_t index,Args... constructorArgs) {
            assert(0 <= index && index <= size);
            if (size+1 > internalArrSize) {
                expand((size+1) * 2);
            }
            if (index < size) {
                memmove(internalArr + index + 1, internalArr + index, (size - index) * sizeof(T));
            }
            size += 1;
            new(internalArr + index) T(constructorArgs...);  //Construct object
            return internalArr + index;
        }

        /**
         * Allocate a bunch of memory. If the memory is already available, only expand size.
         * This can be used to guarantee enough memory
         * The constructor will not be called
         */
        virtual T* allocate(ssize_t amount,Args... constructorArgs) {
            ssize_t requiredSize = size + amount;
            if (requiredSize > internalArrSize)
                expand(requiredSize * 2);

            for(int i=size;i<requiredSize;++i){
                new(internalArr + i) T(constructorArgs...);
            }

            T* rlt=internalArr + size;
            size += amount;
            return rlt;
        }

        virtual inline T *pushBack(Args... constructorArgs) {
            return insertAt(size,constructorArgs...);
        }

        virtual inline void popBack() {
            size -= 1;
        }


        virtual inline ssize_t getSize() {
            return size;
        }

        virtual inline ssize_t getTypeSizeInBytes() {
            return sizeof(T);
        }

        virtual inline bool willExpand() {
            return size == internalArrSize;
        }

        virtual inline void clear() {
            size = 0;
        }

        T *data() const {
            return internalArr;
        }

        T *internalArr = nullptr;

    protected:
        ssize_t internalArrSize = 0;
        ssize_t size = 0;

        virtual bool expand(ssize_t newSize) {
            T *oldInternalArr = internalArr;

            internalArr = (T *) malloc(newSize * sizeof(T));
            if(!internalArr){
                fprintf(stderr,"Cannot allocate memory");
                exit(-1);
                return false;
            }

            if(oldInternalArr){
                memset(internalArr, 0, newSize * sizeof(T));
                memcpy(internalArr, oldInternalArr, internalArrSize * sizeof(T));
            }
            free(oldInternalArr);
            internalArrSize = newSize;
            return true;
        }
    };

    template<typename T, typename... Args>
    class FixedArray : public Array<T,Args...> {
    public:

        //todo: Check initialized
        explicit FixedArray(const ssize_t &initialSize,Args...) : Array<T,Args...>(initialSize) {
        }

    protected:
        bool expand(ssize_t newSize) override {
            fatalErrorS("FixedArray of size %zd cannot expand to %zd. Assign a larger initial size.",this->size,newSize);
            return false;
        }
    };


}
#endif
