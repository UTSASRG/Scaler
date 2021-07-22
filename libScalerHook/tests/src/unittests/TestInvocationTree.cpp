#include <iostream>

#include <gtest/gtest.h>
#include <type/InvocationTree.h>
#include <thread>

TEST(INVOCATION_TREE, general) {
    scaler::SerilizableInvocationTree serializableTree;
    scaler::InvocationTreeNode *newNode11 = &serializableTree.treeRoot;
    newNode11->setFileID(11);

    scaler::InvocationTreeNode *newNode23 = new scaler::InvocationTreeNode();
    newNode23->setFileID(23);
    newNode11->addChild(newNode23);

    scaler::InvocationTreeNode *newNode22 = new scaler::InvocationTreeNode();
    newNode22->setFileID(22);
    newNode11->addChild(newNode22);

    scaler::InvocationTreeNode *newNode21 = new scaler::InvocationTreeNode();
    newNode21->setFileID(21);
    newNode11->addChild(newNode21);

    scaler::InvocationTreeNode *newNode34 = new scaler::InvocationTreeNode();
    newNode34->setFileID(34);
    newNode23->addChild(newNode34);

    scaler::InvocationTreeNode *newNode33 = new scaler::InvocationTreeNode();
    newNode33->setFileID(33);
    newNode23->addChild(newNode33);

    scaler::InvocationTreeNode *newNode32 = new scaler::InvocationTreeNode();
    newNode32->setFileID(32);
    newNode21->addChild(newNode32);

    scaler::InvocationTreeNode *newNode31 = new scaler::InvocationTreeNode();
    newNode31->setFileID(31);
    newNode21->addChild(newNode31);

    auto rlt = serializableTree.serializeRootNode();
    EXPECT_EQ(rlt.size(), 8);
    EXPECT_EQ(rlt[0]->getFileID(), 11);
    EXPECT_EQ(rlt[0]->firstChildIndex, 1);
    EXPECT_EQ(rlt[0]->getChildrenSize(), 3);

    EXPECT_EQ(rlt[1]->getFileID(), 21);
    EXPECT_EQ(rlt[1]->firstChildIndex, 4);
    EXPECT_EQ(rlt[1]->getChildrenSize(), 2);
    EXPECT_EQ(rlt[2]->getFileID(), 22);
    EXPECT_EQ(rlt[2]->firstChildIndex, -1);
    EXPECT_EQ(rlt[3]->getFileID(), 23);
    EXPECT_EQ(rlt[3]->firstChildIndex, 6);
    EXPECT_EQ(rlt[3]->getChildrenSize(), 2);


    EXPECT_EQ(rlt[4]->getFileID(), 31);
    EXPECT_EQ(rlt[5]->getFileID(), 32);
    EXPECT_EQ(rlt[6]->getFileID(), 33);
    EXPECT_EQ(rlt[7]->getFileID(), 34);
}




