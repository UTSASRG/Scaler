#ifndef SCALER_FORWARDITERATOR_H
#define SCALER_FORWARDITERATOR_H

namespace scaler {


    class Iterator {
    public:

        virtual bool operator==(const Iterator &rho) const= 0;

        virtual bool operator!=(const Iterator &rho) const= 0;
    };

    template<typename T>
    class ForwardIterator : public virtual Iterator {
    public:

        virtual ForwardIterator<T> &operator++() = 0;

        virtual ForwardIterator<T> &operator++(int) = 0;

        virtual T &operator*() = 0;

    };

    template<typename T>
    class ReverseIterator : public virtual Iterator{
    public:
        virtual ReverseIterator<T> &operator--() = 0;

        virtual ReverseIterator<T> &operator--(int) = 0;

        virtual T &operator*() = 0;
    };

    template<typename T>
    class Iteratable {
        virtual const T& begin() = 0;

        virtual const T& end() = 0;
    };

    template<typename T>
    class ReverseIteratable {

        virtual const T& rbegin() = 0;

        virtual const T& rend() = 0;
    };
}


#endif //SCALER_FORWARDITERATOR_H
