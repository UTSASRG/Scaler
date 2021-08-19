/*
 * @file   internal.cpp
 * @brief  Managing internal synchronizations used by StopGap itself.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#include "internalsyncs.hh"

#include "real.hh"
#include "threadstruct.hh"

void lock_thread(thread_t* thread) { Real::pthread_mutex_lock(&thread->mutex); }
void spinlock_thread(thread_t* thread) { 
	if(current->index!=thread->holdingSpinLockThread
				|| thread->spinlockcounter==0){
		pthread_spin_lock(&thread->spin);
		thread->holdingSpinLockThread=current->index;
	}
	thread->spinlockcounter++;

} //Hongyu

void unlock_thread(thread_t* thread) { Real::pthread_mutex_unlock(&thread->mutex); }
void spinunlock_thread(thread_t* thread) {
	thread->spinlockcounter--;
	pthread_spin_unlock(&thread->spin); 
} //Hongyu

void wait_thread_epochbegin(thread_t* thread) {
	current->condwait = &thread->condepochbegin;
	// Wait on the thread's internal cond that is for epoch begin only.	
	Real::pthread_cond_wait(&thread->condepochbegin, &thread->mutex);
}

void wait_thread(thread_t* thread) {
	current->condwait = &thread->cond;

	// Wait on the thread's internal cond.	
	Real::pthread_cond_wait(&thread->cond, &thread->mutex);
}

void wait_thread_joiner(thread * thread) {
	current->condwait = &thread->condjoiner;

	// Wait on the thread's internal cond.	
	Real::pthread_cond_wait(&thread->condjoiner, &thread->mutex);

}

void wait_thread() {
	current->condwait = &current->cond;
	//PRINT("Thread %d status %d is waiting, condwait %p\n", current->index, current->status, current->condwait);
	Real::pthread_cond_wait(current->condwait, &current->mutex); 
}

void signal_thread_epochbegin(thread_t* thread) { 
	//PRINT("Thread %d status %d condwait %p\n", thread->index, thread->status, thread->condwait);
	Real::pthread_cond_signal(&thread->condepochbegin); 
}

void signal_thread(thread_t* thread) { 
	//PRINT("Thread %d status %d condwait %p\n", thread->index, thread->status, thread->condwait);
	Real::pthread_cond_signal(&thread->cond); 
}

void signal_thread_joiner(thread_t* thread) { 
	//PRINT("Thread %d status %d condwait %p\n", thread->index, thread->status, thread->condwait);
	Real::pthread_cond_signal(&thread->condjoiner); 
}

void signal_waiting_thread(thread_t * thread) {
	//PRINT("Signal thread %d status %d condwait %p\n", thread->index, thread->status, thread->condwait);
	Real::pthread_cond_signal(thread->condwait); 
} 
