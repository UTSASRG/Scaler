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

        bool load() override;

        bool save() override;

    };


    class InvocationTreeNodeV1 : public SerializableMixIn {
    protected:
        FileID realFileID = -1;
        int64_t funcAddr = -1;
        FuncID extFuncID;
        int64_t startTimestamp = -1;
        int64_t endTimeStamp = -1;
        int64_t childrenSize = 0;

        InvocationTreeNodeV1 *parent = nullptr;
        InvocationTreeNodeV1 *firstChild = nullptr;
        InvocationTreeNodeV1 *nextSibling = nullptr;

        enum Type {
            UNSPECIFIED = -1,
            NORMAL_FUNC = 1,
            PTHREAD_FUNC = 2,
            SEMAPHORE_FUNC = 3
        };
        int8_t type = -1;
    public:
        InvocationTreeNodeV1();

        InvocationTreeNodeV1(InvocationTreeNodeV1 &) = delete;

        InvocationTreeNodeV1(InvocationTreeNodeV1 &&) = delete;

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
            this->extFuncID = reinterpret_cast<FuncID>(extFuncID);
        }

        inline int64_t getExtFuncID() {
            return extFuncID;
        }


        inline InvocationTreeNodeV1 *getParent() {
            return parent;
        }

        inline InvocationTreeNodeV1 *getFirstChild() {
            return firstChild;
        }

        inline InvocationTreeNodeV1 *getNextSibling() {
            return nextSibling;
        }

        bool load() override;

        bool save() override;

        InvocationTreeNodeV1 *addChild(InvocationTreeNodeV1 *childNode, ssize_t posi = -2);

        //void rmChild(size_t posi);

        int64_t firstChildIndex = Type::UNSPECIFIED;
    };

    class PthreadInvocationTreeNodeV1 : public InvocationTreeNodeV1 {
    public:
        PthreadInvocationTreeNodeV1();

        int64_t extraField1;
        int64_t extraField2;

        bool load() override;

        bool save() override;

    };

    class SemaphoreInvocationTreeNodeV1 : public InvocationTreeNodeV1 {
    public:
        SemaphoreInvocationTreeNodeV1();

        int64_t extraField1;

        bool load() override;

        bool save() override;
    };

    class SerilizableInvocationTreeV1 : Serializable {
    public:

        InvocationTreeNodeV1 treeRoot;
        scaler::ExtFuncCallHook *libPltHook;

        SerilizableInvocationTreeV1();

        explicit SerilizableInvocationTreeV1(std::string fileName);

        bool load() override;

        bool save() override;

        ~SerilizableInvocationTreeV1() override;

        inline pthread_t getTid() {
            return tid;
        }

        std::vector<InvocationTreeNodeV1 *> serializeRootNode();

    protected:
        pthread_t tid = 0;
        bool saveOnExit = true;
        std::string fileName;

    };

}