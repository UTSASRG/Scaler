/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <iostream>
#include <sys/times.h>
#include <unistd.h>
#include <cstdio>

using namespace std;

static void pr_times(clock_t, struct tms *, struct tms *);

static void do_cmd(char *);

int main(int argc, char *argv[]) {
    int i;
    setbuf(stdout, NULL);
    do_cmd(argv[i]);    /* once for each command-line arg */
    exit(0);
}

static void
do_cmd(char *cmd)        /* execute and time the "cmd" */
{
    struct tms tmsstart, tmsend;
    clock_t start, end;
    int status;


    if ((start = times(&tmsstart)) == -1)    /* starting values */
        cout << "times error" << endl;

    int a = 0;
    for (int i = 0; i < 1000000000; ++i) {
        a += i;
    }
    printf("%d\n", a);
    sleep(10);

    if ((end = times(&tmsend)) == -1)
        cout << "times error" << endl;
    pr_times(end - start, &tmsstart,
             &tmsend);
}

static void
pr_times(clock_t real, struct tms *tmsstart, struct tms *tmsend) {
    static long clktck = 0;

    if (clktck == 0)    /* fetch clock ticks per second first time */
        if ((clktck = sysconf(_SC_CLK_TCK)) < 0)
            cout << "sysconf error" << endl;
    printf(" real:  %7.2f(%ld)\n", real / (double) clktck, real);
    printf(" user:  %7.2f(%ld)\n",
           (tmsend->tms_utime - tmsstart->tms_utime) / (double) clktck, (tmsend->tms_utime - tmsstart->tms_utime));
    printf(" sys:   %7.2f(%ld)\n",
           (tmsend->tms_stime - tmsstart->tms_stime) / (double)
                   clktck, (tmsend->tms_stime - tmsstart->tms_stime));
    printf(" child user:   %7.2f(%ld)\n",
           (tmsend->tms_cutime - tmsstart->tms_cutime) /
           (double) clktck, (tmsend->tms_cutime - tmsstart->tms_cutime));
    printf(" child sys:    %7.2f(%ld)\n",
           (tmsend->tms_cstime - tmsstart->tms_cstime) /
           (double) clktck, (tmsend->tms_cstime - tmsstart->tms_cstime));
}