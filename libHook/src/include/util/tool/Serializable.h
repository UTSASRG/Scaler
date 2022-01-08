

#ifndef SCALER_SERIALIZABLE_H
#define SCALER_SERIALIZABLE_H
#include <cstdio>
namespace scaler {
    class Serializable {
    public:
        Serializable();

        virtual bool load(FILE *fp);

        virtual bool save(FILE *fp);

        virtual ~Serializable();
    };


}

#endif //SCALER_SERIALIZABLE_H
