#ifndef SCALER_LIST_H
#define SCALER_LIST_H

#include <cassert>
#include <util/datastructure/ForwardIterator.h>

namespace scaler {
    template<typename T>
    class List;

    template<typename T>
    class ListIterator;


    template<typename T>
    class ListEntry {
    public:
        ListEntry() : prev(nullptr), next(nullptr), value() {

        }

        bool operator==(const ListEntry &rho) {
            return value == rho.value;
        }

        bool operator!=(const ListEntry &rho) {
            return !operator==();
        }

        ListEntry(const ListEntry &rho) {
            operator=(rho);
        };

        ListEntry &operator=(const ListEntry &rho) {
            prev = rho.prev;
            next = rho.next;
            value = rho.value;
            return *this;
        };

        virtual ~ListEntry() = default;

        inline T &getVal() {
            return value;
        }

        inline void setVal(const T &val) {
            value = val;
        }

        inline ListEntry<T> *getPrev() {
            return prev;
        }

        inline ListEntry<T> *getNext() {
            return next;
        }


    protected:
        ListEntry *prev;
        ListEntry *next;
        T value;

        friend class List<T>;

        friend class ListIterator<T>;
    };


    template<typename T>
    class ListIterator : public ForwardIterator<T>, ReverseIterator<T> {
    public:
        using List_ = List<T>;
        using ListEntry_ = ListEntry<T>;

        explicit ListIterator(List_ *list) : list(list) {
            curNode = list->head.next;
        }

        ListIterator &operator++() override {
            assert(curNode->next != nullptr && curNode->next != &list->tail);
            curNode = curNode->next;
            return *this;
        }

        ListIterator &operator++(int) override {
            return operator++();
        }

        ListIterator &operator--() override {
            assert(curNode->prev != nullptr && curNode->prev != &list->head);
            curNode = curNode->prev;
            return *this;
        }

        ListIterator &operator--(int i) override {
            return operator--();
        }

        bool operator!=(const Iterator &rho) override {
            return !operator==(rho);
        }

        bool operator==(const Iterator &rho) override {
            auto *rhoPtr = dynamic_cast<const ListIterator<T> *>(&rho);
            if (rhoPtr) {
                return list == rhoPtr->list && curNode == rhoPtr->curNode;
            } else {
                return false;
            }
        }

        T &operator*() override {
            assert(curNode != nullptr && curNode != &list->tail);
            return curNode->value;
        }

    protected:
        friend class List<T>;

        // which bucket at the moment? [0, nbucket-1]
        List_ *list;
        ListEntry_ *curNode;
    };


    /**
     * Double linked list
     * There are two dummy, default value: head and tail
     * head.prev === null, tail.next === null
     * If list is empty, head.next == tail, tail.prev=head
     * Nodes allocated by new, and should be freed by delete
     * @tparam T Value type
     */
    template<typename T>
    class List : public Iteratable<ListIterator<T>>, ReverseIteratable<ListIterator<T>> {
    public:
        using ListEntry_ = ListEntry<T>;

        List() : beginIter(this), endIter(this), rbeginIter(this), rendIter(this),size(0) {
            head.next = &tail;
            tail.prev = &head;

            beginIter.curNode = head.next;
            endIter.curNode = &tail;
            rbeginIter.curNode = tail.next;
            rendIter.curNode = &head;
        };

        List(const List<T> &rho) {
            operator=(rho);
        };

        List &operator=(List &rho) {
            beginIter = rho.beginIter;
            endIter = rho.endIter;
            rbeginIter = rho.rbeginIter;
            rendIter = rho.rendIter;

            for (auto &elem:rho) {
                insertAfter(tail.prev, elem);
            }
        }


        virtual bool isEmpty() {
            return head.next == &tail;
        }

        virtual void insertAfter(ListEntry_ *node, const T &val) {
            //Cannot insert value in a null node
            assert(node != nullptr);
            //Cannot insert after tail
            assert(node != &tail);

            //Create a net entry
            //todo: When new fails, it will throw an exception
            ListEntry_ *newEntry = new ListEntry_();

            newEntry->value = val;
            newEntry->next = node->next;
            newEntry->prev = node;
            node->next = newEntry;
            if (newEntry->next)
                newEntry->next->prev = newEntry;

            ++size;
        }

        virtual void erase(ListEntry_ *node) {
            assert(node != &head);
            assert(node != &tail);

            auto prev = node->prev;
            auto next = node->prev;
            prev->next = node->next;
            next->prev = node->prev;
            delete node;
            node = nullptr;

            --size;
        }

        virtual void pushBack( const T &val) {
            insertAfter(tail.prev, val);
        }


        virtual ~List() {
            ListEntry_ *curNode = head.next;
            while (curNode != &tail) {
                ListEntry_ *nextNode = curNode->next;
                delete curNode;
                curNode = nextNode;
            }
        }

        virtual inline ListEntry_ *getHead() {
            return &head;
        }

        virtual inline ListEntry_ *getTail() {
            return &tail;
        }

        virtual inline ssize_t getSize() {
            return size;
        }

        ListIterator<T> begin() override {
            return beginIter;
        }

        ListIterator<T> end() override {
            return endIter;
        }

        ListIterator<T> rbegin() override {
            return rbeginIter;
        }

        ListIterator<T> rend() override {
            return rendIter;
        }

    protected:
        friend class ListIterator<T>;

        //Head dummy node
        ListEntry_ head;
        //Tail dummy node
        ListEntry_ tail;

        ListIterator<T> beginIter;
        ListIterator<T> endIter;
        ListIterator<T> rbeginIter;
        ListIterator<T> rendIter;

        ssize_t size;

    };


}
#endif
