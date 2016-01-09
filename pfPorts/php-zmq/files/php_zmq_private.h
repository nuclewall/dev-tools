/*
+-----------------------------------------------------------------------------------+
|  ZMQ extension for PHP                                                            |
|  Copyright (c) 2010, Mikko Koppanen <mkoppanen@php.net>                           |
|  All rights reserved.                                                             |
+-----------------------------------------------------------------------------------+
|  Redistribution and use in source and binary forms, with or without               |
|  modification, are permitted provided that the following conditions are met:      |
|     * Redistributions of source code must retain the above copyright              |
|       notice, this list of conditions and the following disclaimer.               |
|     * Redistributions in binary form must reproduce the above copyright           |
|       notice, this list of conditions and the following disclaimer in the         |
|       documentation and/or other materials provided with the distribution.        |
|     * Neither the name of the copyright holder nor the                            |
|       names of its contributors may be used to endorse or promote products        |
|       derived from this software without specific prior written permission.       |
+-----------------------------------------------------------------------------------+
|  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND  |
|  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED    |
|  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           |
|  DISCLAIMED. IN NO EVENT SHALL MIKKO KOPPANEN BE LIABLE FOR ANY                   |
|  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES       |
|  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;     |
|  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND      |
|  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       |
|  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS    |
|  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                     |
+-----------------------------------------------------------------------------------+
*/

#ifndef _PHP_ZMQ_PRIVATE_H_
# define _PHP_ZMQ_PRIVATE_H_

#include "ext/standard/info.h"
#include "Zend/zend_exceptions.h"
#include "main/php_ini.h"

#include <zmq.h>

#ifdef PHP_WIN32
# include "win32/php_stdint.h"
#else
# include <stdint.h>
#endif

/* {{{ typedef struct _php_zmq_pollitem 
*/
typedef struct _php_zmq_pollitem {
	int events;
	zval *entry;
	char key[35];
	int key_len;
	
	/* convenience pointer containing fd or socket */
	void *socket;
	int fd;
} php_zmq_pollitem;
/* }}} */

/* {{{ typedef struct _php_zmq_pollset 
*/
typedef struct _php_zmq_pollset {
	php_zmq_pollitem *php_items;
	int num_php_items;

	/* items and a count */
	zmq_pollitem_t *items;
	int num_items;
	
	/* How many allocated */
	int alloc_size;
	
	/* Errors in the last poll */
	zval *errors;
} php_zmq_pollset;
/* }}} */

/* {{{ typedef struct _php_zmq_context
*/
typedef struct _php_zmq_context {
	/* zmq context */
	void *z_ctx;
	
	/* Amount of io-threads */
	int io_threads;
	
	/* Is this a persistent context */
	zend_bool is_persistent;
} php_zmq_context;
/* }}} */

/* {{{ typedef struct _php_zmq_socket
*/
typedef struct _php_zmq_socket  {
	void *z_socket;
	php_zmq_context *ctx;

	HashTable connect;
	HashTable bind;
	
	int type;
	zend_bool is_persistent;
} php_zmq_socket;
/* }}} */

/* {{{ typedef struct _php_zmq_context_object 
*/
typedef struct _php_zmq_context_object  {
	zend_object zo;
	php_zmq_context *context;
} php_zmq_context_object;
/* }}} */

/* {{{ typedef struct _php_zmq_socket_object 
*/
typedef struct _php_zmq_socket_object  {
	zend_object zo;
	php_zmq_socket *socket;
	
	/* options for the context */
	char *persistent_id;
	
	/* zval of the context */
	zval *context_obj;
} php_zmq_socket_object;
/* }}} */

/* {{{ typedef struct _php_zmq_poll_object 
*/
typedef struct _php_zmq_poll_object  {
	zend_object zo;
	php_zmq_pollset set;
} php_zmq_poll_object;
/* }}} */

/* {{{ typedef struct _php_zmq_device_object 
*/
typedef struct _php_zmq_device_object  {
	zend_object zo;
} php_zmq_device_object;
/* }}} */

#ifdef ZTS
# define ZMQ_G(v) TSRMG(php_zmq_globals_id, zend_php_zmq_globals *, v)
#else
# define ZMQ_G(v) (php_zmq_globals.v)
#endif

#define PHP_ZMQ_CONTEXT_OBJECT (php_zmq_context_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

#define PHP_ZMQ_SOCKET_OBJECT (php_zmq_socket_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

#define PHP_ZMQ_POLL_OBJECT (php_zmq_poll_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

#define ZMQ_RETURN_THIS RETURN_ZVAL(getThis(), 1, 0);

#ifndef Z_ADDREF_P
# define Z_ADDREF_P(pz) (pz)->refcount++
#endif

#ifndef Z_DELREF_P
# define Z_DELREF_P(pz) (pz)->refcount--
#endif

#ifndef Z_REFCOUNT_P
# define Z_REFCOUNT_P(pz) (pz)->refcount
#endif

#if ZEND_MODULE_API_NO > 20060613

#define PHP_ZMQ_ERROR_HANDLING_INIT() zend_error_handling error_handling;

#define PHP_ZMQ_ERROR_HANDLING_THROW() zend_replace_error_handling(EH_THROW, php_zmq_socket_exception_sc_entry, &error_handling TSRMLS_CC);

#define PHP_ZMQ_ERROR_HANDLING_RESTORE() zend_restore_error_handling(&error_handling TSRMLS_CC);

#else

#define PHP_ZMQ_ERROR_HANDLING_INIT()

#define PHP_ZMQ_ERROR_HANDLING_THROW() php_set_error_handling(EH_THROW, php_zmq_socket_exception_sc_entry TSRMLS_CC);

#define PHP_ZMQ_ERROR_HANDLING_RESTORE() php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);

#endif


#endif /* _PHP_ZMQ_PRIVATE_H_ */
