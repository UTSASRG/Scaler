
#include <type/InvocationTreeV1.h>
#include <assert.h>
#include <exceptions/ScalerException.h>
#include <exceptions/ErrCode.h>
#include <util/tool/Timer.h>
#include <vector>
#include <queue>
#include <util/hook/ExtFuncCallHookAsm.hh>

bool scaler::SerilizableInvocationTreeV1::load() {
    fatalError("Serialization of invocation tree not implemented")
    return false;
}


bool scaler::SerilizableInvocationTreeV1::save() {
    fatalError("save of invocation tree not implemented")
//    if (saveOnExit) {
//        auto appEndTimestamp = getunixtimestampms();
//        //Add information for root node
//        //treeRoot.setStartTimestamp(libPltHook->appStartTimestamp);
//        //treeRoot.setEndTimestamp(appEndTimestamp);
//        treeRoot.setRealFileID(0);
//
//        char fileName[255];
//        sprintf(fileName, "thread_%lu.bin", tid);
//        auto layerOrderedElem = serializeRootNode();
//
//
//        FILE *fp = fopen(fileName, "w");
//        if (!fp) {
//            ERR_LOG("Cannot open saving file");
//            return false;
//        }
//
//        for (ssize_t i = 0; i < layerOrderedElem.size(); ++i) {
//            auto &curElem = layerOrderedElem[i];
//
//            if ((curElem->getExtFuncID() == -1 || curElem->getFuncAddr() == -1 ||
//                 curElem->getRealFileID() == -1 || curElem->getStartTimestamp() == -1 ||
//                 curElem->getEndTimestamp() == -1) && curElem->getParent() != nullptr) {
//                //After previous op, all nodes other than the root node should be complete.
//                ERR_LOG("Program exits abnormally, one or more attributes are -1.");
//            }
//        }
//
//        for (ssize_t i = 0; i < layerOrderedElem.size(); ++i) {
//            if (!layerOrderedElem[i]->save(fp)) {
//                ERR_LOGS("Saving tree node %zd failed", i);
//                continue;
//            }
//        }
//        if (!fp) {
//            fclose(fp);
//        }
//        if (libPltHook) {
//            libPltHook->saveAllSymbolId();
//        }
//    }
    return true;
}

scaler::SerilizableInvocationTreeV1::SerilizableInvocationTreeV1() {
    //Record thread id
    tid = pthread_self();
}

//static pthread_mutex_t saveLock;

scaler::SerilizableInvocationTreeV1::~SerilizableInvocationTreeV1() {
//    pthread_mutex_lock(&saveLock);
//    save();
//    pthread_mutex_unlock(&saveLock);
}

scaler::SerilizableInvocationTreeV1::SerilizableInvocationTreeV1(std::string fileName) : fileName(fileName) {

}

std::vector<scaler::InvocationTreeNodeV1 *> scaler::SerilizableInvocationTreeV1::serializeRootNode() {
    //Stored in layer order
    std::vector<InvocationTreeNodeV1 *> saveStack;
    std::queue<InvocationTreeNodeV1 *> treeNodeQueue;

    //1.Insert all children into stack
    //todo: Write a builder
    treeNodeQueue.push(&treeRoot);
    saveStack.push_back(&treeRoot);

    InvocationTreeNodeV1 *curChild = nullptr;
    while (treeNodeQueue.size() != 0) {
        auto &curNode = treeNodeQueue.front();
        treeNodeQueue.pop();
        if (curNode->getFirstChild() != nullptr) {
            curNode->firstChildIndex = saveStack.size();
        }
        curChild = curNode->getFirstChild();
        //1.Insert all children into queue
        while (curChild != nullptr) {
            treeNodeQueue.push(curChild);
            saveStack.push_back(curChild);
            curChild = curChild->getNextSibling();
        }
    }
    return saveStack;
}

scaler::InvocationTreeNodeV1::InvocationTreeNodeV1() {
    type = Type::NORMAL_FUNC;
}


/**
 * Insert at the back of childNode[posi], posi starts with 0
 * If posi==-1, then insert at front
 * If posi==-size-1 or posi==-2 (default), then insert at back
 * @param childNode
 * @param posi
 * @return newIndex
 */
scaler::InvocationTreeNodeV1 *scaler::InvocationTreeNodeV1::addChild(scaler::InvocationTreeNodeV1 *childNode,
                                                                     ssize_t posi) {
    assert(posi >= -2 && posi < childrenSize);

    if (posi == -2)
        posi = childrenSize - 1;

    ssize_t curPosi = -1;
    scaler::InvocationTreeNodeV1 *curNode = this;
    while (curPosi != posi) {
        ++curPosi;
    }

    childNode->parent = this;
    childNode->nextSibling = firstChild;
    firstChild = childNode;
    ++childrenSize;
    return childNode;
}

bool scaler::InvocationTreeNodeV1::load() {
    fatalError("Serialization of invocation tree is not implemented")
    return false;
}

bool scaler::InvocationTreeNodeV1::save() {
    fatalError("save is not implemented")
//    if (!fwrite(&type, sizeof(type), 1, fp)) {
//        ERR_LOG("Failed to write type into file");
//        return false;
//    }
//    if (!fwrite(&realFileID, sizeof(realFileID), 1, fp)) {
//        ERR_LOG("Failed to write realFileID into file");
//        return false;
//    }
//    if (!fwrite(&funcAddr, sizeof(funcAddr), 1, fp)) {
//        ERR_LOG("Failed to write funcAddr into file");
//        return false;
//    }
//    if (!fwrite(&startTimestamp, sizeof(startTimestamp), 1, fp)) {
//        ERR_LOG("Failed to write startTimestamp into file");
//        return false;
//    }
//    if (!fwrite(&endTimeStamp, sizeof(endTimeStamp), 1, fp)) {
//        ERR_LOG("Failed to write endTimeStamp into file");
//        return false;
//    }
//    if (!fwrite(&firstChildIndex, sizeof(firstChildIndex), 1, fp)) {
//        ERR_LOG("Failed to write firstChildIndex into file");
//        return false;
//    }
//    if (!fwrite(&childrenSize, sizeof(childrenSize), 1, fp)) {
//        ERR_LOG("Failed to write childrenSize into file");
//        return false;
//    }
    return true;
}

scaler::PthreadInvocationTreeNodeV1::PthreadInvocationTreeNodeV1() {
    type = Type::PTHREAD_FUNC;
}

bool scaler::PthreadInvocationTreeNodeV1::load() {
    fatalError("load has not been implemented");
    return false;
}

bool scaler::PthreadInvocationTreeNodeV1::save() {
    fatalError("save has not been implemented");

//    if (!scaler::InvocationTreeNodeV1::save(fp)) {
//        return false;
//    }
//    if (!fwrite(&extraField1, sizeof(extraField1), 1, fp)) {
//        ERR_LOG("Cannot write pthread info into file");
//        return false;
//    }
//    if (!fwrite(&extraField2, sizeof(extraField2), 1, fp)) {
//        ERR_LOG("Cannot write pthread info into file");
//        return false;
//    }
    return true;
}

scaler::SemaphoreInvocationTreeNodeV1::SemaphoreInvocationTreeNodeV1() {
    type = Type::SEMAPHORE_FUNC;
}

bool scaler::SemaphoreInvocationTreeNodeV1::load() {
    fatalError("load has not been implemented");
    return false;
}

bool scaler::SemaphoreInvocationTreeNodeV1::save() {
    fatalError("save was not implemented");

//    if (!scaler::InvocationTreeNodeV1::save(fp)) {
//        return false;
//    }
//    if (!fwrite(&extraField1, sizeof(extraField1), 1, fp)) {
//        ERR_LOG("Cannot write semaphore info into file");
//    }
    return true;
}


bool scaler::SerializableMixIn::load() {
    return Serializable::load();
}

bool scaler::SerializableMixIn::save() {
    return Serializable::save();
}
