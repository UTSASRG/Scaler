#include <util/tool/Serializable.h>
#include <cstdio>
#include <util/tool/Logging.h>
#include <exceptions/ScalerException.h>

scaler::Serializable::Serializable() {

}

scaler::Serializable::~Serializable() {
}

bool scaler::Serializable::load(FILE *fp) {
    fatalError("Deserialization is not implemented in the base class, use the subsequent one");
    return false;
}

bool scaler::Serializable::save(FILE *fp) {
    fatalError("Serialization is not implemented in the base class, use the subsequent one");
    return false;
}


