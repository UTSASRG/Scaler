

#ifndef SCALER_SERIALIZABLE_H
#define SCALER_SERIALIZABLE_H
#include <cstdio>
namespace scaler {
    class Serializable {
    public:
        Serializable();

        virtual bool load();

        virtual bool save();

        virtual ~Serializable();
    };


}

#endif //SCALER_SERIALIZABLE_H
