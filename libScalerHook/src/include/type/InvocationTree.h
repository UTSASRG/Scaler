#include "pthread.h"
#include <util/tool/Serializable.h>
#include <list>
#include <stdio.h>
#include <cstdint>
#include <cassert>
#include <string>
#include <vector>

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


        void load(FILE *fp) override;

        void save(FILE *fp) override;

    };

    class InvocationTreeNode : public SerializableMixIn {
    protected:
        ssize_t fileID = -1;
        ssize_t funcID = -1;
        int64_t startTimestamp = -1;
        int64_t endTimeStamp = -1;

        InvocationTreeNode *parent = nullptr;
        InvocationTreeNode *firstChild = nullptr;
        InvocationTreeNode *nextSibling = nullptr;
        ssize_t childrenSize = 0;

    public:
        InvocationTreeNode();

        InvocationTreeNode(InvocationTreeNode &) = delete;

        InvocationTreeNode(InvocationTreeNode &&) = delete;

        inline bool isEmpty() {
            assert(fileID == -1 && funcID == -1 || fileID != -1 && funcID != -1);
            return fileID == -1 && funcID == -1;
        }

        inline bool isLeaf() {
            return childrenSize == 0;
        }

        inline ssize_t getChildrenSize() {
            return childrenSize;
        }

        inline void setStartTimestamp(ssize_t startTimestamp) {
            this->startTimestamp = startTimestamp;
        }

        inline ssize_t getStartTimestamp() {
            return startTimestamp;
        }

        inline void setEndTimestamp(ssize_t endTimeStamp) {
            this->endTimeStamp = endTimeStamp;
        }

        inline ssize_t getEndTimestamp() {
            return endTimeStamp;
        }

        inline void setFileID(size_t fileID) {
            this->fileID = fileID;
        }

        inline ssize_t getFileID() {
            return fileID;
        }

        inline void setFuncID(size_t funcID) {
            this->funcID = funcID;
        }

        inline ssize_t getFuncID() {
            return funcID;
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

        void load(FILE *fp) override;

        void save(FILE *fp) override;

        InvocationTreeNode *addChild(InvocationTreeNode *childNode, ssize_t posi = -2);

        //void rmChild(size_t posi);

    };


    class PthreadInvocationTreeNode : public InvocationTreeNode {
    public:
        PthreadInvocationTreeNode();

        int64_t extraFiled1;
        int64_t extraFiled2;

        void load(FILE *fp) override;

        void save(FILE *fp) override;

    };

    class SemaphoreInvocationTreeNode : public InvocationTreeNode {
    public:
        SemaphoreInvocationTreeNode();

        int64_t extraFiled1;

        void load(FILE *fp) override;

        void save(FILE *fp) override;
    };

    class SerilizableInvocationTree : Serializable {
    public:

        InvocationTreeNode treeRoot;

        SerilizableInvocationTree();

        explicit SerilizableInvocationTree(std::string fileName);

        void load(FILE *fp) override;

        void save(FILE *fp) override;

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