```
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
```
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

int main() {
    long answer;
    answer = branch(10, 20, 30, 40, 50, 60, 70, 80);
    return 0;
}
```
