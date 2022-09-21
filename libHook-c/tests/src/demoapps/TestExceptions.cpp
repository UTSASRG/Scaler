
#include <cstdio>

int main() {
    try {

        try {
            throw 3;
        } catch (int i) {
            printf("here\n");
            throw 5;
        }
    } catch (int i) {
        printf("here1\n");
    }

}