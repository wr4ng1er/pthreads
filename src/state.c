/*
  +----------------------------------------------------------------------+
  | pthreads                                                             |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2012 - 2014                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Joe Watkins <joe.watkins@live.co.uk>                         |
  +----------------------------------------------------------------------+
 */
 
/*
* @TODO
*	1. make errors more meaningful, where are my exceptions already !!
*/
#ifndef HAVE_PTHREADS_STATE
#define HAVE_PTHREADS_STATE

#ifndef HAVE_PTHREADS_STATE_H
#	include <src/state.h>
#endif

#ifndef HAVE_PTHREADS_THREAD_H
#	include <src/thread.h>
#endif

/* {{{ allocate state object */
pthreads_state pthreads_state_alloc(int mask TSRMLS_DC) {
	pthreads_state state = calloc(1, sizeof(*state));
	if (state != NULL) {
		state->bits |= mask;
		if (!(state->lock = pthreads_lock_alloc(TSRMLS_C))) {
		    free(state);
		    return NULL;
		}
			
		if (!(state->synchro = pthreads_synchro_alloc(TSRMLS_C))) {
		    free(state);
		    return NULL;
		}

		return state;
	}
	return NULL;
} /* }}} */

/* {{{ free state object */
void pthreads_state_free(pthreads_state state TSRMLS_DC) {
	if (state) {
	    pthreads_lock_free(state->lock TSRMLS_CC);
	    pthreads_synchro_free(state->synchro TSRMLS_CC);
	    free(state);
	}
} /* }}} */

/* {{{ lock state */
zend_bool pthreads_state_lock(pthreads_state state, zend_bool *locked TSRMLS_DC) {
	return pthreads_lock_acquire(state->lock, locked TSRMLS_CC);
} /* }}} */

/* {{{ unlock state */
zend_bool pthreads_state_unlock(pthreads_state state, zend_bool locked TSRMLS_DC) {
	return pthreads_lock_release(state->lock, locked TSRMLS_CC);
} /* }}} */

/* {{{ check state (assumes appropriate locking in place) */
int pthreads_state_check(pthreads_state state, int mask TSRMLS_DC) {
	return (state->bits & mask)==mask;
} /* }}} */

/* {{{ set state (assumes appropriate locking in place */
int pthreads_state_set_locked(pthreads_state state, int mask TSRMLS_DC) {
	state->was |= mask;
	pthreads_synchro_notify(
		state->synchro TSRMLS_CC
	);
	return (state->bits |= mask);
} /* }}} */

/* {{{ unset state ( assumes appropriate locking in place */
int pthreads_state_unset_locked(pthreads_state state, int mask TSRMLS_DC) {
	return (state->bits &= mask);
} /* }}} */

/* {{{ set state on state object */
zend_bool pthreads_state_set(pthreads_state state, int mask TSRMLS_DC) {
	zend_bool locked, result = 1;
	
	if (state) {
		if (pthreads_lock_acquire(state->lock, &locked TSRMLS_CC)) {
			state->was |= mask;
			pthreads_synchro_notify(
				state->synchro TSRMLS_CC
			);
			state->bits |= mask;
			pthreads_lock_release(state->lock, locked TSRMLS_CC);
		} else result = 0;
	} else result = 0;
	return result;
} /* }}} */

/* {{{ wait until a state has been set */
zend_bool pthreads_state_wait(pthreads_state state, int mask TSRMLS_DC) {
	zend_bool locked = 0, wasset = 0, result = 1;
	if (state) {
		if (pthreads_lock_acquire(state->lock, &locked TSRMLS_CC)) {
			wasset = ((state->was & mask)==mask);
			if (locked)
				pthreads_lock_release(state->lock, locked TSRMLS_CC);
			if (!wasset) do {
				pthreads_synchro_lock(
				    state->synchro TSRMLS_CC);
				pthreads_synchro_wait(state->synchro TSRMLS_CC);
				pthreads_synchro_unlock(
				    state->synchro TSRMLS_CC);
			} while(!((state->was & mask)==mask));
		} else result = 0;
	} else result = 0;
	return result;
} /* }}} */

/* {{{ check for state on state object */
zend_bool pthreads_state_isset(pthreads_state state, int mask TSRMLS_DC) {
	zend_bool locked = 0, isset = 0;
	if (state) {
		if (pthreads_lock_acquire(state->lock, &locked TSRMLS_CC)) {
			isset = ((state->bits & mask)==mask);
			pthreads_lock_release(state->lock, locked TSRMLS_CC);
		}
	}
	return isset;
} /* }}} */

/* {{{ unset state on state object */
zend_bool pthreads_state_unset(pthreads_state state, int mask TSRMLS_DC) {
	zend_bool locked = 0, result = 1;
	if (state) {
		if (pthreads_lock_acquire(state->lock, &locked TSRMLS_CC)) {
			state->bits &= ~mask;
			pthreads_lock_release(state->lock, locked TSRMLS_CC);
		} else result = 0;
	} else result = 0;
	return result;
} /* }}} */

#endif
