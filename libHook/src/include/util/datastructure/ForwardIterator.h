#ifndef SCALER_FORWARDITERATOR_H
#define SCALER_FORWARDITERATOR_H

namespace scaler {

    template<typename IteratorType>
    class Iterator {
    public:

        virtual bool operator==(const IteratorType &rho) const = 0;

        virtual bool operator!=(const IteratorType &rho) const = 0;
    };

    template<typename T, typename IteratorType>
    class ForwardIterator : public virtual Iterator<IteratorType> {
    public:

        virtual IteratorType &operator++() = 0;

        virtual const IteratorType operator++(int) = 0;

        virtual T &operator*() = 0;

    };

    template<typename T, typename IteratorType>
    class ReverseIterator : public virtual Iterator<IteratorType> {
    public:
        virtual IteratorType &operator--() = 0;

        virtual const IteratorType operator--(int) = 0;

        virtual T &operator*() = 0;
    };

    template<typename T>
    class Iteratable {
        virtual const T &begin() = 0;

        virtual const T &end() = 0;
    };

    template<typename T>
    class ReverseIteratable {

        virtual const T &rbegin() = 0;

        virtual const T &rend() = 0;
    };
}


#endif //SCALER_FORWARDITERATOR_H
