#include "pthread_impl.h"

int pthread_rwlock_timedwrlock(pthread_rwlock_t *restrict rw, const struct timespec *restrict at)
{
#ifdef __EMSCRIPTEN__
	/// XXX Emscripten: The spec allows detecting when multiple write locks would deadlock, which we do here to avoid hangs.
	/// If attempting to lock the write lock that we already own, error out.
	if (rw->_rw_wr_owner == (int)pthread_self()) return EDEADLK;
#endif
	int r, t;
	while ((r=pthread_rwlock_trywrlock(rw))==EBUSY) {
		if (!(r=rw->_rw_lock)) continue;
		t = r | 0x80000000;
		a_inc(&rw->_rw_waiters);
		a_cas(&rw->_rw_lock, r, t);
		r = __timedwait(&rw->_rw_lock, t, CLOCK_REALTIME, at, 0, 0, 0);
		a_dec(&rw->_rw_waiters);
		if (r && r != EINTR) return r;
	}
#ifdef __EMSCRIPTEN__
	/// XXX Emscripten: The spec allows detecting when multiple write locks would deadlock, which we do here to avoid hangs.
	/// Mark this thread as the owner of this write lock.
	rw->_rw_wr_owner = (int)pthread_self();
#endif
	return r;
}
