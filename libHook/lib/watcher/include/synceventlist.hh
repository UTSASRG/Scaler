#if !defined(DOUBLETAKE_SYNCEVENTLIST_H)
#define DOUBLETAKE_SYNCEVENTLIST_H

/*
 * @file   synceventlist.h
 * @brief  Manage the list of synchronization event.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <pthread.h>
#include <stddef.h>
#include "list.hh"
#include "log.hh"
#include "real.hh"
#include "recordentries.hh"
#include "threadstruct.hh"
#include "xdefines.hh"

#ifdef SINGLELL
class SyncEventList {
    public:
        SyncEventList(void* var, thrSyncCmd cmd, int lockType_) 
        {
            mpHeadEvent = NULL;
            mpTailEvent = NULL;
            mpCurrentEvent = NULL;
            Real::pthread_mutex_init(&mLock, NULL);
            mSyncVariable = NULL;
            mSyncCmd = cmd;
            mSyncVariable = var;
            
            holdingThread = NULL;
            currentLockCounter = 0;
            lockType = lockType_;
        }

        inline bool checkRecursiveLock(SyncEvent* threadEvent) {
          return lockType==PTHREAD_MUTEX_RECURSIVE
            && currentLockCounter
            && holdingThread == threadEvent->thread;
        }

        inline bool checkReadWriteLock(SyncEvent* threadEvent, thrSyncCmd cmd) {
          bool ret = false;
          if(cmd == E_SYNC_RWLOCK_UNLOCK) {
            --currentLockCounter;
            assert(currentLockCounter>=0);
            if(currentLockCounter == 0){
              holdingThread = NULL;
              ret = true;
            }
          } else if(cmd == E_SYNC_RWLOCK_RDLOCK) {
            if(mpCurrentEvent->synccmd == E_SYNC_RWLOCK_RDLOCK 
                && mpCurrentEvent->next == threadEvent) {
              holdingThread = threadEvent->thread;
              ret = true;
            } 
          }
          return ret;
        }

        inline void updateOwner(thrSyncCmd cmd, thread_t* currentThread) {
          //PRINT("%d, thread %p update lock %p, old thread %p, currentLockCounter %d", cmd, currentThread, mSyncVariable, holdingThread, currentLockCounter);
          if(cmd == E_SYNC_MUTEX_LOCK) {
            if(holdingThread != currentThread) {
              assert(currentLockCounter==0);
              holdingThread = currentThread;
            }
            ++currentLockCounter;
          } else if(cmd == E_SYNC_MUTEX_UNLOCK) {
            --currentLockCounter;
            assert(currentLockCounter>=0);
            if(currentLockCounter == 0){
              holdingThread = NULL;
            }
          } else if(cmd == E_SYNC_RWLOCK_RDLOCK 
              || cmd == E_SYNC_RWLOCK_WRLOCK) { 
            if(holdingThread != currentThread) {
              holdingThread = currentThread;
            }
            ++currentLockCounter;
          }
        }

        inline void resetLockInfo(int lockType_) {
          holdingThread = NULL;
          currentLockCounter = 0;
          lockType = lockType_;
        }

        inline void cleanup()
        {
            mpHeadEvent = NULL;
            mpTailEvent = NULL;

            // backup data
            bak_holdingThread = holdingThread;
            bak_currentLockCounter = currentLockCounter;
            bak_lockType = lockType;
        }

        inline syncEvent* recordSyncEvent(void *var, thrSyncCmd cmd, int ret) 
        {
            SyncEvent *pEvent = allocSyncEvent();
            pEvent->next = NULL;
            pEvent->thread = current;
            pEvent->eventlist = this;
            pEvent->ret = ret;
            pEvent->synccmd = cmd;
            pEvent->syncvar = var; 
            PRINF("Thread %d: recordSyncEvent event %p (type %d at %ld) on eventlist %p\n"
                    , current->index, pEvent, cmd, current->syncevents.getEntriesNumb()-1, this);

            PRINF("recordSyncEvent line %d: event %p (at %ld) thread %d eventlist %p\n"
                    , __LINE__, pEvent, current->syncevents.getEntriesNumb()-1, current->index, this);

            if(cmd != E_SYNC_MUTEX_LOCK) 
            {
                //if((cmd == E_SYNC_MUTEX_TRY_LOCK || cmd == E_SYNC_COND_WAIT) && ret!=0)
                if((cmd == E_SYNC_MUTEX_TRY_LOCK) && ret!=0)
                {
                    PRINF("a failed event, return value is %d\n", ret);	
                }
                else 
                {
                    Real::pthread_mutex_lock(&this->mLock);
                    appendEvent(pEvent);
                    Real::pthread_mutex_unlock(&this->mLock);
                }
            } 
            else 
            {
                appendEvent(pEvent);
            }
            return pEvent;
        }

        inline void* getSyncVariable() 
        { 
            return mSyncVariable; 
        }

        inline thrSyncCmd getSyncCmd() 
        { 
            return mSyncCmd; 
        }

        inline SyncEvent *advanceSyncEvent() 
        {
            PRINF("Thread %d curentry is %p this->list address %p\n"
                    , current->index, mpCurrentEvent, mpHeadEvent);

            if(mpCurrentEvent == NULL) 
            {
                PRINT("Assertion: curentry is NULL at advanceSyncEvent!!!\n");
                return mpCurrentEvent;
                abort();
            }

            if(mpCurrentEvent->next != NULL)
            {
                mpCurrentEvent = mpCurrentEvent->next;
            }
            else
            {
                mpCurrentEvent = NULL;
            }
            return mpCurrentEvent;
        }

        inline void lock_eventlist() { Real::pthread_mutex_lock(&this->mLock); }
        inline void unlock_eventlist() { Real::pthread_mutex_unlock(&this->mLock); }
        inline void wait_eventlist(pthread_cond_t* cond) { Real::pthread_cond_wait(cond, &this->mLock); }
        inline void signal_eventlist(pthread_cond_t* cond) { Real::pthread_cond_signal(cond); }

        inline struct syncEvent* getCurrentSyncEvent() {
          return (struct syncEvent*)mpCurrentEvent;
        }

        inline struct syncEvent* getNextSyncEvent() {
          return (struct syncEvent*)mpCurrentEvent->next;
        }

        inline SyncEvent *allocSyncEvent() 
        { 
            return current->syncevents.alloc(); 
        }

        inline SyncEvent *prepareRollback() 
        {
            holdingThread = bak_holdingThread;
            currentLockCounter = bak_currentLockCounter;
            lockType = bak_lockType;

            mpCurrentEvent = mpHeadEvent;
            PRINF("prepareRollback: synceventlist at %p event %p\n", mpHeadEvent, mpCurrentEvent);
            return mpCurrentEvent;
        }

        inline void appendEvent(SyncEvent *pEvent)
        {
            if(mpHeadEvent == NULL && mpTailEvent == NULL)
            {
                mpHeadEvent = pEvent;
                mpTailEvent = pEvent;
                mpCurrentEvent = pEvent;
            }
            else
            {
                pEvent->next = NULL;
                mpTailEvent->next = pEvent;
                mpTailEvent = pEvent;
            }
        }

    private:
        SyncEvent *mpHeadEvent;
        SyncEvent *mpTailEvent;
        SyncEvent *mpCurrentEvent;
        pthread_mutex_t mLock;
        void* mSyncVariable;
        thrSyncCmd mSyncCmd;

        void* holdingThread;
        int currentLockCounter;
        int lockType;

        void* bak_holdingThread;
        int bak_currentLockCounter;
        int bak_lockType;
};

#else

class SyncEventList {

    public:
        SyncEventList(void* variable, thrSyncCmd synccmd) {
            //    PRINF("synceventlist initialization at list %p\n", &list);
            // Initialize the sequence number
            listInit(&list);
            curentry = NULL;
            Real::pthread_mutex_init(&lock, NULL);
            syncVariable = variable;
            syncCmd = synccmd;
        }

        void initialization(thrSyncCmd synccmd) {
            listInit(&list);
            curentry = NULL;
            Real::pthread_mutex_init(&lock, NULL);
            syncVariable = NULL;
            syncCmd = synccmd;
        }

        // Record a synchronization event
        syncEvent* recordSyncEvent(void * variable, thrSyncCmd synccmd, int ret) {
            struct syncEvent* event = allocSyncEvent();
            listInit(&event->list);
            // Change the event there.
            event->thread = current;
            event->eventlist = this;
            event->ret = ret;
            event->synccmd = synccmd;
            event->syncvar = variable; 
            PRINF("Thread %d: recordSyncEvent event %p (type %d at %ld) on eventlist %p\n", current->index, event, synccmd, current->syncevents.getEntriesNumb()-1, this);

            PRINF("recordSyncEvent line %d: event %p (at %ld) thread %d eventlist %p\n", __LINE__, event, current->syncevents.getEntriesNumb()-1, current->index, this);
            if(synccmd != E_SYNC_MUTEX_LOCK) {
                //if((synccmd == E_SYNC_MUTEX_TRY_LOCK || synccmd == E_SYNC_COND_WAIT) && ret!=0)
                if((synccmd == E_SYNC_MUTEX_TRY_LOCK) && ret!=0)
                    PRINF("a failed event, return value is %d\n", ret);	
                }
                else {
                    Real::pthread_mutex_lock(&this->lock);
                    listInsertTail(&event->list, &this->list);
                    Real::pthread_mutex_unlock(&this->lock);
                }
            } else {
                // We only record synchronization inside critical section.
                // so there is no need to acquire another lock.
                listInsertTail(&event->list, &this->list);
            }
            // PRINF("RECORDING: syncCmd %d on event %p thread %p (THREAD%d)", synccmd, event,
            // event->thread, current->index);
            return event;
        }

        inline void* getSyncVariable() { return syncVariable; }

        inline thrSyncCmd getSyncCmd() { return syncCmd; }

        // Move forward for current thread event list
        inline struct syncEvent* advanceSyncEvent() {
            list_t* curentry = this->curentry;
            PRINF("Thread %d currenty is %p this->list address %p\n", current->index, curentry, &this->list);

            // If curentry is NULL, now print 
            if(curentry == NULL) {
                //FIXME
                PRINT("Assertion: curentry is NULL at advanceSyncEvent!!!\n");
                return (struct syncEvent*)this->curentry;
                abort();
            }
            if(!isListTail(curentry, &this->list)) {
                this->curentry = nextEntry(curentry);
            } else {
                this->curentry = NULL;
            }
            return (struct syncEvent*)this->curentry;
        }

        inline struct syncEvent* getCurrentSyncEvent() {
          return (struct syncEvent*)this->curentry;
        }

        inline struct syncEvent* getNextSyncEvent() {
          if(!isListTail(this->curentry, &this->list)) {
            return (struct syncEvent*)nextEntry(this->curentry);
          }else{
            return NULL;
          }
        }

        inline void lock_eventlist() { Real::pthread_mutex_lock(&this->lock); }
        inline void unlock_eventlist() { Real::pthread_mutex_unlock(&this->lock); }
        inline void wait_eventlist(pthread_cond_t* cond) { Real::pthread_cond_wait(cond, &this->lock); }
        inline void signal_eventlist(pthread_cond_t* cond) { Real::pthread_cond_signal(cond); }

        // cleanup all events in a list.
        void cleanup() {
            listInit(&this->list); 
        }

        struct syncEvent *allocSyncEvent() { return current->syncevents.alloc(); }

        // Set the first entry and return it
        struct syncEvent *prepareRollback() {
            struct syncEvent* event = NULL;
            if(!isListEmpty(&list)) {
                this->curentry = nextEntry(&list);
                event = (struct syncEvent*)this->curentry;
            }
            PRINF("prepareRollback: synceventlist at %p event %p\n", &list, event);
            return event;
        }

        list_t list; // List for all synchronization events.
    private:
        list_t* curentry;
        pthread_mutex_t lock;
        void* syncVariable;
        thrSyncCmd syncCmd;
};
#endif
#endif
