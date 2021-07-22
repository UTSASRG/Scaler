

#ifndef SCALER_SERIALIZABLE_H
#define SCALER_SERIALIZABLE_H
#include <cstdio>
namespace scaler {
    class Serializable {
    public:
        Serializable();

        virtual void load(FILE *fp);

        virtual void save(FILE *fp);

        virtual ~Serializable();
    };


}

#endif //SCALER_SERIALIZABLE_H
