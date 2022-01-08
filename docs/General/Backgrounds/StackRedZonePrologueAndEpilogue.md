Based on [Stack Frames. Red Zone, Prologue and Epilogue on x86-64, demystified. Demo on the GNU Debugger.](https://www.youtube.com/watch?v=-52uAgw60yM)

The program

```c++
long leaf(long a, long b, long c) {
    int var = 90;
    int testarray[5];
    long x = a + 7;
    long y = b + 9;
    long z = c + 2;
    long sum = x + y + z;
    return x * y * z + sum;
}

long branch(long a, long b, long c, long d,
            long e, long f, long g, long h) {
    long *pointer;
    int testarrayxxx[20];
    long x = a * b * c * d * e * f * g * h;
    pointer = &x;
    long y = a + b + c + d + e + f + g + h;
    long z = leaf(x, y, x % y);
    return z;
}
 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
int main() {
    long answer;
    answer = branch(10, 20, 30, 40, 50, 60, 70, 80);
    return 0;
}
```
