//
// Created by st on 1/10/21.
//

#ifndef SCALER_FILETOOL_H
#define SCALER_FILETOOL_H


#include <cstddef>
#include <cstdio>
#include <vector>
#include <sstream>

namespace scaler {
    long int getFileSize(FILE *file);

    std::vector<size_t> findStrSplit(std::string &srcStr, char splitChar);

    void *binarySearch(void *target, size_t targetSize, void *keyword, size_t keywordSize);

}


#endif //SCALER_FILETOOL_H
