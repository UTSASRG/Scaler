#include <util/tool/FileTool.h>

std::vector<size_t> scaler::findStrSplit(std::string &srcStr, char splitChar) {
    std::vector<size_t> splitPoints;
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

void *scaler::binCodeSearch(void *target, size_t targetSize, void *keyword, size_t keywordSize) {
    //Convert it to uint8* so that we can perform arithmetic operation on those pointers
    uint8_t *kwd = static_cast<uint8_t *>(keyword);
    uint8_t *tgt = static_cast<uint8_t *>(target);

    int i = 0, j = 0; //i is the index in target and j is the index in keyword
    uint8_t *beg = nullptr; //Markes the begging of the match

    while (i < targetSize && j < keywordSize) {
        if (tgt[i] == kwd[j]) {
            if (beg == nullptr) {
                //First match. It's a potential starting position.
                beg = tgt + i;
            }
            ++j;
        } else {
            //If tgt[i] != kwd[j] it means this is not the correct keyword. Reset beg and j.
            beg = nullptr;
            j = 0;
        }
        ++i;
    }
    // If j==keywordSize it means the previous loop exit because of this. Then it means a match if found.
    return j == keywordSize ? beg : nullptr;
}

long int scaler::getFileSize(FILE *file) {
    //The type of this return value is used by ftell. So it should be universal
    fseek(file, 0L, SEEK_END);
    long int fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);
    return fileSize;
}

std::string scaler::extractFileName_Linux(std::string pathName) {
    auto posi = pathName.find_last_of('/');
    if (posi == std::string::npos) {
        //Not found, return full string
        return pathName;
    } else {
        return pathName.substr(posi + 1, pathName.length() - posi);
    }
}