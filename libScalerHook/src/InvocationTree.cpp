
#include <type/InvocationTree.h>
#include <assert.h>
#include <exceptions/ScalerException.h>
#include <exceptions/ErrCode.h>
#include <util/tool/Timer.h>
#include <util/tool/Config.h>
#include <vector>
#include <queue>
#include <util/hook/ExtFuncCallHookAsm.hh>

void scaler::SerilizableInvocationTree::load(FILE *fp) {
    throwScalerException(ErrCode::FUNC_NOT_IMPLEMENTED, "load has not been implemented");
    if (fileName != "") {

    }

}


void scaler::SerilizableInvocationTree::save(FILE *fp) {
    if (saveOnExit) {
        auto appEndTimestamp = getunixtimestampms();
        //Add information for root node
//        treeRoot.setStartTimestamp(libPltHook->appStartTimestamp);
//        treeRoot.setEndTimestamp(appEndTimestamp);
        treeRoot.setRealFileID(0);

        char fileName[255];
        sprintf(fileName, "thread_%lu.bin", tid);
        auto layerOrderedElem = serializeRootNode();


        FILE *fp = NULL;
        fp = fopen(fileName, "w");

        for (ssize_t i = 0; i < layerOrderedElem.size(); ++i) {
            auto &curElem = layerOrderedElem[i];

//            if (curElem->getEndTimestamp() == -1) {
//                curElem->setEndTimestamp(appEndTimestamp);
//                ERR_LOG("Program exits abnormally, adding timestamp");
//            }

//            if (curElem->getRealFileID() == -1 && curElem->getParent() != nullptr) {
//                //todo: move this to ExtFuncCallHookAsm
//                int64_t callerFileID = curElem->getParent()->getRealFileID();
//                int64_t fileIDInCaller = curElem->getExtFuncID();
//                void *funcAddr = nullptr;
//                int64_t libraryID = -1;
//                assert(callerFileID != -1);
//                assert(fileIDInCaller != -1);
//                libPltHook->parseFuncInfo(callerFileID, fileIDInCaller, funcAddr, libraryID);
//                curElem->setFuncAddr(reinterpret_cast<int64_t>(funcAddr));
//                curElem->setRealFileID(libraryID);
//                ERR_LOGS("Program exits abnormally, parsing %ld:%ld for realAddr, funcAddr=%p",callerFileID,fileIDInCaller,funcAddr);
//            }

            if ((curElem->getExtFuncID() == -1 || curElem->getFuncAddr() == -1 ||
                 curElem->getRealFileID() == -1 || curElem->getStartTimestamp() == -1 ||
                 curElem->getEndTimestamp() == -1) && curElem->getParent() != nullptr) {
                //After previous op, all nodes other than the root node should be complete.
                ERR_LOG("Program exits abnormally, one or more attributes are -1.");
//                assert(false);
            }
        }

        for (ssize_t i = 0; i < layerOrderedElem.size(); ++i) {
            layerOrderedElem[i]->save(fp);
        }
        fclose(fp);
        if (libPltHook) {
            libPltHook->saveAllSymbolId();
        }
    }
}

scaler::SerilizableInvocationTree::SerilizableInvocationTree() {
    //Record thread id
    tid = pthread_self();
}

static pthread_mutex_t saveLock;

scaler::SerilizableInvocationTree::~SerilizableInvocationTree() {
    pthread_mutex_lock(&saveLock);
    save(nullptr);
    pthread_mutex_unlock(&saveLock);
}

scaler::SerilizableInvocationTree::SerilizableInvocationTree(std::string fileName) : fileName(fileName) {

}

std::vector<scaler::InvocationTreeNode *> scaler::SerilizableInvocationTree::serializeRootNode() {
    //Stored in layer order
    std::vector<InvocationTreeNode *> saveStack;
    std::queue<InvocationTreeNode *> treeNodeQueue;

    //1.Insert all children into stack
    //todo: Write a builder
    treeNodeQueue.push(&treeRoot);
    saveStack.push_back(&treeRoot);

    InvocationTreeNode *curChild = nullptr;
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

scaler::InvocationTreeNode::InvocationTreeNode() {
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
scaler::InvocationTreeNode *scaler::InvocationTreeNode::addChild(scaler::InvocationTreeNode *childNode,
                                                                 ssize_t posi) {
    assert(posi >= -2 && posi < childrenSize);

    if (posi == -2)
        posi = childrenSize - 1;

    ssize_t curPosi = -1;
    scaler::InvocationTreeNode *curNode = this;
    while (curPosi != posi) {
        ++curPosi;
    }

    childNode->parent = this;
    childNode->nextSibling = firstChild;
    firstChild = childNode;
    ++childrenSize;
    return childNode;
}

void scaler::InvocationTreeNode::load(FILE *fp) {
    throwScalerException(ErrCode::FUNC_NOT_IMPLEMENTED, "load has not been implemented");

}

void scaler::InvocationTreeNode::save(FILE *fp) {
    fwrite(&type, sizeof(type), 1, fp);
    fwrite(&realFileID, sizeof(realFileID), 1, fp);
    fwrite(&funcAddr, sizeof(funcAddr), 1, fp);
    fwrite(&startTimestamp, sizeof(startTimestamp), 1, fp);
    fwrite(&endTimeStamp, sizeof(endTimeStamp), 1, fp);
    fwrite(&firstChildIndex, sizeof(firstChildIndex), 1, fp);
    fwrite(&childrenSize, sizeof(childrenSize), 1, fp);
}

scaler::PthreadInvocationTreeNode::PthreadInvocationTreeNode() {
    type = Type::PTHREAD_FUNC;
}

void scaler::PthreadInvocationTreeNode::load(FILE *fp) {
    throwScalerException(ErrCode::FUNC_NOT_IMPLEMENTED, "load has not been implemented");

}

void scaler::PthreadInvocationTreeNode::save(FILE *fp) {
    scaler::InvocationTreeNode::save(fp);
    fwrite(&extraField1, sizeof(extraField1), 1, fp);
    fwrite(&extraField2, sizeof(extraField2), 1, fp);
}

scaler::SemaphoreInvocationTreeNode::SemaphoreInvocationTreeNode() {
    type = Type::SEMAPHORE_FUNC;
}

void scaler::SemaphoreInvocationTreeNode::load(FILE *fp) {
    throwScalerException(ErrCode::FUNC_NOT_IMPLEMENTED, "load has not been implemented");
}

void scaler::SemaphoreInvocationTreeNode::save(FILE *fp) {
    scaler::InvocationTreeNode::save(fp);
    fwrite(&extraField1, sizeof(extraField1), 1, fp);
}

void scaler::SerializableMixIn::load(FILE *fp) {
    Serializable::load(fp);
}

void scaler::SerializableMixIn::save(FILE *fp) {
    Serializable::save(fp);
}

scaler::RawRecordEntry::RawRecordEntry(scaler::SymID symId, scaler::FileID fileId, uint64_t timeStamp, Type operation)
        : symId(symId),
          fileId(fileId),
          timeStamp(timeStamp),
          operation(operation) {

}
