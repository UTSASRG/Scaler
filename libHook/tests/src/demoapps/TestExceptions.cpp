
#include <cstdio>

int main() {
    try {
        throw 3;
    } catch (int i) {
        printf("here");
    }
}