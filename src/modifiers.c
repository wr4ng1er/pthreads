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
#ifndef HAVE_PTHREADS_MODIFIERS
#define HAVE_PTHREADS_MODIFIERS

#ifndef HAVE_PTHREADS_MODIFIERS_H
#	include <src/modifiers.h>
#endif

#ifndef HAVE_PTHREADS_LOCK_H
#	include <src/lock.h>
#endif

#ifndef HAVE_PTHREADS_OBJECT_H
#	include <src/object.h>
#endif

/* {{{ statics */
static void pthreads_modifiers_modifiers_dtor(void **element);
static void pthreads_modifiers_protection_dtor(void **element); /* }}} */

/* {{{ allocate modifiers */
pthreads_modifiers pthreads_modifiers_alloc(TSRMLS_D) {
	pthreads_modifiers modifiers = calloc(1, sizeof(*modifiers));
	
	if (modifiers) {
		/*
		* Initialize modifiers
		*/
		zend_hash_init(&modifiers->modified, 32, NULL, (dtor_func_t) pthreads_modifiers_modifiers_dtor, 1);
		zend_hash_init(&modifiers->protection, 32, NULL, (dtor_func_t) pthreads_modifiers_protection_dtor, 1);
	}
	
	return modifiers;
} /* }}} */

/* {{{ initialize pthreads modifiers using the referenced class entry */
void pthreads_modifiers_init(pthreads_modifiers modifiers, zend_class_entry *entry TSRMLS_DC) {
	HashPosition position;
	zend_function *method;

	for (zend_hash_internal_pointer_reset_ex(&entry->function_table, &position);
		 zend_hash_get_current_data_ex(&entry->function_table, (void**)&method, &position) == SUCCESS;
		 zend_hash_move_forward_ex(&entry->function_table, &position)) {
		if (method && (method->type != ZEND_INTERNAL_FUNCTION)) {
			if (method->common.fn_flags & ZEND_ACC_PRIVATE){
				pthreads_modifiers_set(
					modifiers,
					method->common.function_name,
					ZEND_ACC_PRIVATE TSRMLS_CC
				);
			}
			
			if (method->common.fn_flags & ZEND_ACC_PROTECTED) {
				pthreads_modifiers_set(
					modifiers,
					method->common.function_name,
					ZEND_ACC_PROTECTED TSRMLS_CC
				);
			}
		}
	}
} /* }}} */

/* {{{ set access modifier for method */
int pthreads_modifiers_set(pthreads_modifiers modifiers, const char *method, zend_uint modify TSRMLS_DC) {
	size_t mlength = strlen(method);
	if (zend_hash_add(
		&modifiers->modified, 
		method, mlength, 
		(void**) &modify, sizeof(zend_uint), 
		NULL
	)==SUCCESS) {
		pthreads_lock lock = pthreads_lock_alloc(TSRMLS_C);
		if (lock) {
			return zend_hash_add(
				&modifiers->protection, 
				method, mlength, 
				(void**) &lock, sizeof(*lock), 
				NULL
			);
		}
	}
	return FAILURE;
} /* }}} */

/* {{{ get access modifier for method */
zend_uint pthreads_modifiers_get(pthreads_modifiers modifiers, const char *method TSRMLS_DC) {
	zend_uint *modified;
	size_t mlength = strlen(method);
	if (zend_hash_find(
			&modifiers->modified,
			method, mlength, 
			(void*) &modified
		)==SUCCESS) {
		return *modified;
	}
	return 0;
} /* }}} */

/* {{{ protect a method call */
zend_bool pthreads_modifiers_protect(pthreads_modifiers modifiers, const char *method, zend_bool *unprotect TSRMLS_DC) {
	pthreads_lock *protection;
	size_t mlength = strlen(method);
	if (zend_hash_find(&modifiers->protection, method, mlength, (void**)&protection)==SUCCESS) {
		return pthreads_lock_acquire(*protection, unprotect TSRMLS_CC);
	} else return 0;
} /* }}} */

/* {{{ unprotect a method call */
zend_bool pthreads_modifiers_unprotect(pthreads_modifiers modifiers, const char *method, zend_bool unprotect TSRMLS_DC) {
	pthreads_lock *protection;
	size_t mlength = strlen(method);
	if (zend_hash_find(&modifiers->protection, method, mlength, (void**)&protection)==SUCCESS) {
		return pthreads_lock_release(*protection, unprotect TSRMLS_CC);
	} else return 0;
} /* }}} */

/* {{{ free modifiers */
void pthreads_modifiers_free(pthreads_modifiers modifiers TSRMLS_DC) {
	if (modifiers) {
	    zend_hash_destroy(&modifiers->modified);
	    zend_hash_destroy(&modifiers->protection);
	    free(modifiers);
	}
} /* }}} */

/* {{{ destructor callback for modifiers (definition) hash table */
static void pthreads_modifiers_modifiers_dtor(void **element) {
	
} /* }}} */

/* {{{ destructor callback for modifiers (protection) hash table */
static void pthreads_modifiers_protection_dtor(void **element) {
	TSRMLS_FETCH();
	
	pthreads_lock_free((pthreads_lock) (*element) TSRMLS_CC);
} /* }}} */

#endif

