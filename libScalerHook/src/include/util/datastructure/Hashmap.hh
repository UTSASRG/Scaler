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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <util/tool/Logging.h>
#include <pthread.h>
#include <util/datastructure/List.h>
#include <new>

namespace scaler {

    template<class TpKey,       // What is the key? A long or string
            class TpVal>        // What is the value there?
    class HashMap {

        /**
         * An entry to hash map.
         * Each entry has a lock
         */
        class HashBucket {
        public:
            class Entry {
            public:
                TpKey key;
                TpVal val;

                Entry() = default;

                Entry(const TpKey &key, const TpVal &val) : key(key), val(val) {

                }

                Entry(const Entry &rho) {
                    operator=(rho);
                };

                Entry &operator=(const Entry &rho) {
                    key = rho.key;
                    val = rho.val;
                    return *this;
                };

                bool operator==(const Entry &rho) {
                    return key == rho.key && val == rho.val;
                }
            };

            List<Entry> entryList; //There are always a head node
            // Each entry has a separate lock
            ssize_t count; // How many _entries are there in this list
            const ssize_t index;

            HashBucket(ssize_t index) : index(index) {
                count = 0;
            }

            HashBucket(const HashBucket &) = delete;

            const HashBucket &operator=(const HashBucket &) = delete;

            void print() {
                entryList.print();
            }
        };

        bool initialized = false;
        HashBucket *buckets; //There are bucketNum+1 nodes. The last node indicates the end of bucket.
        size_t bucketNum;     // How many buckets in total

        /**
         * Comparator.
         * 0: src==dst
         * -1: src<dst
         * 1: src>dst
         */
        typedef uint8_t (*Comparator)(const TpKey &src, const TpKey &dst);

        typedef ssize_t (*HashFunc)(const TpKey &key);

        HashFunc hfunc;
        Comparator kcmp;
    public:
        HashMap(const HashFunc &hfunc, const Comparator &kcmp, const ssize_t bucketNum = 4096) : hfunc(hfunc),
                                                                                                 kcmp(kcmp),
                                                                                                 initialized(false),
                                                                                                 buckets(nullptr),
                                                                                                 bucketNum(bucketNum) {
            assert(hfunc != nullptr && kcmp != nullptr);
            // Initialize all of these _entries.
            buckets = (HashBucket *) malloc(sizeof(HashBucket) * (bucketNum + 1));
            for (int i = 0; i < bucketNum + 1; ++i) {
                new(&buckets[i]) HashBucket(i);
            }
            initialized = true;
        }


        inline size_t hashIndex(const TpKey &key) {
            size_t hkey = hfunc(key);
            return hkey % bucketNum;
        }

        // Look up whether an entry is existing or not.
        // If existing, return true. *value should be carried specific value for this key.
        // Otherwise, return false.
        bool get(const TpKey &key, TpVal *&value) {
            assert(initialized);
            size_t hindex = hashIndex(key);

            typename HashBucket::Entry *hashEntry;
            HashBucket *bucket = getHashBucket(hindex);
            bool found = getHashEntry(key, bucket, hashEntry);
            if (found) {
                value = &hashEntry->val;
            }
            return found;
        }

        /**
         * Insert key:value pair into hashmap
         * If key exists, insert will overwrite the value
         *
         * @param key
         * @param val
         * @return
         */
        bool put(const TpKey &key, const TpVal &val) {
            assert(initialized);
            size_t hindex = hashIndex(key);

            HashBucket *bucket = getHashBucket(hindex);
            typename HashBucket::Entry *hashEntry;

            if (getHashEntry(key, bucket, hashEntry)) {
                hashEntry->val = val;
            } else {
                typename HashBucket::Entry entry(key, val);

                bucket->entryList.insertAfter(bucket->entryList.head, entry);
            }
            return true;
        }

        // Free an entry with specified
        bool erase(const TpKey &key) {
            assert(initialized);
            size_t hindex = hashIndex(key);

            auto *curBucket = getHashBucket(hindex);

            typename HashBucket::Entry *rlt;
            getHashEntry(key, curBucket, rlt);
            struct HashBucket *first = getHashBucket(hindex);
            typename List<typename HashBucket::Entry>::Entry *entry;
            bool isFound = getHashEntry(key, curBucket, entry);

            if (isFound) {
                entry->prev->next = entry->next;
                entry->next->prev = entry->prev;
                delete entry;
                entry = nullptr;
            }

            return isFound;
        }

    protected:
        inline HashBucket *getHashBucket(const ssize_t &hindex) {
            assert(hindex < bucketNum);
            return &buckets[hindex];
        }

        /**
         * Return the first found entry or the last element of entry list if not found
         * @return :Found entry or not
         */
        inline bool
        getHashEntry(const TpKey &key, HashBucket *bucket, typename HashBucket::Entry *&entry) {
            assert(bucket != nullptr);
            size_t hindex = hashIndex(key);
            bool found = false;
            if (bucket != nullptr) {
                auto *listEntry = bucket->entryList.firstEntry();

                while (listEntry != nullptr) {
                    if (kcmp(listEntry->value.key, key) == 0) {
                        found = true;
                        break;
                    }
                    listEntry = listEntry->next;
                }

                if (found)
                    entry = &listEntry->value;

            }
            return found;
        }

    public:

        class Iterator {
        public:
            Iterator(const Iterator &rho) {
                operator=(rho);
            }

            Iterator &operator=(const Iterator &rho) {
                curEntry = rho.curEntry;
                curBucket = rho.curBucket;
                hashMap = rho.hashMap;
                return *this;
            }

            Iterator &operator++() {
                bool hasNext = getNextEntry(curBucket, curEntry);
                assert(hasNext);

                return *this;
            }

            Iterator &operator++(int) // in postfix ++  /* parameter? */
            {
                return operator++();
            }

            inline bool operator==(const Iterator &rho) const {
                return curEntry == rho.curEntry && curBucket == rho.curBucket && hashMap == rho.hashMap;
            }

            inline bool operator!=(const Iterator &rho) const {
                return !operator==(rho);
            }

            const TpVal &operator*() {
                return val();
            }

            const TpVal &val() {
                assert(curEntry != nullptr);
                return curEntry->value.val;
            }

            const TpKey &key() {
                //User need to make sure there are still values before calling this function
                assert(curEntry != nullptr);
                return curEntry->value.key;
            }

            inline bool hasNext() {
                HashMap::HashBucket *bucket;
                typename List<typename HashMap::HashBucket::Entry>::Entry *entry;
                return getNextEntry(bucket, entry);
            }

            inline bool hasValue() {
                return !curEntry->root;
            }

        protected:

            HashMap::HashBucket *curBucket;          // which bucket at the moment? [0, nbucket-1]
            typename List<typename HashMap::HashBucket::Entry>::Entry *curEntry;          // which bucket at the moment? [0, nbucket-1]
            HashMap *hashMap;

            Iterator(HashMap *hashMap) : hashMap(hashMap), curBucket(hashMap->buckets),
                                         curEntry(curBucket->entryList.head) {

            }

            inline bool
            getNextEntry(HashMap::HashBucket *&bucket,
                         typename List<typename HashMap::HashBucket::Entry>::Entry *&entry) {
                bool hasNext = bucket->index != hashMap->bucketNum;
                entry = curEntry;
                bucket = curBucket;
                while (bucket->index < hashMap->bucketNum) {
                    if (entry != nullptr)
                        entry = entry->next;
                    if (entry == nullptr) {
                        //Reached the end of the list, try next bucket
                        bucket = &hashMap->buckets[bucket->index + 1];
                        entry = bucket->entryList.head;
                        continue;
                    } else {
                        //Found the next entry
                        break;
                    }
                }

                return hasNext;
            }

            inline bool
            getPrevEntry(HashMap::HashBucket *&bucket,
                         typename List<typename HashMap::HashBucket::Entry>::Entry *&entry) {
                entry = curEntry;
                bucket = curBucket;
                while (true) {
                    if (entry->prev == nullptr || entry->prev == curBucket->entryList.head) {
                        if (bucket->index - 1 >= 0) {
                            //Reached the end of the list, try prev bucket
                            bucket = &hashMap->buckets[bucket->index - 1];
                            entry = bucket->entryList.tail;
                            if (!entry->root)
                                //If it is not root node, then we need to search forward
                                return true;
                        } else {
                            //This is the first entry to the first bucket. There is no more elements ahead
                            bucket = &hashMap->buckets[0];
                            entry = bucket->entryList.head;
                            return false;
                        }
                    } else {
                        //If the previous node is both not null and not head, so we just get previous entry
                        entry = entry->prev;
                        return true;
                    }
                }


            }

            ~Iterator() {}


        };

        /**
         * Acquire the first entry of the hash table.
         * If there is no entry, then begin()==end()==head of the first bucket
         * @return
         */
        Iterator begin() {

            Iterator curIter(this);
            //CurIter will point to head at first. We should move it to the first entry.
            if (curIter.hasNext()) {
                ++curIter;
                return curIter;
            } else
                return end();
        }

        Iterator end() {
            Iterator curIter(this);
            curIter.curBucket = &buckets[bucketNum];
            curIter.curEntry = curIter.curBucket->entryList.tail;
            return curIter;
        }
    };
}
#endif
