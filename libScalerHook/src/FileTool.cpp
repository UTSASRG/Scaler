#include <util/tool/FileTool.h>

std::vector<size_t> scaler::findStrSplit(std::string &srcStr, char splitChar) {
    std::vector<size_t> splitPoints;
    std::stringstream ss;
    ss << splitChar << srcStr << splitChar;

    std::string augStr = ss.str();

    for (int i = 1; i < augStr.size(); ++i) {
        if (augStr[i - 1] != augStr[i]) {
            if (augStr[i - 1] == splitChar) {
                splitPoints.emplace_back(i-1); //String Segment Begin
            } else if (augStr[i] == splitChar) {
                splitPoints.emplace_back(i-1); //String Segment End
            }

        }
    }
    return splitPoints;
}

void *scaler::binarySearch(void *target, size_t targetSize, void *keyword, size_t keywordSize) {
    uint8_t *kwd = static_cast<uint8_t *>(keyword);
    uint8_t *tgt = static_cast<uint8_t *>(target);

    int i = 0, j = 0;
    uint8_t *beg = nullptr;
    while (i < targetSize && j < keywordSize) {
        if (tgt[i] == kwd[j]) {
            if (beg == nullptr) {
                beg = tgt + i;
            }
            ++j;
        } else {
            beg = nullptr;
            j = 0;
        }
        ++i;
    }
    return j == keywordSize ? beg : nullptr;
}

long int scaler::getFileSize(FILE *file) {
    fseek(file, 0L, SEEK_END);
    long int fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);
    return fileSize;
}