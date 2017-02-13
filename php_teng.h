/*
  +----------------------------------------------------------------------+
  | PHP Version 4                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2002 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.02 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available at through the world-wide-web at                           |
  | http://www.php.net/license/2_02.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Ondrej Prochazka <ondra@firma.seznam.cz>                     |
  +----------------------------------------------------------------------+

  $Id: php_teng.h,v 1.1 2004-07-29 20:56:51 solamyl Exp $
*/

#ifndef _PHP_TENG_H
#define _PHP_TENG_H

extern zend_module_entry teng_module_entry;
#define phpext_teng_ptr &teng_module_entry

#ifdef PHP_WIN32
#define PHP_TENG_API __declspec(dllexport)
#else
#define PHP_TENG_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(teng);
PHP_MSHUTDOWN_FUNCTION(teng);
PHP_RINIT_FUNCTION(teng);
PHP_RSHUTDOWN_FUNCTION(teng);
PHP_MINFO_FUNCTION(teng);

ZEND_FUNCTION( teng_init );                /* creates teng resource. */
ZEND_FUNCTION( teng_release );             /* releases teng resource. */
ZEND_FUNCTION( teng_create_data_root );    /* initializes data tree. */
ZEND_FUNCTION( teng_release_data );        /* releases a data tree. */
ZEND_FUNCTION( teng_add_fragment );        /* adds fragment to data tree. */
ZEND_FUNCTION( teng_page_string );         /* genpage from template file. */
ZEND_FUNCTION( teng_page_string_from_string );
                                           /* genpage from string. */
ZEND_FUNCTION( teng_dict_lookup );         /* perform dict lookup */
ZEND_FUNCTION( teng_list_content_types );  /* list supported content types */


/*
  	Declare any global variables you may need between the BEGIN
	and END macros here:
*/

ZEND_BEGIN_MODULE_GLOBALS(teng)
    char *template_root;
    char *default_dict;
    char *default_lang;
    char *default_config;
    char *default_skin;
    char *default_content_type;
    char *default_encoding;
    long *log_to_output;
    long *error_fragment;
    long validation;
ZEND_END_MODULE_GLOBALS(teng)

/* In every utility function you add that needs to use variables
   in php_teng_globals, call TSRM_FETCH(); after declaring other
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as TENG_G(variable).  You are
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define TENG_G(v) TSRMG(teng_globals_id, zend_teng_globals *, v)
#else
#define TENG_G(v) (teng_globals.v)
#endif

#endif	/* _PHP_TENG_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
