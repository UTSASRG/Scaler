
#include <type/InvocationTree.h>
#include <assert.h>
#include <exceptions/ScalerException.h>
#include <exceptions/ErrCode.h>
#include <util/tool/Timer.h>
#include <util/tool/Config.h>
#include <vector>
#include <queue>

void scaler::SerilizableInvocationTree::load(FILE *fp) {
    throwScalerException(ErrCode::FUNC_NOT_IMPLEMENTED, "load has not been implemented");
    if (fileName != "") {

    }

}

void scaler::SerilizableInvocationTree::save(FILE *fp) {
    if (saveOnExit) {
        char fileName[255];
        sprintf(fileName, "thread_%p.bin", tid);
        auto layerOrderedElem = serializeRootNode();


        FILE *fp = NULL;
        fp = fopen(fileName, "w");
        for (size_t i = 0; i < layerOrderedElem.size(); ++i) {
            layerOrderedElem[i]->save(fp);
        }
        fclose(fp);
    }
}

scaler::SerilizableInvocationTree::SerilizableInvocationTree() {
    //Record thread id
    tid = pthread_self();
}

scaler::SerilizableInvocationTree::~SerilizableInvocationTree() {
    save(nullptr);
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
    fwrite(&fileID, sizeof(fileID), 1, fp);
    fwrite(&funcID, sizeof(funcID), 1, fp);
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
    fwrite(&extraFiled1, sizeof(extraFiled1), 1, fp);
    fwrite(&extraFiled2, sizeof(extraFiled2), 1, fp);
}

scaler::SemaphoreInvocationTreeNode::SemaphoreInvocationTreeNode() {
    type = Type::SEMAPHORE_FUNC;
}

void scaler::SemaphoreInvocationTreeNode::load(FILE *fp) {
    throwScalerException(ErrCode::FUNC_NOT_IMPLEMENTED, "load has not been implemented");
}

void scaler::SemaphoreInvocationTreeNode::save(FILE *fp) {
    scaler::InvocationTreeNode::save(fp);
    fwrite(&extraFiled1, sizeof(extraFiled1), 1, fp);
}

void scaler::SerializableMixIn::load(FILE *fp) {
    Serializable::load(fp);
}

void scaler::SerializableMixIn::save(FILE *fp) {
    Serializable::save(fp);
}
