#include <unistd.h>
#include <linux/futex.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <linux/sched.h>

#include <climits>

#include "cppgenerics/IPC_Mutex.h"
#include "cppgenerics/Syscalls.h"
#include "cppgenerics/Exception.h"
#include "cppgenerics/Logger.h"

#define L_FUTEX_STATE_LOCKED 0
#define L_FUTEX_STATE_UNLOCKED 1
#define L_FUTEX_STATE_INITIAL 1

namespace IPC {

static inline int sys_futex(futex_id_t* uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, int val3)
{
	return syscall(__NR_futex, uaddr, futex_op, val,timeout, uaddr, val3);
}

/*
 * MutexBase
 */

Mutex::Mutex(futex_id_t* ptr, bool isNew) :
		m_futexId(ptr)
{
	if (isNew) {
		*m_futexId = L_FUTEX_STATE_INITIAL ;
	}
}

void Mutex::lock() throw (std::exception&)
{
	/* __sync_bool_compare_and_swap(ptr, oldval, newval) is a gcc
	       built-in function.  It atomically performs the equivalent of:

	           if (*ptr == oldval)
	 	 	 	 *ptr = newval;

	       It returns true if the test yielded true and *ptr was updated.
	       The alternative here would be to employ the equivalent atomic
	       machine-language instructions.  For further information, see
	       the GCC Manual. */

	while ( 1 ) {
		/* Is the futex available? */
		if ( __sync_bool_compare_and_swap(m_futexId, L_FUTEX_STATE_UNLOCKED, L_FUTEX_STATE_LOCKED)) {
			//Yes
			break; //locked
		}
		/* Futex is not available; wait */
		else {
			if ( sys_futex(m_futexId, FUTEX_WAIT, L_FUTEX_STATE_LOCKED, NULL, NULL, -1) < 0 && errno != EAGAIN) {
				THROW_SYS_EXCEPTION("futex-FUTEX_WAIT");
			}
		}
	}
}


void Mutex::unlock() noexcept
{
	if ( __sync_bool_compare_and_swap(m_futexId, L_FUTEX_STATE_LOCKED, L_FUTEX_STATE_UNLOCKED)) {
		/*
		 * wakeup all processes
		 */
		sys_futex(m_futexId, FUTEX_WAKE, 1, NULL, NULL, -1);
	}
	else {
		THROW_SYS_EXCEPTION(" IPC::Mutex CRIT in unlock() ");
	}
}


} //namespace
