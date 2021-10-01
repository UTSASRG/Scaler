#ifndef SCALER_LIST_H
#define SCALER_LIST_H

namespace scaler {

    /**
     * Double linked list
     * @tparam T Value type
     */
    template<typename T>
    class List {
    public:
        /**
             * An entry to the list
             */
        class Entry {
        public:
            Entry *prev;
            Entry *next;
            T value;
            bool root = false;

            Entry() : prev(nullptr), next(nullptr), value() {

            }

            bool operator==(const Entry &rho) {
                return value == rho.value;
            }

            Entry(const Entry &rho) {
                operator=(rho);
            };

            Entry &operator=(const Entry &rho) {
                prev = rho.prev;
                next = rho.next;
                value = rho.value;
                return *this;
            };

            inline bool hasNext() {
                return next != nullptr;
            }
        };
    protected:

        Entry root;

        bool isEmpty() {
            return head->next == nullptr;
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

    public:

        inline Entry *firstEntry() {
            return head->next;
        }

        List() : head(&root), tail(&root) {
            root.root = true;
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

        Entry *head;
        Entry *tail;

    };


}
#endif
