# Data format

Replace **boldface** with actual value.

eg: **number**.txt should have the form of 1.txt, 2.txt, 1958.txt .etc

## FileName


| Name template              | Comments                                            | Example                                     |
| -------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| thread_0x**THREAD_ID**.bin | A serialized tree structure                     | See below |
| symbol.json                | Map fileID to fildName, and map (fileID, funcID) to function name. If there are no symbol, a nullptr will present. Check example. | {"0":{"fileName":"/home/st/Projects/Scaler/cmake-build-debug/libScalerHook/tests/libScalerHook-demoapps-Pthread","funcNames":{"0":"printf","1":"_ZN6scaler18ExtFuncCallHookAsm7getInstEv","10":"_ZNSt8ios_base4InitC1Ev","11":"_ZN6scaler18ExtFuncCallHookAsm15saveAllSymbolIdEv","2":"_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7compareEPKc","3":"_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE5c_strEv","4":"pthread_join","5":"pthread_create","6":"_Z7installPFbNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES4_E","7":"__cxa_atexit","8":"_ZN6scaler18ExtFuncCallHookAsm16saveCommonFuncIDEv","9":"fprintf"}},"10":{"fileName":"/usr/lib/x86_64-linux-gnu/ld-2.31.so","funcNames":null},"2":{"fileName":"","funcNames":null},"3":{"fileName":"/usr/lib/x86_64-linux-gnu/libgcc_s.so.1","funcNames":null},"4":{"fileName":"/usr/lib/x86_64-linux-gnu/libm-2.31.so","funcNames":null},"5":{"fileName":"/usr/lib/x86_64-linux-gnu/libdl-2.31.so","funcNames":null},"6":{"fileName":"/usr/lib/x86_64-linux-gnu/libc-2.31.so","funcNames":null},"7":{"fileName":"/usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.28","funcNames":null},"8":{"fileName":"/usr/lib/x86_64-linux-gnu/libpthread-2.31.so","funcNames":null},"9":{"fileName":"/home/st/Projects/Scaler/cmake-build-debug/libScalerHook/libScalerHook-HookManualAsm.so","funcNames":null}} |
| commonFuncId.json               | Save pthread and semaphore ids in each file. This is a convenient method for programmer to test whether a function id belongs to pthread/semaphore | {"0":{"pthread":[5,4],"semaphore":null},"10":{"pthread":null,"semaphore":null},"2":{"pthread":null,"semaphore":null},"3":{"pthread":null,"semaphore":null},"4":{"pthread":null,"semaphore":null},"5":{"pthread":null,"semaphore":null},"6":{"pthread":null,"semaphore":null},"7":{"pthread":null,"semaphore":null},"8":{"pthread":null,"semaphore":null},"9":{"pthread":null,"semaphore":null}} |



### thread_0x**THREAD_ID**.bin

Data stored in layer-order traverse of a tree. The format is decided by the first attribute "type". Which is an integer defined in SerializableMixIn::Type. Same type has same size, different type has different size.

Use [Source Code](https://github.com/UTSASRG/Scaler/blob/dev-libScalerHook-brkpointptrace/libScalerHook/src/include/type/InvocationTree.h) as a reference to prevent bugs caused by outdated documentation.

For example:

<img src="imgs/libScalerHook/libScalerhookDataformat/image-20210722161257130.png" alt="image-20210722161257130" style="zoom:50%;" />

<img src="imgs/libScalerHook/libScalerhookDataformat/image-20210722161311401.png" alt="image-20210722161311401" style="zoom:50%;" />

SerializableMixIn::Type

| Name           | Value | Comments                              |
| -------------- | ----- | ------------------------------------- |
| UNSPECIFIED    | -1    | Should not happen                     |
| NORMAL_FUNC    | 1     | All other functions                   |
| PTHREAD_FUNC   | 2     | PTHREAD_FUNC+pthread specific fields  |
| SEMAPHORE_FUNC | 3     | NORMAL_FUNC+semaphore specific fields |

### NORMAL_FUNC

[Source Code](https://github.com/UTSASRG/Scaler/blob/c2f9eefa373754068e0f542be5380ebef0a454c1/libScalerHook/src/InvocationTree.cpp#L114)

| Name                               | Size              | Comments                                                     |
| ---------------------------------- | ----------------- | ------------------------------------------------------------ |
| SerializableMixIn::type            | 1 byte (int8_t)   | Use this to determine how many types are there               |
| InvocationTreeNode::fileID         | 8 bytes (int64_t) | Use this fileID to index symbol.json to get executable name  |
| InvocationTreeNode::funcID         | 8 bytes (int64_t) | Use fileID and this funcID to index symbol.json to get symbol name |
| InvocationTreeNode::startTimestamp | 8 bytes (int64_t) | Unix timestamp time in microseconds $1$ second=$10^6$ micro seconds |
| InvocationTreeNode::endTimeStamp   | 8 bytes (int64_t) | Unix timestamp time in microseconds $1$ second=$10^6$ micro seconds |
| SerializableMixIn::firstChildIndex | 8 bytes (int64_t) | The index for array item (Rather than index in bytes).<br />Should be used for error check. |
| InvocationTreeNode::childrenSize   | 8 bytes (int64_t) | How many children a node ahs                                 |

### PTHREAD_FUNC

[Source Code](https://github.com/UTSASRG/Scaler/blob/c2f9eefa373754068e0f542be5380ebef0a454c1/libScalerHook/src/include/type/InvocationTree.h#L117)

| Name        | Size             | Comments                                  |
| ----------- | ---------------- | ----------------------------------------- |
| ......      | ......           | The same as NORMAL_FUNC                   |
| extraField1 | 8bytes (int64_t) | Different functions have special meanings |
| extraField2 | 8bytes (int64_t) | Different functions have special meanings |

For entire pthread function list check: [Source Code]( https://github.com/UTSASRG/Scaler/blob/c2f9eefa373754068e0f542be5380ebef0a454c1/libScalerHook/src/include/util/hook/ExtFuncCallHook_Linux.hh#L42)

| Function              | extraField1                       | extraField2                       |
| --------------------- | ----------------------------------------- | ----------------------------------------- |
| PTHREAD_CREATE        | Newly created thread's thread id (pthread_t, or hex value if converted to void*) | Empty |
| PTHREAD_*****JOIN*****    | Which thread the current thread is trying to join  (pthread_t, or hex value if converted to void*) | Empty |
| PTHREAD_MUTEX_*****   | Which lock current thread is trying to manipulate (mutex_t, or hex value if converted to void*) | Empty |
| PTHREAD_RWLOCK_*****  | Which rwlock current thread is trying to manipulate (pthread_rwlock_t, or hex value if converted to void*) | Empty |
| PTHREAD_COND_*****WAIT | Which condition variable current thread is trying to manipulate (cond_t, or hex value if converted to void*) | The mutex lock corresponding to this conditional variable (mutex_t, or hex value if converted to void*) |
| PTHREAD_COND_**{SIGNAL/BROADCAST}** | Which condition variable current thread is trying to manipulate (cond_t, or hex value if converted to void*) | Empty |
| PTHREAD_SPIN_*****    | Which spinlock current thread is trying to manipulate (pthread_spinlock_t, or hex value if converted to void*) | Empty |
| PTHREAD_BARRIER_***** | Which barrier current thread is trying to manipulate (barrier_t, or hex value if converted to void*) | Empty |


### SEMAPHORE_FUNC

[Source Code](https://github.com/UTSASRG/Scaler/blob/c2f9eefa373754068e0f542be5380ebef0a454c1/libScalerHook/src/include/type/InvocationTree.h#L130)

| Name        | Size             | Comments                                  |
| ----------- | ---------------- | ----------------------------------------- |
| ......      | ......           | The same as NORMAL_FUNC                   |
| extraField1 | 8bytes (int64_t) | Different functions have special meanings |

For entire pthread function list check: [Source Code]( https://github.com/UTSASRG/Scaler/blob/c2f9eefa373754068e0f542be5380ebef0a454c1/libScalerHook/src/include/util/hook/ExtFuncCallHook_Linux.hh#L75)

| Function              | extraField1                       | extraField2                       |
| --------------------- | ----------------------------------------- | ----------------------------------------- |
| SEM_* | Which signal current thread is trying to manipulate<br />Semaphore id (sem_t, or hex value if converted to void*) | Empty |

