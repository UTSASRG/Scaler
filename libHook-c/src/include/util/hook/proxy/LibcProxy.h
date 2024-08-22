/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifndef SCALER_LIBCPROXY_H
#define SCALER_LIBCPROXY_H

extern "C" {

typedef int (*main_fn_t)(int, char **, char **);
#ifndef MANUAL_INSTALL

void exit(int __status) __attribute__ ((noreturn));


#endif
}
#endif //SCALER_LIBCPROXY_H
