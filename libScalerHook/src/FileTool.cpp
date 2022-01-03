#include <util/tool/FileTool.h>
#include <exceptions/ScalerException.h>
#include <exceptions/ErrCode.h>
#include <climits>
#include <unistd.h>
#include <cstring>


std::vector<ssize_t> scaler::findStrSplit(std::string &srcStr, char splitChar) {
    std::vector<ssize_t> splitPoints;
    //Augment the first and last character in a string with splitChar.
    //This make edge cases easier to handle
    std::stringstream ss;
    ss << splitChar << srcStr << splitChar;
    std::string augSrcStr = ss.str(); //Augmented srcStr

    for (int i = 1; i < augSrcStr.size(); ++i) {
        //Loop through every character in augSrcStr

        if (augSrcStr[i - 1] != augSrcStr[i]) {
            //If there's a character change, then it may be the staring/ending position of a text segment

            if (augSrcStr[i - 1] == splitChar) {
                //If current symbol is different from last symbol, and the previous symbol is splitchar.
                //Then previous location i should be marked as the begging of the begging of a text segment.
                //Here we are manipulating augmented string, there's one more character in the beginning.
                //We want to return the index of the original string. So we should push back i-1 rather than 1.

                //Mark the starting of this text segment as i-1
                splitPoints.emplace_back(i - 1);

            } else if (augSrcStr[i] == splitChar) {
                //If current symbol is different from last symbol, and the current symbol is splitchar.
                //Then current location i (right NOT inclusive) should be marked as the end of a text segment.
                //Here we are manipulating augmented string, there's one more character in the beginning.
                //We want to return the index of the original string. So we should push back i-1 rather than 1.

                //Mark the ending of this text segment as i-1
                splitPoints.emplace_back(i - 1);
            }

        }
    }
    return splitPoints;
}

long int scaler::getFileSize(FILE *file) {
    //The type of this return value is used by ftell. So it should be universal
    if (fseek(file, 0L, SEEK_END) != 0) {
        ERR_LOGS("fseek failed because: %s", strerror(errno));
        return -1;
    }
    long int fileSize = ftell(file);
    if (!fileSize) {
        ERR_LOGS("ftell failed because: %s", strerror(errno));
        return -1;
    }
    if (fseek(file, 0L, SEEK_SET) != 0) {
        ERR_LOGS("ftell failed because: %s", strerror(errno));
        return -1;
    }
    return fileSize;
}

bool scaler::extractFileName_Linux(std::string absolutePath, std::string &pathName, std::string &fileName) {
    auto posi = absolutePath.find_last_of('/');
    if (posi == std::string::npos) {
        //Not found, return full string
        ERR_LOGS("Path incorrect: %s", absolutePath.c_str());
        return false;
    } else {
        pathName = absolutePath.substr(0, posi);
        fileName = absolutePath.substr(posi + 1, absolutePath.length() - posi);
        return true;
    }
}

bool scaler::getPWD(std::string &retPwdPath) {
    auto scalerWorkDirPtr = getenv("SCALER_WORKDIR");

    if (!scalerWorkDirPtr) {
        //Use cwd
        char temp[PATH_MAX];

        if (getcwd(temp, PATH_MAX) != 0) {
            retPwdPath = std::string(temp);
            return true;
        } else {
            ERR_LOGS("getcwd failed because %s", strerror(errno));
            return false;
        }
    } else {
        //Use env variable
        retPwdPath = scalerWorkDirPtr;
        return true;
    }
}
