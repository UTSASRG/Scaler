#if !defined(DOUBLETAKE_SOCKETOPS_H)
#define DOUBLETAKE_SOCKETOPS_H

/*
 * @file   socketops.hh
 * @brief  handle socket request
 * @author Hongyu Liu
 * 
 * */

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <list>
#include <netinet/in.h>

#include "hashfuncs.hh"
#include "hashmap.hh"
#include "internalheap.hh"
#include "log.hh"
#include "mm.hh"
#include "real.hh"
#include "sysrecord.hh"
#include "spinlock.hh"
#include "threadstruct.hh"
#include "xdefines.hh"

typedef enum fdType {
	FD_SYSTEM = 0,
	FD_FILE,
	FD_SOCKET,
	FD_PIPE,
	FD_SOCKETPAIR,

	FD_DUP_SOCKET,
	FD_DUP_FILE,
} fdType;

#define DUP_NUMBER_PER_FD 4

	// for FD_SOCKET, just save data to the same one
	// for FD_PIPE or FD_SOCKETPAIR, only update the type
	// for FD_DUP_XX, save real obj to array and update counter
	struct socketCachedDataInfo {	
		// file descriptor 
		int fd;
		fdType type;
		// these fields are used for recording data
		int totalsize;
		int remaining; 
		char* head; //cached data pointer
		char* current; 
		// used for FD_DUP_XX
		socketCachedDataInfo* realObj[DUP_NUMBER_PER_FD]; // if type is FD_DUP_XX, current points to actual obj
		int curRealObj;
		bool isOldDup;
		// used for close
		int aliveCounter; // reused times

		// for rolback
		fdType bakType;
		int bakAliveCounter;
  };

extern __thread int cache_fd;
extern __thread socketCachedDataInfo* cache_socket_data;

class socketops {

	public:

	socketops() {}

	// How to initalize
	void initialize() {
		// Initialialize the hashmap to store socket.
		_socketMap.initialize(HashFuncs::hashInt, HashFuncs::compareInt, xdefines::SOCKET_MAP_SIZE);
		fdcounter = 0;
	}

	bool checkSocketType(int fd, fdType type) {
		bool ret = false;
		socketCachedDataInfo* sock = NULL;
    if(fd == cache_fd) {
      sock = cache_socket_data;
    } else if(_socketMap.find(fd, sizeof(int), &sock)) {
      cache_fd = fd;
      cache_socket_data = sock;
		} 

    if(sock != NULL) {
			if(type == sock->type){
				ret = true;
			}
    }
		return ret;
	}

	// search socket info by fd
	socketCachedDataInfo* getSocketInfo(int fd){
		socketCachedDataInfo* sock = NULL;
		_socketMap.find(fd, sizeof(int), &sock);
		return sock;
	}

	socketCachedDataInfo* saveSocketInfo(int fd, fdType type){
		socketCachedDataInfo* sock = getSocketInfo(fd);
		if(sock == NULL){
			sock = createNewSock(fd, type);
			// Insert this socket into the hash map.
			_socketMap.insert(fd, sizeof(int), sock);
		}else{
			sock->type = type;
			sock->aliveCounter++;
		}

		return sock;
	}

	// save a pair socket, fd should be two elements array
	void savePairSock(int pairfd[], int ret, eRecordSyscall sc, fdType type){
		// Save fd info
		_sysrecord.recordPairOps(sc, pairfd, ret);

		int i;
		for(i = 0; i<2; i++){
			int fd = pairfd[i];
			if(fd != -1) {
				saveSocketInfo(fd, type);
			}
		}
		PRINF("save %d pair %d, %d. ret %d\n", type, pairfd[0], pairfd[1], ret);
	}

	// save socketpair fd
	void savePairFD(int sv[], int ret){
		savePairSock(sv, ret, E_SYS_SOCKET_PAIR, FD_SOCKETPAIR);
	}

	// save two fds of pipe, treat them as socket.
	// we use "type" to distinguish socket.
	void savePipeFD(int pipefd[], int ret){
		savePairSock(pipefd, ret, E_SYS_SOCKET_PIPE, FD_PIPE);
	}

	// save fd info, when a new connection is accepted.
	// Typically, it is called in accept function
	void saveNewSockFD(int fd) {
		// Save fd info
		_sysrecord.recordSocketOps(E_SYS_SOCKET_NEW, fd);

		if(fd != -1) {
			saveSocketInfo(fd, FD_SOCKET);

			PRINF("save new socket %d\n", fd);
		}
	}

	void saveNewSockFDAll(int fd, struct sockaddr* addr, socklen_t* addrlen) {
		// Save fd info
		_sysrecord.recordSocketOps(E_SYS_SOCKET_NEW, fd, addr, addrlen);

		if(fd != -1) {
			saveSocketInfo(fd, FD_SOCKET);

			PRINF("save new socket for all %d\n", fd);
		}
	}

	void saveSockConn(int fd, int ret){
		_sysrecord.recordSocketOps(E_SYS_SOCKET_CONN, fd, ret);	
	}

	void saveSockName(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int ret){
		saveName(E_SYS_SOCKET_NAME, sockfd, addr, addrlen, ret); 
	}

	void savePeerName(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int ret){
		saveName(E_SYS_SOCKET_PEERNAME, sockfd, addr, addrlen, ret); 
	}

	//cache data
	void saveSockRecvData(int fd, void* data, int size){
		if(size < 0){
			_sysrecord.recordSocketOps(E_SYS_SOCKET_RECV, fd, data, size); 
		}else{
			void *p = saveData(fd, data, size);
			if(p != NULL){
				_sysrecord.recordSocketOps(E_SYS_SOCKET_RECV, fd, p, size);
			}
		}
	}

  void saveSockRecvFromData(int sockfd, void* data, struct sockaddr* from, socklen_t* fromlen, int ret){
    if(ret < 0){
      _sysrecord.recordSocketOps(E_SYS_SOCKET_RECVFROM, sockfd, from, fromlen, ret); 
    }else{
      void *p = saveData(sockfd, data, ret);
      if(p != NULL){
        _sysrecord.recordSocketOps(E_SYS_SOCKET_RECVFROM, sockfd, from, fromlen, p, ret); 
      }
    }
  }

	void saveSockSendmsg(int fd, int ret) {
		_sysrecord.recordSocketOps(E_SYS_SOCKET_SENDMSG, fd, ret);
	}

	//check whether cache has enough to store
	// if space is not enough, start a new epoch
	bool isCacheAvailable(int fd, int size){
		bool ret = false;
		socketCachedDataInfo* sock;
		if(_socketMap.find(fd, sizeof(int), &sock)) {
			PRINF("sock %d type %d cache remaining %d size %d\n", fd, sock->type, sock->remaining, size);

			if(sock->type == FD_DUP_SOCKET){
				sock = (socketCachedDataInfo*)sock->realObj[sock->curRealObj];
			}

			if(sock->remaining>=size){
				ret = true;
			}

		}
		else{
			PRINF("can not find sock %d\n", fd);	
		}

		return ret;
	}

	//save select epoll event
	void saveEvents(int epfd, struct epoll_event *events, int ret){
		if(ret >= 0){
			int size = sizeof(epoll_event) * ret;
			void *p = saveData(epfd, events, size);
			if(p != NULL){
				_sysrecord.recordSocketOps(E_SYS_SOCKET_EVENT, epfd, p, ret);
			}
		}
	}

  void saveSockOpt(eRecordSyscall sc, int fd, int level, int optname, void* optval, socklen_t* optlen, int ret) {
		_sysrecord.recordSockOpt(sc, fd, level, optname, optval, optlen, ret); 
	}
	
	void saveDescAndStatus(eRecordSyscall sc, int fd, int cmd, long arg, int ret){
		_sysrecord.recordDescAndStatus(sc, fd, ret); 
	}

	// set socket alive or not
	bool findAndcloseSock(int fd){
		bool ret = false;
		socketCachedDataInfo* sock;
		if(_socketMap.find(fd, sizeof(int), &sock)) {

			if(sock->type == FD_DUP_SOCKET){
				sock = (socketCachedDataInfo*)sock->realObj[sock->curRealObj];
			}

			sock->aliveCounter--;
			ret = true;
		}
		return ret;
	}

	// clean socket info and reset cache info in epochBegin
	void cleanSockets() {

    bool is_disabled = current->disablecheck;
    if(!is_disabled) {
      current->internalheap = true;
      current->disablecheck = true;
    }

		std::list<int> deadsock;
		// check all dead connect and save them to list.
		socketHashMap::iterator i;
		for(i = _socketMap.begin(); i != _socketMap.end(); i++) {
			socketCachedDataInfo* sock;
			sock = (socketCachedDataInfo*)i.getData();

			if(sock->type == FD_DUP_SOCKET){
				socketCachedDataInfo* dupsock = (socketCachedDataInfo*)sock->realObj[sock->curRealObj];
				if(dupsock->aliveCounter <= 0){
					//save dead sockets
					deadsock.push_back(sock->fd);
				}
			}
			else{
				if(sock->aliveCounter <= 0){
					//save dead sockets
					deadsock.push_back(sock->fd);
					//PRINT("add %d to dead socket list\n", sock->fd);
				}
				else{
					//reset data
					//PRINT("reset %d cache data\n", sock->fd);
					sock->remaining = sock->totalsize;
					sock->current = sock->head;
					sock->realObj[0] = sock->realObj[sock->curRealObj];
					sock->curRealObj = 0;
					sock->bakType = sock->type;
					sock->bakAliveCounter = sock->aliveCounter;
				}
			}
		}

		//free all related data
		for (std::list<int>::iterator it=deadsock.begin(); it != deadsock.end(); ++it){
			//PRINT("remove %d from socket mapping\n", *it);
			socketCachedDataInfo* sock;
			if(_socketMap.find(*it, sizeof(int), &sock)) {
				// decrement counter
				fdcounter--; 
				//delete fd from map
				_socketMap.erase(*it, sizeof(int)); 
				//free cached block
				if(sock->type != FD_DUP_SOCKET){
					InternalHeap::getInstance().free(sock->head);
					Real::close(sock->fd);
				}
				//free data info
				InternalHeap::getInstance().free(sock);
			}
		}

    cache_fd = 0;
    cache_socket_data = NULL;

    if(is_disabled != current->disablecheck) {
      current->internalheap = is_disabled;
      current->disablecheck = is_disabled;
    }
	}

	void prepareRollback() {
		socketHashMap::iterator i;
		for(i = _socketMap.begin(); i != _socketMap.end(); i++) {
			socketCachedDataInfo* sock;
			sock = (socketCachedDataInfo*)i.getData();

			// rollback
			sock->type = sock->bakType;
			sock->aliveCounter = sock->bakAliveCounter;
			sock->curRealObj = 0;
		}
	}

	void updateFdType(int fd, fdType type){	
		socketCachedDataInfo* sock = getSocketInfo(fd);
		assert(sock != NULL);
		sock->type = type;
	}

	int getPipeFd(int pipefd[2]){
		int ret = -1;
		_sysrecord.getPairOps(E_SYS_SOCKET_PIPE, pipefd, &ret);
		for(int i=0; i<2; i++){
			updateFdType(pipefd[i], FD_PIPE);
		}
		return ret;
	}

	int getPairFd(int sv[2]){
		int ret = -1;
		_sysrecord.getPairOps(E_SYS_SOCKET_PAIR, sv, &ret);
		for(int i=0; i<2; i++){
			updateFdType(sv[i], FD_SOCKETPAIR);
		}
		return ret;
	}

	// Called in the rollback phase
	// Try to get socket fd for a new connection.
	int getSocketFd() {
		int fd = -1;
		_sysrecord.getSocketOps(E_SYS_SOCKET_NEW, &fd);
    if(fd != -1) {
		  updateFdType(fd, FD_SOCKET);
    }
		return fd;
	}

	int getSocketFdAll(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
		int fd = -1;
		_sysrecord.getSocketOps(E_SYS_SOCKET_NEW, &fd, addr, addrlen);
		updateFdType(fd, FD_SOCKET);
		return fd;
	}

	int getSockConn(int sockfd) {
		int fd = -1;
		int ret = -1;
		_sysrecord.getSocketOps(E_SYS_SOCKET_CONN, &fd, &ret);
		assert(fd==sockfd);
		return ret;
	}

	int getSockName(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
		return getName(E_SYS_SOCKET_NAME, sockfd, addr, addrlen);
	}

	int getPeerName(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
		return getName(E_SYS_SOCKET_PEERNAME, sockfd, addr, addrlen);
	}

	//return size
	int getSockRecvData(int fd, void* data) {
		void *p;
		int size = -1;
		_sysrecord.getSocketOps(E_SYS_SOCKET_RECV, fd, &p, &size);
		if(size != -1){
			memcpy(data, p, size);
		}
	
		// update dup 
		socketCachedDataInfo* sock = getSocketInfo(fd);
		if(sock != NULL && sock->type==FD_DUP_SOCKET && sock->curRealObj==0){
			sock->isOldDup = true;
		}
		
		return size;
	}

  //return size
  int getSockRecvFromData(int fd, void* data, socklen_t* fromlen) {
    void *p;
    int size = -1;
    _sysrecord.getSocketOps(E_SYS_SOCKET_RECVFROM, fd, &p, NULL, fromlen, &size);
    if(size != -1){
      memcpy(data, p, size);
    }

    socketCachedDataInfo* sock = getSocketInfo(fd);
    if(sock != NULL && sock->type==FD_DUP_SOCKET && sock->curRealObj==0){
      sock->isOldDup = true;
    }

    return size;
  }

	//return ret 
	int getSockSendmsg(int sockfd) {
		int fd = -1;
		int ret = -1;
		_sysrecord.getSocketOps(E_SYS_SOCKET_SENDMSG, &fd, &ret);
		assert(fd==sockfd);
		return ret;
	}

	//return size
	int getEpollEvent(int epfd, void* data) {
		void *p;
		int ret = 0;
		_sysrecord.getSocketOps(E_SYS_SOCKET_EVENT, epfd, &p, &ret);
		int size = sizeof(epoll_event) * ret;
		memcpy(data, p, size);
		return ret;
	}

	int getSockOpt(eRecordSyscall sc, int fd, int level, int optname, void* optval, socklen_t* optlen) {
		return _sysrecord.getSockOpt(sc, fd, level, optname, optval, optlen); 
	}

	int getDescAndStatus(eRecordSyscall sc, int fd, int cmd, long arg){
		return _sysrecord.getDescAndStatus(sc, fd);
	}

	void saveSockDupFd(int oldfd, int newfd){
		// Save fd info
		_sysrecord.recordSocketOps(E_SYS_DUP_SOCKET, newfd);

		if(newfd != -1) {
			socketCachedDataInfo* sock = saveSocketInfo(newfd, FD_DUP_SOCKET);

			socketCachedDataInfo* oldsock;
			if(_socketMap.find(oldfd, sizeof(int), &oldsock)) {
				if((sock->isOldDup && sock->curRealObj==0)
							|| sock->curRealObj>0 ){
					sock->curRealObj++;
				}
				if(sock->curRealObj >= DUP_NUMBER_PER_FD) { PRINT("can not sve dup on fd %d", oldfd); abort(); }
				
				sock->realObj[sock->curRealObj] = oldsock;
			}

			PRINF("save dup socket %d, old fd %d\n", newfd, oldfd);
		}
	}

	int getSockDupFd(int oldfd){
		int fd = -1;
		_sysrecord.getSocketOps(E_SYS_DUP_SOCKET, &fd);
		updateFdType(fd, FD_DUP_SOCKET);
		return fd;
	}

	int getFdCounter() { return fdcounter; }

	private:

	socketCachedDataInfo* createNewSock(int fd, fdType type){
		socketCachedDataInfo* sock = (socketCachedDataInfo*)InternalHeap::getInstance().malloc(sizeof(socketCachedDataInfo));
		sock->fd = fd;

		// Allocate a block of memory to cache data
		void* ptr = InternalHeap::getInstance().malloc(xdefines::SOCKET_CACHE_SIZE);
		sock->totalsize = xdefines::SOCKET_CACHE_SIZE;
		sock->remaining = sock->totalsize;
		sock->type = type;
		sock->head = (char*)ptr;
		sock->current = sock->head;
		sock->isOldDup = false;

		sock->curRealObj = 0;
		sock->aliveCounter = 1;
		sock->bakType = type;
		sock->bakAliveCounter = 1;

		PRINF("fd:%d save data\n", fd);
		__atomic_add_fetch(&fdcounter, 1, __ATOMIC_RELAXED);
		return sock;
	}

	//save data to cache
	void* saveData(int fd, void* data, int size){
		void *p = NULL;
		socketCachedDataInfo* sock;
    if(cache_fd == fd) {
      sock = cache_socket_data; 
    } else
		// find fd info in MAP
		if(_socketMap.find(fd, sizeof(int), &sock)) {
      cache_fd = fd;
      cache_socket_data = sock;
    }
    if(sock != NULL) {
			// check whether it is created by DUP
			if(sock->type == FD_DUP_SOCKET){
				sock->isOldDup = true;
				sock = (socketCachedDataInfo*)sock->realObj[sock->curRealObj];
			}
		
			// allocate a block
			sock->remaining -= size;
			p = sock->current;
			sock->current += size;

			// copy data to cache
			memcpy(p, data, size);

			PRINF("fd:%d, save data %p, size %d, %s\n", fd, p, size, data);
		}else{
			PRINT("fd:%d, can not save data\n", fd);
		}

		return p;
	}

	void saveName(eRecordSyscall sc, int sockfd, struct sockaddr *addr, socklen_t *addrlen, int ret){
		_sysrecord.recordSocketOps(sc, sockfd, addr, addrlen, ret); 
	}

	int getName(eRecordSyscall sc, int sockfd, struct sockaddr *addr, socklen_t *addrlen){
		int ret = 0;
		_sysrecord.getSocketOps(sc, sockfd, addr, addrlen, &ret);
		return ret;
	}

	int fdcounter;
	SysRecord _sysrecord;
	typedef HashMap<int, socketCachedDataInfo*, spinlock, InternalHeapAllocator> socketHashMap; 
	socketHashMap _socketMap;

};

#endif
