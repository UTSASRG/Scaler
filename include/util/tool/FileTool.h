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
    /**
     * Get the size of a file
     */
    long int getFileSize(FILE *file);

    /**
     * Find the split indexes of string srcStr separated by splitChar
     * [ret[2i],ret[2i+1]) marks the starting and ending indexes of segment i.
     * Notice the right bound is NOT inclusive, meaning the length of this string segment is ret[2i+1]-ret[2i]
     *
     * Repeated splitChar is treated as a single character.
     *
     * eg: Input ""
     *
     * @return An array of paired indexes.
     */
    std::vector<size_t> findStrSplit(std::string &srcStr, char splitChar);

    /**
     * Search for a binary keyword in another array
     * @param target Target array to search
     * @param keyword Keyword to search
     * @return The pointer of the first starting address of keyword in target
     */
    void *binCodeSearch(void *target, size_t targetSize, void *keyword, size_t keywordSize);

    std::string extractFileName_Linux(std::string pathName);
}


#endif //SCALER_FILETOOL_H
