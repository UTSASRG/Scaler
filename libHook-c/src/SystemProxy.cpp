/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include<util/hook/proxy/SystemProxy.h>
#include <unistd.h>
#include<util/tool/Logging.h>
#include <csignal>

//extern bool PARENT_CONTINUE;
//extern bool CHILD_CONTINUE;

//bool PARENT_CONTINUE=false;
//bool CHILD_CONTINUE=false;
__pid_t fork_proxy(void) {
    __pid_t forkRlt=fork();

//    if(forkRlt==0){
//        INFO_LOGS("Pausing on child pid=%d",getpid());
//        while(!CHILD_CONTINUE){
//            getchar();
//        }
//    }else{
//        INFO_LOGS("Pausing on parent childPid=%d",forkRlt);
//        while(!PARENT_CONTINUE){
//            getchar();
//        }
//    }
    return forkRlt;
}
