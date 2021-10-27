#if !defined(DOUBLETAKE_HASHMAP_H)
#define DOUBLETAKE_HASHMAP_H

/*
 * @file   Hahsmap.h
 * @brief  Management about hash table. The "get" function of this class shouldn't call any external library.
 * @author
 *         Steven Tang <jtang@umass.edu>
 *         The original design is from:
 *         Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *         kulesh [squiggly] isis.poly.edu
 */


#include <unistd.h>
#include <util/tool/Logging.h>
#include <pthread.h>
#include <util/datastructure/List.h>
#include <new>
#include <cassert>
#include <cstdlib>

namespace scaler {

    /**
     * Default comparator for hashmap by using operator==
     * @tparam TpKey The type of hashmap getKey
     * @return 0 if equals, 1 if src>dst, -1 if src<dst
     */
    template<class TpKey>
    char cmpWeq(const TpKey &src, const TpKey &dst) {
        if (src == dst) {
            return 0;
        } else if (src > dst) {
            return 1;
        } else {
            return -1;
        }
    }

    /**
     * Default hash function that passes getKey without processing.
     * This only works for hashmap that use numerical value as getKey.
     */
    template<class TpKey>
    ssize_t hfuncPassThrough(const TpKey &key) {
        return (ssize_t) (key);
    }

    template<class TpKey, class TpVal>
    class HashEntry {
    public:

        HashEntry() = default;

        HashEntry(const TpKey &key, const TpVal &val) : key(key), val(val) {

        }

        bool operator==(const HashEntry &rho) {
            return key == rho.key && val == rho.val;
        }

        inline const TpKey &getKey() {
            return key;
        }

        inline TpVal &getVal() {
            return val;
        }

        inline TpVal setVal(const TpVal &newVal) {
            return val = newVal;
        }


    protected:
        TpKey key;
        TpVal val;
    };

    template<class TpKey, class TpVal>
    class HashBucket : public List<HashEntry<TpKey, TpVal>> {
    public:
        using HashEntry_ = HashEntry<TpKey, TpVal>;

        List<HashEntry_> entryList; //There are always a head node
        // Each entry has a separate lock
        const ssize_t index;

        explicit HashBucket() : index(0) {
        }

        List<HashEntry<TpKey, TpVal>> &operator=(const List<HashEntry<TpKey, TpVal>> &rho) {
            if (this != &rho) {
                //Different object
                List<HashEntry<TpKey, TpVal>>::operator=(rho);
                if (dynamic_cast<HashBucket<TpKey, TpVal> *>(&rho)) {
                    //Sub-class
                    index = rho.index;
                }
            }
            return *this;
        }

        HashBucket &operator=(HashBucket &rho) {
            List<HashEntry<TpKey, TpVal>>::operator=(rho);
        }


    };

    template<class TpKey, class TpVal>
    class HashMap;

    template<class TpKey, class TpVal>
    class HashMapIterator : public ForwardIterator<TpVal> {
    public:
        using HashMap_ = HashMap<TpKey, TpVal>;
        using HashBucket_ = HashBucket<TpKey, TpVal>;
        using HashEntry_ = HashEntry<TpKey, TpVal>;
        using BucketEntry_ = ListEntry<HashEntry_>;


        HashMapIterator<TpKey, TpVal> &operator++() override {
            bool hasNext = getNextEntry(curBucket, curBucketEntry);
            assert(hasNext);
            return *this;
        }


        HashMapIterator<TpKey, TpVal> &operator++(int) override {
            return operator++();
        }

        bool operator==(const Iterator &rho) const override {
            const HashMapIterator<TpKey, TpVal> *rhoPtr = dynamic_cast<const HashMapIterator<TpKey, TpVal> *>(&rho);
            if (rhoPtr) {
                return curBucketEntry == rhoPtr->curBucketEntry && curBucket == rhoPtr->curBucket &&
                       hashMap == rhoPtr->hashMap;
            } else {
                return false;
            }
        }

        inline bool operator!=(const Iterator &rho) const override {
            return !operator==(rho);
        }


        TpVal &operator*() override {
            return curBucketEntry->getVal().getVal();
        }

        const TpKey &getKey() {
            return curBucketEntry->getVal().getKey();
        }

        TpVal &getVal() {
            return operator*();
        }


        explicit HashMapIterator(HashMap_ *hashMap) : hashMap(hashMap), curBucket(&hashMap->buckets[0]),
                                                      curBucketEntry(curBucket->entryList.getHead()) {

        }

    protected:
        friend class HashMap<TpKey, TpVal>;

        // which bucket at the moment? [0, nbucket-1]
        HashMap_ *hashMap;
        HashBucket_ *curBucket;
        // which bucket at the moment? [0, nbucket-1]
        BucketEntry_ *curBucketEntry;

        inline bool getNextEntry(HashBucket_ *&bucket, BucketEntry_ *&entry) {
            bool hasNext = false;
            entry = curBucketEntry;
            bucket = curBucket;
            while (true) {
                if (entry->getNext() != curBucket->entryList.getTail()) {
                    entry = entry->getNext();
                    hasNext = true;
                } else if (bucket->index + 1 < hashMap->bucketNum) {
                    //Reached the end of the list, try next bucket
                    bucket = &hashMap->buckets[bucket->index + 1];
                    entry = bucket->entryList.getHead();
                } else {
                    break;
                }
            }

            return hasNext;
        }

        HashMap<TpKey, TpVal> *hashmap;
    };

    template<class TpKey, class TpVal>
    class HashMap : public Iteratable<HashMapIterator<TpKey, TpVal>> {
    public:
        using HashBucket_ = HashBucket<TpKey, TpVal>;
        using HashEntry_ = HashEntry<TpKey, TpVal>;
        using HashMapIterator_ = HashMapIterator<TpKey, TpVal>;
        using BucketEntry_ = ListEntry<HashEntry_>;

        /**
         * Comparator.
         * 0: src==dst
         * -1: src<dst
         * 1: src>dst
         */
        typedef char (*Comparator)(const TpKey &src, const TpKey &dst);

        typedef ssize_t (*HashFunc)(const TpKey &key);


        HashMap &operator=(const HashMap &rho) {
            hfunc = rho.hfunc;
            kcmp = rho.kcmp;
            bucketNum = rho.bucketNum;
            //The extra two buckets are used for forward and backward end() iterator
            buckets = new HashBucket_[bucketNum];
            *buckets = *rho.buckets;
            initialized = rho.initialized;
            return *this;
        }
        //todo: Implement right value copy

        HashMap(const HashFunc &hfunc = hfuncPassThrough<TpKey>, const Comparator &kcmp = cmpWeq<TpKey>,
                const ssize_t bucketNum = 4096) : hfunc(hfunc),
                                                  kcmp(kcmp),
                                                  initialized(false),
                                                  buckets(nullptr),
                                                  bucketNum(bucketNum),
                                                  beginIter(this),
                                                  endIter(this) {
            assert(hfunc != nullptr && kcmp != nullptr);
            // Initialize all of these _entries.
            buckets = new HashBucket_[bucketNum];
            initialized = true;
            beginIter.curBucket = &buckets[0];
            beginIter.curBucketEntry = beginIter.curBucket->getHead()->getNext();

            endIter.curBucket = &buckets[bucketNum - 1];
            endIter.curBucketEntry = endIter.curBucket->getTail();

        }

        HashMap(const HashMap &rho) {
            operator=(rho);
        }

        inline ssize_t hashIndex(const TpKey &key) {
            ssize_t hkey = hfunc(key);
            return hkey % bucketNum;
        }

        // Look up whether an entry is existing or not.
        // If existing, return true. *value should be carried specific value for this key.
        // Otherwise, return false.
        bool get(const TpKey &key, TpVal *&value) {
            assert(initialized);
            ssize_t hindex = hashIndex(key);

            HashEntry_ *hashEntry;
            HashBucket_ *bucket = getHashBucket(hindex);
            bool found = getHashEntry(key, bucket, hashEntry);
            if (found) {
                value = &hashEntry->getVal();
            }
            return found;
        }

        /**
         * Insert getKey:value pair into hashmap
         * If getKey exists, insert will overwrite the value
         *
         * @param key
         * @param val
         * @return
         */
        bool put(const TpKey &key, const TpVal &val) {
            assert(initialized);
            ssize_t hindex = hashIndex(key);

            HashBucket_ *bucket = getHashBucket(hindex);
            HashEntry_ *hashEntry;

            if (getHashEntry(key, bucket, hashEntry)) {
                hashEntry->setVal(val);
            } else {
                HashEntry_ newEntry(key, val);

                bucket->entryList.insertAfter(bucket->entryList.getHead(), newEntry);
            }
            return true;
        }

        // Free an entry with specified
        bool erase(const TpKey &key) {
            assert(initialized);
            ssize_t hindex = hashIndex(key);

            auto *curBucket = getHashBucket(hindex);

            BucketEntry_ *entry;
            bool isFound = getBucketEntry(key, curBucket, entry);

            if (isFound) {
                curBucket->entryList.erase(entry);
            }

            return isFound;
        }


        /**
         * Acquire the first entry of the hash table.
         * If there is no entry, then begin()==end()==head of the first bucket
         * @return
         */
        const HashMapIterator<TpKey, TpVal>& begin() {
            return beginIter;
        }

        const HashMapIterator<TpKey, TpVal>& end() {
            return endIter;
        }

        ~HashMap() {

        }

        bool initialized = false;
        HashBucket_ *buckets = nullptr; //There are bucketNum+1 nodes. The last node indicates the end of bucket.
        ssize_t bucketNum = -1;     // How many buckets in total
        HashFunc hfunc = nullptr;
        Comparator kcmp = nullptr;

    protected:
        inline HashBucket_ *getHashBucket(const ssize_t &hindex) {
            assert(hindex < bucketNum);
            return &buckets[hindex];
        }

        /**
         * Return the first found entry or the last element of entry list if not found
         * @return :Found entry or not
         */
        inline bool
        getHashEntry(const TpKey &key, HashBucket_ *bucket, HashEntry_ *&entry) {
            BucketEntry_ *listEntry;
            bool found = getBucketEntry(key, bucket, listEntry);
            if (found)
                entry = &listEntry->getVal();
            return found;
        }


        inline bool
        getBucketEntry(const TpKey &key, HashBucket_ *bucket, BucketEntry_ *&entry) {
            assert(bucket != nullptr);
            bool found = false;
            if (bucket != nullptr) {
                //Start with the first node
                auto *listEntry = bucket->entryList.getHead()->getNext();

                while (listEntry != bucket->entryList.getTail()) {
                    if (kcmp(listEntry->getVal().getKey(), key) == 0) {
                        found = true;
                        break;
                    }
                    listEntry = listEntry->getNext();
                }

                if (found)
                    entry = listEntry;

            }
            return found;
        }

        HashMapIterator_ beginIter;
        HashMapIterator_ endIter;


    };


}
#endif
