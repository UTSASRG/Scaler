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

        // Each entry has a separate lock
        ssize_t index;

        explicit HashBucket(const ssize_t &index = 0) : index(index) {
        }

        HashBucket(const HashBucket<TpKey, TpVal> &rho) {
            operator=(rho);
        };

        HashBucket &operator=(const HashBucket<TpKey, TpVal> &rho) {
            List<HashEntry<TpKey, TpVal>>::operator=(rho);
            index = rho.index;
            return *this;
        }

    };

    template<class TpKey, class TpVal>
    class HashMap;

    template<class TpKey, class TpVal>
    class HashMapIterator : public ForwardIterator<TpVal, HashMapIterator<TpKey, TpVal>> {
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

        const HashMapIterator operator++(int) override {
            return operator++();
        }

        bool operator==(const HashMapIterator &rho) const override {
            return curBucketEntry == rho.curBucketEntry && curBucket == rho.curBucket &&
                   hashMap == rho.hashMap;
        }

        inline bool operator!=(const HashMapIterator &rho) const override {
            return !operator==(rho);
        }


        TpVal &operator*() override {
            assert(curBucketEntry != curBucket->getTail());
            return curBucketEntry->getVal().getVal();
        }

        const TpKey &getKey() {
            return curBucketEntry->getVal().getKey();
        }

        TpVal &getVal() {
            return operator*();
        }


        explicit HashMapIterator(HashMap_ *hashMap = nullptr) : hashMap(hashMap), curBucket(nullptr),
                                                                curBucketEntry(nullptr) {

        }

    protected:
        friend class HashMap<TpKey, TpVal>;

        // which bucket at the moment? [0, nbucket-1]
        HashMap_ *hashMap;
        HashBucket_ *curBucket;
        // which bucket at the moment? [0, nbucket-1]
        BucketEntry_ *curBucketEntry;

        inline bool getNextEntry(HashBucket_ *&bucket, BucketEntry_ *&entry) {
            bool success = false;
            entry = curBucketEntry;
            bucket = curBucket;
            while (true) {
                if (entry->getNext() != bucket->getTail()) {
                    //Not tail, go to next element.
                    entry = entry->getNext();
                    success = true;
                    break;
                } else if (bucket->index == hashMap->bucketNum - 1) {
                    //Last bucket and tail node, this is valid for end iterator
                    entry = entry->getNext();
                    success = true;
                    break;
                } else {
                    //Not last bucket but is tail node
                    bucket = &hashMap->buckets[bucket->index + 1];
                    entry = bucket->getHead();
                }
            }
            return success;
        }

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
            if (&rho != this) {
                hfunc = rho.hfunc;
                kcmp = rho.kcmp;
                bucketNum = rho.bucketNum;
                //The extra two buckets are used for forward and backward end() iterator
                buckets = new HashBucket_[bucketNum];
                for (int i = 0; i < bucketNum; ++i) {
                    buckets[i] = rho.buckets[i];
                }
            }
            return *this;
        }
        //todo: Implement right value copy

        HashMap(const ssize_t bucketNum = 16, const HashFunc &hfunc = hfuncPassThrough<TpKey>,
                const Comparator &kcmp = cmpWeq<TpKey>) : hfunc(hfunc),
                                                          kcmp(kcmp),
                                                          buckets(nullptr),
                                                          bucketNum(bucketNum) {
            assert(bucketNum >= 1);
            assert(hfunc != nullptr && kcmp != nullptr);
            // Initialize all of these _entries.
            buckets = new HashBucket_[bucketNum];
            for (int i = 0; i < bucketNum; ++i) {
                buckets[i].index = i;
            }
            beginIter.hashMap = this;
            endIter.hashMap = this;
        }

        HashMap(const HashMap &rho) {
            operator=(rho);
            beginIter.hashMap = this;
            endIter.hashMap = this;
            endIter.curBucket = &buckets[bucketNum - 1];
            endIter.curBucketEntry = endIter.curBucket->getTail();
        }

        inline ssize_t hashIndex(const TpKey &key) {
            ssize_t hkey = hfunc(key);
            return hkey % bucketNum;
        }

        // Look up whether an entry is existing or not.
        // If existing, return true. *value should be carried specific value for this key.
        // Otherwise, return false.
        TpVal &get(const TpKey &key) {
            TpVal *rltPtr = getPtr(key);
            assert(rltPtr != nullptr);
            return *rltPtr;
        }


        TpVal *getPtr(const TpKey &key) {
            ssize_t hindex = hashIndex(key);

            HashEntry_ *hashEntry;
            HashBucket_ *bucket = getHashBucket(hindex);
            bool found = getHashEntry(key, bucket, hashEntry);
            return found ? &hashEntry->getVal() : nullptr;
        }

        bool has(const TpKey &key) {
            ssize_t hindex = hashIndex(key);
            HashEntry_ *hashEntry;
            HashBucket_ *bucket = getHashBucket(hindex);
            return getHashEntry(key, bucket, hashEntry);
        }

        /**
         * Insert getKey:value pair into hashmap
         * If getKey exists, insert will overwrite the value
         *
         * @param key
         * @param val
         * @return
         */
        void put(const TpKey &key, const TpVal &val, bool replace = true) {
            ssize_t hindex = hashIndex(key);
            HashBucket_ *bucket = getHashBucket(hindex);
            HashEntry_ *hashEntry;

            if (getHashEntry(key, bucket, hashEntry)) {
                assert(replace);
                hashEntry->setVal(val);
            } else {
                HashEntry_ newEntry(key, val);
                bucket->insertAfter(bucket->getHead(), newEntry);
            }
        }

        // Free an entry with specified
        void erase(const TpKey &key, bool mustExist = false) {
            ssize_t hindex = hashIndex(key);

            auto *curBucket = getHashBucket(hindex);

            BucketEntry_ *entry;
            bool isFound = getBucketEntry(key, curBucket, entry);

            if (mustExist)
                assert(isFound);
            if (isFound)
                curBucket->erase(entry);
        }


        /**
         * Acquire the first entry of the hash table.
         * If there is no entry, then begin()==end()==head of the first bucket
         * @return
         */
        const HashMapIterator<TpKey, TpVal> &begin() {
            beginIter.curBucket = &buckets[0];
            beginIter.curBucketEntry = beginIter.curBucket->getHead();
            return ++beginIter;
        }

        const HashMapIterator<TpKey, TpVal> &end() {
            endIter.curBucket = &buckets[bucketNum - 1];
            endIter.curBucketEntry = endIter.curBucket->getTail();
            return endIter;
        }

        void print() {
            for (int i = 0; i < bucketNum; ++i) {
                DBG_LOGS("Bucket %d has %d numbers\n", i, buckets[i].getSize());
            }
        }

        ~HashMap() {

        }

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
                auto *listEntry = bucket->getHead()->getNext();

                while (listEntry != bucket->getTail()) {
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
