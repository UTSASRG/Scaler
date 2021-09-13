#ifndef SCALER_LIST_H
#define SCALER_LIST_H

namespace scaler {

    /**
     * Double linked list
     * @tparam T Value type
     */
    template<typename T>
    class List {
    protected:
        /**
         * An entry to the list
         */
        class Entry {
        public:
            Entry *prev;
            Entry *next;
            T value;
            bool root=false;

            Entry() : prev(nullptr), next(nullptr), value() {

            }

            Entry(Entry &rho) = delete;

            Entry &operator=(Entry &rho) = delete;

//            Entry(Entry &rho) {
//                operator=(rho);
//            };
//
//            Entry &operator=(Entry &rho) {
//                prev = rho.prev;
//                next = rho.next;
//                value = rho.value;
//                return *this;
//            };
            inline bool hasNext() {
                return next != nullptr;
            }
        };

        Entry root;

        Entry *head;
        Entry *tail;

        List() : head(&root), tail(&root) {
            root.root=true;
        }

        bool isEmpty() {
            return head->next == nullptr;
        }

        inline void insertAfter(Entry *node, const T &val) {
            //Cannot insert value in a null node
            assert(node != nullptr);

            //Create a net entry
            //todo: When new fails, it will throw an exception
            Entry *newEntry = new Entry();

            if (!node->hasNext()) {
                tail = newEntry;
            }

            newEntry->value = val;
            newEntry->next = node->next;
            newEntry->prev = node;
            node->next = newEntry;
            if (newEntry->next)
                newEntry->next->prev = newEntry;
        }

        inline void remove(Entry *node) {
            //No element left
            assert(node != head);
            //Can't delete empty node
            assert(node != nullptr);

            if (!node->hasNext()) {
                tail = node->prev;
            }

            node->prev->next = node->next;
            node->next->prev = node->prev;
            delete node;
            node = nullptr;
        }

        inline Entry *firstEntry() {
            return head->next;
        }
    };


}
#endif
