/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
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
