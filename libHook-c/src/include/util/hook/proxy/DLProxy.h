/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifndef SCALER_DLPROXY_H
#define SCALER_DLPROXY_H

#include <dlfcn.h>

namespace scaler{
   void* dlopen_proxy(const char *__file, int __mode) __THROWNL;

   /* Find the run-time address in the shared object HANDLE refers to
      of the symbol called NAME.  */
   void *dlsym_proxy(void *__restrict __handle, const char *__restrict __name) __THROW __nonnull ((2));

   #ifdef __USE_GNU
   void *dlmopen_proxy(Lmid_t __nsid, const char *__file, int __mode) __THROWNL;

   int dlclose_proxy (void *__handle) __THROWNL __nonnull ((1));

   void *dlvsym_proxy(void *__restrict __handle,
               const char *__restrict __name,
               const char *__restrict __version)
   __THROW __nonnull ((2, 3));
}
#endif



#endif //SCALER_DLPROXY_H
