/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/

#include  <stdio.h>
#include  <signal.h>
#include <cstdlib>

long prev_fact, i;

void SIGhandler(int);

void SIGhandler(int sig) {
    printf("\nReceived a SIGUSR1.  The answer is %ld! = %ld\n",
           i - 1, prev_fact);
}

int main(void) {
    long fact;

    printf("Factorial Computation:\n\n");
    signal(SIGUSR1, SIGhandler);
    for (prev_fact = i = 1;; i++, prev_fact = fact) {
        fact = prev_fact * i;
        if (fact < 0){
            printf("Raise SIGUSR1\n");
            raise(SIGUSR1);
            break;
        }
        else if (i % 3 == 0)
            printf("     %ld! = %ld\n", i, fact);
    }
    printf("Raise SIGUSR2\n");
    raise(SIGUSR2);
    printf("Raise signal alarm\n");
    raise(SIGALRM);
    printf("Raise signal term\n");
    raise(SIGTERM);
    printf("Raise complete\n");

    return 0;
}

