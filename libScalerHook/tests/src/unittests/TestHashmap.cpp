#include <util/datastructure/Hashmap.hh>

uint8_t cmp(const int &src, const int &dst) {
    if (src == dst) {
        return 0;
    } else if (src > dst) {
        return 1;
    } else {
        return -1;
    }
}

ssize_t hfunc(const int &key) {
    return key;
};

#include <type/breakpoint.h>

void test1() {


    scaler::HashMap<int, int> hashMap(hfunc, cmp, 10);

    for (int i = 0; i < 20; ++i) {
        hashMap.put(i, i);
    }
    assert(hashMap.begin() != hashMap.end());

    auto iterBeg = hashMap.begin();
    auto iterEnd = hashMap.end();
    while (iterBeg != iterEnd) {
        fprintf(stderr, "%d iterBag!=iterENd=%s\n", iterBeg.val(), iterBeg == iterEnd ? "true" : "false");

        iterBeg++;
    }
}

void test2() {
    scaler::HashMap<int, int> hashMap(hfunc, cmp, 10);

    for (int i = 5; i < 10; ++i) {
        hashMap.put(i, i);
    }
    assert(hashMap.begin() != hashMap.end());

    auto iterBeg = hashMap.begin();
    auto iterEnd = hashMap.end();

    while (iterBeg != iterEnd) {
        fprintf(stderr, "%d iterBag!=iterENd=%s\n", iterBeg.val(), iterBeg == iterEnd ? "true" : "false");

        iterBeg++;
    }

}

void test3() {
    scaler::HashMap<int, int> hashMap(hfunc, cmp, 1);

    for (int i = 5; i < 10; ++i) {
        hashMap.put(i, i);
    }
    assert(hashMap.begin() != hashMap.end());

    auto iterBeg = hashMap.begin();
    auto iterEnd = hashMap.end();

    while (iterBeg != iterEnd) {
        fprintf(stderr, "%d iterBag!=iterENd=%s\n", iterBeg.val(), iterBeg == iterEnd ? "true" : "false");

        iterBeg++;
    }

}

void test4() {
    scaler::HashMap<int, int> hashMap(hfunc, cmp, 10);

    hashMap.put(5, 5);
    assert(hashMap.begin() != hashMap.end());

    auto iterBeg = hashMap.begin();
    auto iterEnd = hashMap.end();

    while (iterBeg != iterEnd) {
        fprintf(stderr, "%d iterBag!=iterENd=%s\n", iterBeg.val(), iterBeg == iterEnd ? "true" : "false");
        iterBeg++;
    }

}

void test5() {
    scaler::HashMap<int, int> hashMap(hfunc, cmp, 10);


    hashMap.put(5, 5);
    assert(hashMap.begin() != hashMap.end());

    auto iterBeg = hashMap.begin();
    auto iterEnd = hashMap.end();

    while (iterBeg != iterEnd) {
        fprintf(stderr, "%d iterBag!=iterENd=%s\n", iterBeg.val(), iterBeg == iterEnd ? "true" : "false");

        iterBeg++;
    }

}

void test6() {
    scaler::HashMap<int, int> hashMap(hfunc, cmp, 10);

    assert(hashMap.begin() == hashMap.end());

    auto iterBeg = hashMap.begin();
    auto iterEnd = hashMap.end();

    while (iterBeg != iterEnd) {
        fprintf(stderr, "%d iterBag!=iterENd=%s\n", iterBeg.val(), iterBeg == iterEnd ? "true" : "false");

        iterBeg++;
    }

}


uint8_t cmp(void *const &src, void *const &dst) {
    if (src > dst) {
        return 1;
    } else if (src == dst) {
        return 0;
    } else if (src < dst) {
        return -1;
    }
}

ssize_t hfunc(void *const &key) {
    return (ssize_t) key;
}


void test7() {
    scaler::HashMap<void *, scaler::Breakpoint> brkPointInfo(hfunc, cmp);         //Mapping fileID to PltCodeInfo


    scaler::Breakpoint a;
    scaler::Breakpoint *b;
    a.addr = reinterpret_cast<uint8_t *>(11);
    a.fileID = 12;
    a.funcID = 13;
    a.oriCode[0] = 1;
    a.oriCode[1] = 2;
    a.oriCode[2] = 3;
    a.oriCode[3] = 4;
    a.instLen = 4;
    brkPointInfo.put((void *) 11, a);

    brkPointInfo.get((void *) 11, b);
    assert(b->addr == (uint8_t *) 11);
    assert(b->fileID == 12);
    assert(b->funcID == 13);
    assert(b->oriCode[0] == 1);
    assert(b->oriCode[1] == 2);
    assert(b->oriCode[2] == 3);
    assert(b->oriCode[3] == 4);
    assert(b->instLen == 4);
    assert(a==*b);

}

int main() {
//    test1();
//    test2();
//    test3();
//    test4();
//    test5();
//    test6();
    test7();
    return 0;
}