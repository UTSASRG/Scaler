//
// Created by st on 1/10/21.
//

#ifndef SCALER_FILETOOL_H
#define SCALER_FILETOOL_H


#include <cstddef>
#include <cstdio>
#include <vector>
#include <sstream>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstring>
#include <unistd.h>
#include "Logging.h"

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
    std::vector<ssize_t> findStrSplit(std::string &srcStr, char splitChar);

    bool extractFileName(std::string absolutePath, std::string &pathName, std::string &fileName);

    bool getPWD(std::string &retPwdPath);

    /**
     * Open a new file and write to it
     * @param fd Returned file descriptor
     * @param rltMemAddr
     * @return Success or not
     */
    template<typename T>
    bool fOpen4Write(const char *fileName, int &fd, size_t fileSizeInBytes, T *&rltMemAddr){
        fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, (mode_t) 0644);
        if (fd == -1) {
            ERR_LOGS("Cannot open %s because:%s", fileName, strerror(errno));
            return false;
        }

        rltMemAddr = (T *) mmap(NULL, fileSizeInBytes,
                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (rltMemAddr == MAP_FAILED) {
            ERR_LOGS("Cannot mmap %s because:%s", fileName, strerror(errno));
            close(fd);
            return false;
        }

        if (ftruncate(fd, fileSizeInBytes) == -1) {
            ERR_LOGS("Cannot truncate %s because:%s", fileName, strerror(errno));
            close(fd);
            return false;
        }

        return true;
    }

    template<typename T>
    bool fClose(int &fd, size_t fileSizeInBytes, T *&rltMemAddr) {
        if (munmap(rltMemAddr, fileSizeInBytes) == -1) {
            ERR_LOG("Cannot close file");
            return false;
        }
        close(fd);
        return true;
    }

}


#endif //SCALER_FILETOOL_H
