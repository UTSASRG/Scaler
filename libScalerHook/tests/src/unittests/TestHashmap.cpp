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


int main() {
//    test1();
//    test2();
    test3();
//    test4();
//    test5();
//    test6();
    return 0;
}