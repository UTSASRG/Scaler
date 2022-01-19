#include "pthread.h"
#include <util/tool/Serializable.h>
#include <list>
#include <stdio.h>
#include <cstdint>
#include <cassert>
#include <string>
#include <vector>
#include <util/hook/ExtFuncCallHook.hh>

namespace scaler {

    class SerializableMixIn : public Serializable {
    public:
        enum Type {
            UNSPECIFIED = -1,
            NORMAL_FUNC = 1,
            PTHREAD_FUNC = 2,
            SEMAPHORE_FUNC = 3
        };
        int8_t type = -1;
        int64_t firstChildIndex = Type::UNSPECIFIED;


        bool load(FILE *fp) override;

        bool save(FILE *fp) override;

    };


    class InvocationTreeNode : public SerializableMixIn {
    protected:
        int64_t realFileID = -1;
        int64_t funcAddr = -1;
        int64_t extFuncID;
        int64_t startTimestamp = -1;
        int64_t endTimeStamp = -1;
        int64_t childrenSize = 0;

        InvocationTreeNode *parent = nullptr;
        InvocationTreeNode *firstChild = nullptr;
        InvocationTreeNode *nextSibling = nullptr;

    public:
        InvocationTreeNode();

        InvocationTreeNode(InvocationTreeNode &) = delete;

        InvocationTreeNode(InvocationTreeNode &&) = delete;

        inline bool isEmpty() {
            assert(realFileID == -1 && funcAddr == -1 || realFileID != -1 && funcAddr != -1);
            return realFileID == -1 && realFileID == -1;
        }

        inline bool isLeaf() {
            return childrenSize == 0;
        }

        inline ssize_t getChildrenSize() {
            return childrenSize;
        }

        inline void setStartTimestamp(const ssize_t& startTimestamp) {
            this->startTimestamp = startTimestamp;
        }

        inline ssize_t getStartTimestamp() {
            return startTimestamp;
        }

        inline void setEndTimestamp(const ssize_t& endTimeStamp) {
            this->endTimeStamp = endTimeStamp;
        }

        inline ssize_t getEndTimestamp() {
            return endTimeStamp;
        }

        inline void setRealFileID(FileID fileID) {
            this->realFileID = fileID;
        }

        inline FileID getRealFileID() {
            return realFileID;
        }

        inline void setFuncAddr(void *funcAddr) {
            this->funcAddr = reinterpret_cast<int64_t>(funcAddr);
        }

        inline int64_t getFuncAddr() {
            return funcAddr;
        }

        inline void setExtFuncID(FuncID extFuncID) {
            this->extFuncID = reinterpret_cast<int64_t>(extFuncID);
        }

        inline int64_t getExtFuncID() {
            return extFuncID;
        }


        inline InvocationTreeNode *getParent() {
            return parent;
        }

        inline InvocationTreeNode *getFirstChild() {
            return firstChild;
        }

        inline InvocationTreeNode *getNextSibling() {
            return nextSibling;
        }

        bool load(FILE *fp) override;

        bool save(FILE *fp) override;

        InvocationTreeNode *addChild(InvocationTreeNode *childNode, ssize_t posi = -2);

        //void rmChild(size_t posi);

    };

    class PthreadInvocationTreeNode : public InvocationTreeNode {
    public:
        PthreadInvocationTreeNode();

        int64_t extraField1;
        int64_t extraField2;

        bool load(FILE *fp) override;

        bool save(FILE *fp) override;

    };

    class SemaphoreInvocationTreeNode : public InvocationTreeNode {
    public:
        SemaphoreInvocationTreeNode();

        int64_t extraField1;

        bool load(FILE *fp) override;

        bool save(FILE *fp) override;
    };

    class SerilizableInvocationTree : Serializable {
    public:

        InvocationTreeNode treeRoot;
        scaler::ExtFuncCallHook *libPltHook;

        SerilizableInvocationTree();

        explicit SerilizableInvocationTree(std::string fileName);

        bool load(FILE *fp) override;

        bool save(FILE *fp) override;

        ~SerilizableInvocationTree() override;

        inline pthread_t getTid() {
            return tid;
        }

        std::vector<InvocationTreeNode *> serializeRootNode();

    protected:
        pthread_t tid = 0;
        bool saveOnExit = true;
        std::string fileName;

    };

}