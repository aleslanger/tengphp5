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

  $Id: teng.cpp,v 1.4 2005-11-25 15:14:18 vasek Exp $
*/

#ifdef PHP_WIN32
#include <iostream>
#include <math.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {
    #include "php.h"
    #include "php_ini.h"
    #include "ext/standard/info.h"
	#include "php_teng.h"
}

#include <vector>
#include <set>
#include <utility>
#include <string>
#include <teng.h>

using std::string;
using std::vector;
using std::set;
using std::pair;

using namespace Teng;

ZEND_DECLARE_MODULE_GLOBALS(teng)

/* True global resources - no need for thread safety here */
static int le_teng, le_fragment;

/* {{{ teng_functions[]
 *
 * Every user visible function must have an entry in teng_functions[].
 */
//function_entry teng_functions[] = {
zend_function_entry teng_functions[] = {
    ZEND_FE( teng_init, NULL )          /* Create teng resource. */
    ZEND_FE( teng_release, NULL )       /* Release teng resource. */
    ZEND_FE( teng_create_data_root, NULL )
                                        /* Create data root */
    ZEND_FE( teng_release_data, NULL )  /* Release data root */
    ZEND_FE( teng_add_fragment, NULL )  /* Add fragment to data tree. */
    ZEND_FE( teng_page_string, NULL )
                                        /* Genpage to string from file. */
    ZEND_FE( teng_page_string_from_string, NULL )
                                        /* Genpage to string from string. */
    ZEND_FE( teng_dict_lookup, NULL )   /* Perform dictionary lookup. */
    ZEND_FE( teng_list_content_types, NULL )
                                        /* List supported content-types. */
    {NULL, NULL, NULL}	/* Must be the last line in teng_functions[] */
};
/* }}} */

/* {{{ teng_module_entry
 */
zend_module_entry teng_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "teng",
    teng_functions,
    PHP_MINIT(teng),
    PHP_MSHUTDOWN(teng),
    PHP_RINIT(teng),
    PHP_RSHUTDOWN(teng),
	PHP_MINFO(teng),
#if ZEND_MODULE_API_NO >= 20010901
	"0.9", /* version number for teng extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_TENG
BEGIN_EXTERN_C()
ZEND_GET_MODULE(teng)
END_EXTERN_C()
#endif


/* Resource definition. */

/* Resource fragment structure. */
struct FragmentResource_t {

    /* actual fragment refered to in this resource. */
    Fragment_t * fragment;

    /* root fragment resource id, this for root fragment. */
    FragmentResource_t * rootFragmentRes;

    /* resource ids for descendant resources (non empty for root fragment) */
    set<long> descendants;

#ifdef ZTS
	TSRMLS_D;
#endif //ZTS

    /* create root fragment resource. */
    FragmentResource_t( Fragment_t * fragment )
        : fragment( fragment ), rootFragmentRes( this )
    {}

    /* create non-root fragment resource. */
    FragmentResource_t( Fragment_t * fragment,
        FragmentResource_t * rootFragmentRes )
        : fragment( fragment ), rootFragmentRes( rootFragmentRes )
    {}

    /* check if fragment resouce is root fragment */
    bool isRoot() { return rootFragmentRes == this; }

    /* destructor */
    ~FragmentResource_t() {

        if ( isRoot() ) {

            // root fragment, kill descendant resources
            for ( set<long>::const_iterator it = descendants.begin();
                it != descendants.end(); ++it ) {

                // delete resource from list
                zend_list_delete( *it );
            }

            // delete actual data tree
            delete fragment;
        }

    }

};

/* {{{ release_teng
 * Release a teng resource
 */
static void release_teng( zend_rsrc_list_entry *rsrc TSRMLS_DC )
{
    Teng_t * teng = (Teng_t *) rsrc->ptr;
    delete teng;
}
/* }}} */


/* {{{ release_fragment
 * Release a fragment resource
 */
static void release_fragment( zend_rsrc_list_entry *rsrc TSRMLS_DC )
{
    FragmentResource_t * fragmentRes = (FragmentResource_t *) rsrc->ptr;

#ifdef ZTS
	fragmentRes->TSRMLS_C = TSRMLS_C;
#endif //ZTS

    delete fragmentRes;
}
/* }}} */

/* Initialization file support. */

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY( "teng.template_root", "",
        PHP_INI_ALL, OnUpdateString, template_root, zend_teng_globals,
        teng_globals )
    STD_PHP_INI_ENTRY( "teng.default_dict", "",
        PHP_INI_ALL, OnUpdateString, default_dict, zend_teng_globals,
        teng_globals )
    STD_PHP_INI_ENTRY( "teng.default_lang", "",
        PHP_INI_ALL, OnUpdateString, default_lang, zend_teng_globals,
        teng_globals )
    STD_PHP_INI_ENTRY( "teng.default_config", "",
        PHP_INI_ALL, OnUpdateString, default_config, zend_teng_globals,
        teng_globals )
    STD_PHP_INI_ENTRY( "teng.default_encoding", "utf-8",
        PHP_INI_ALL, OnUpdateString, default_encoding,
        zend_teng_globals, teng_globals )
    STD_PHP_INI_ENTRY( "teng.default_content_type", "text/html",
        PHP_INI_ALL, OnUpdateString, default_content_type,
        zend_teng_globals, teng_globals )
    STD_PHP_INI_ENTRY( "teng.default_skin", "",
        PHP_INI_ALL, OnUpdateString, default_skin, zend_teng_globals,
        teng_globals )
PHP_INI_END()
/* }}} */

/* {{{ php_teng_init_globals
 */
static void php_teng_init_globals(zend_teng_globals *teng_globals)
{
    teng_globals->template_root = NULL;
    teng_globals->default_dict = NULL;
    teng_globals->default_lang = NULL;
    teng_globals->default_config = NULL;
    teng_globals->default_skin = NULL;
    teng_globals->default_content_type = NULL;
    teng_globals->default_encoding = NULL;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(teng)
{
    ZEND_INIT_MODULE_GLOBALS( teng, php_teng_init_globals, NULL );
	REGISTER_INI_ENTRIES();

    // register resource destructors
    le_teng = zend_register_list_destructors_ex(
        NULL, release_teng, "teng",  module_number );
    le_fragment = zend_register_list_destructors_ex(
        release_fragment, NULL, "teng-fragment",  module_number );

    // done
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(teng)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(teng)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(teng)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(teng)
{
    vector<pair<string,string> > contentTypes;

    Teng_t::listSupportedContentTypes( contentTypes );

    php_info_print_table_start();
    php_info_print_table_header( 2, "teng support", "enabled" );
    php_info_print_table_end();

    php_info_print_table_start();
    php_info_print_table_header( 2, "Template content type", "Description" );
    for ( vector< pair<string, string> >::const_iterator it
        = contentTypes.begin(); it != contentTypes.end(); ++it )
        php_info_print_table_row( 2, it->first.c_str(),
            it->second.c_str() );
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}
/* }}} */

/* Helper functions */

/** @short populate a Teng fragment from a PHP data array.
  * @param fragment fragment to be populated
  * @param dataArray zval container containing a data array. Only
  *     associative (string key) elements are relevant.
  * @param recursive set of zval container links prohibited in
  *     data array. Used in recursive invocations to detect
  *     cycles in the data array. When calling on the top level,
  *     initialize to an empty set.
  * @return SUCCESS on success, FAILURE otherwise.
  */
int populateFragmentFromPHPArray( Fragment_t & fragment,
    zval * dataArray, set<zval *> & recursive ) {

    // check for data tree recursion
    if ( recursive.find( dataArray ) != recursive.end() ) {

        zend_error( E_WARNING, "Recursion found in data tree, aborting." );
        return FAILURE;
    }

    // remember data array
    recursive.insert( dataArray );

    // populate from arrays only
    if ( dataArray->type != IS_ARRAY ) {

        zend_error( E_WARNING,
            "Cannot initialize fragment from value other than array." );
        return FAILURE;
    }

    // iterate through array
    HashPosition it;
    zval ** entry;

    zend_hash_internal_pointer_reset_ex( Z_ARRVAL_P( dataArray ), & it );

    while( zend_hash_get_current_data_ex( Z_ARRVAL_P( dataArray ),
        (void **) & entry, & it ) == SUCCESS ) {

        char * stringKey;
        unsigned long intKey;
        unsigned int slen;

        // obtain key
        if ( zend_hash_get_current_key_ex( Z_ARRVAL_P( dataArray ), & stringKey,
           & slen, & intKey, 0, & it ) != HASH_KEY_IS_STRING  ) {

           // non associative elements are ignored
           zend_error( E_NOTICE,
               "Non-associative element '%d' in fragment definition, "
               "skipping.", intKey );
           zend_hash_move_forward_ex( Z_ARRVAL_P( dataArray ), & it );
           continue;
        }

        // array indicates nested fragment
        if ( (*entry)->type == IS_ARRAY ) {

            zval ** nestedEntry;
            HashPosition it;
            FragmentList_t & nestedFragmentList
                = fragment.addFragmentList( stringKey );

            // iterate through nested list
            zend_hash_internal_pointer_reset_ex( Z_ARRVAL_PP( entry ), & it );

            while( zend_hash_get_current_data_ex( Z_ARRVAL_PP( entry ),
                (void **) & nestedEntry, & it ) == SUCCESS ) {

                Fragment_t & nestedFragment
                    = nestedFragmentList.addFragment();

                if ( populateFragmentFromPHPArray( nestedFragment,
                    * nestedEntry, recursive ) != SUCCESS )
                    return FAILURE;

                zend_hash_move_forward_ex( Z_ARRVAL_PP( entry ), & it );
            }
        }

        // non-arrays are treated as variables
        if ( (*entry)->type != IS_ARRAY ) {

            // force conversion to string
            convert_to_string_ex( entry );
            fragment.addVariable( stringKey, Z_STRVAL_PP( entry ) );

        }

        // next element
        zend_hash_move_forward_ex( Z_ARRVAL_P( dataArray ), & it );
    }

    // remove array from recursive link list
    recursive.erase( dataArray );

    // done
    return SUCCESS;
}

struct Options_t {
    string skin;
    string dict, lang;
    string config;
    string definition;
    string content_type, encoding;

    Options_t() {}

    void init(TSRMLS_D) {

        skin = TENG_G( default_skin )
            ? TENG_G( default_skin ) : string();
        dict = TENG_G( default_dict )
            ? TENG_G( default_dict ) : string();
        lang = TENG_G( default_lang )
            ? TENG_G( default_lang ) : string();
        config = TENG_G( default_config )
            ? TENG_G( default_config ) : string();
        content_type = TENG_G( default_content_type )
            ? TENG_G( default_content_type ) : string();
        encoding = TENG_G( default_encoding )
            ? TENG_G( default_encoding ) : string();
    }

    static void checkForOption( zval * options, char * key,
                                string & target ) {

        zval ** option;

        if ( zend_hash_find( Z_ARRVAL_P( options ), key,
                             strlen( key )  + 1, (void **) & option ) == SUCCESS ) {

            convert_to_string_ex( option );
            target = string( Z_STRVAL_PP( option ) );
        }
    }

};

/** @short generic function for page generation
 * This function takes care of the actual page generation. It is inteneded
  * to support all legal input (string or file) and output (string or file)
  * variants. Individual PHP API page generation functions merely parse their
  * arguments and translate them to an appropriate call to this function.
  * @param teng_resource teng resource zval container
  * @param template_path template path, NULL if a template
  *     string is used.
  * @param template_string template string, NULL if a template
  *     path is used.
  * @param data data tree zval container (array or fragment resource),
  *     NULL if no data tree is supplied.
  * @param options options array zval container. NULL if no options
  *     are supplied (global or teng engine defaults will be used).
  * @param output string holding the page generated.
  * @return SUCCESS on success, FAILURE otherwise.
  */
int generate_page( zval * teng_resource, const string* template_path,
    const string * template_string, zval * data, zval * options,
    string & output TSRMLS_DC)
{
    Teng_t * teng;
    Fragment_t * root_fragment;
    bool temp_root_fragment;

    Options_t current_options;
    current_options.init(TSRMLS_C);

    // fetch teng resource
    if ( ( teng = (Teng_t *) zend_fetch_resource( & teng_resource TSRMLS_CC,
        -1, "teng", NULL, 1, le_teng ) ) == NULL )
        return FAILURE;

    // determine parameters
    if ( options != NULL ) {

        Options_t::checkForOption( options, "skin", current_options.skin );
        Options_t::checkForOption( options, "dict", current_options.dict );
        Options_t::checkForOption( options, "lang", current_options.lang );
        Options_t::checkForOption( options, "config", current_options.config );
        Options_t::checkForOption( options, "definition", current_options.definition );
        Options_t::checkForOption( options, "content_type", current_options.content_type );
        Options_t::checkForOption( options, "encoding", current_options.encoding );

    }

    // obtain data root
    do {

        set<zval *> recursive;
        FragmentResource_t * fragmentRes;

        if ( data == NULL ) {

            // new data tree supplied, use empty fragment
            root_fragment = new Fragment_t();
            temp_root_fragment = true;

        } else switch( data->type ) {

            case IS_ARRAY:
                // data is array, use temp root fragment
                root_fragment = new Fragment_t();
                temp_root_fragment = true;

                // convert application data to fragment content
                if ( populateFragmentFromPHPArray( * root_fragment, data,
                    recursive ) != SUCCESS )
                    return FAILURE;
                break;

            case IS_RESOURCE:
                // data is resource, fetch resource
                if ( ( fragmentRes = (FragmentResource_t *)
                    zend_fetch_resource( & data TSRMLS_CC,
                    -1, "teng-fragment", NULL, 1, le_fragment ) ) == NULL )
                    return FAILURE;

                if ( ! fragmentRes->isRoot() ) {
                    zend_error( E_WARNING,
                        "Data argument is not a root resource" );
                    return FAILURE;
                }
                root_fragment = fragmentRes->fragment;
                temp_root_fragment = false;
                break;

            default :
                zend_error( E_WARNING,
                    "Data argument must be array or data root resource." );
                return FAILURE;
        }

    } while( 0 );

    // generate page
    do {

        Error_t errors;
        StringWriter_t writer( output );

        if ( template_path != NULL ) {

            // generate from template path
            if ( teng->generatePage( *template_path, current_options.skin,
                current_options.dict, current_options.lang, current_options.config,
                current_options.content_type, current_options.encoding,
                * root_fragment, writer, errors ) < 0 ) {

                if ( temp_root_fragment ) delete root_fragment;
                zend_error( E_WARNING, "Page generation failed." );
                return FAILURE;
            }

        } else if ( template_string != NULL ) {

            // generate from a template string
            if ( teng->generatePage( *template_string,
                current_options.dict, current_options.lang, current_options.config,
                current_options.content_type, current_options.encoding,
                * root_fragment, writer, errors ) < 0 ) {

                if ( temp_root_fragment ) delete root_fragment;
                zend_error( E_WARNING, "Page generation failed." );
                return FAILURE;
            }
        }

        // free temporary fragment
        if ( temp_root_fragment ) delete root_fragment;

        // report errors
        for ( vector<Error_t::Entry_t>::const_iterator it
            = errors.getEntries().begin();
            it != errors.getEntries().end(); ++it ) {

            if ( it->pos.filename != string() )
                zend_error( E_NOTICE,
                    "Teng error (file '%s', line %d, column %d) '%s'.",
                    it->pos.filename.c_str(), it->pos.lineno, it->pos.col,
                    it->message.c_str() );
            else
                zend_error( E_NOTICE, "Teng error '%s'.", it->message.c_str() );

        }

    } while( 0 );

    // done
    return SUCCESS;
}
/* }}} */

/* Zend functions */

/* {{{ proto resource teng_init( [ string template_root ] )
   Create a new teng resource. */
ZEND_FUNCTION( teng_init )
{
    char *template_root;
    Teng_t *teng;

    // parse parameters
    template_root = TENG_G( template_root );

    switch( ZEND_NUM_ARGS() ) {

        int slen;

        case 0:
            // use defaults
            break;

        case 1:
            if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC,
                "s", & template_root, & slen ) != SUCCESS )
                RETURN_FALSE;
            break;

        default:
            WRONG_PARAM_COUNT
            RETURN_FALSE;
    }

    // obtain teng resource
    {
        zend_rsrc_list_entry * le;
        char * hashKey;
        int hashKeyLength;

        // obtain hash key
        hashKeyLength = strlen( "teng_" ) + strlen( template_root );

        hashKey = (char *) emalloc( hashKeyLength +1 );
  
        
        sprintf( hashKey, "teng_%s", template_root );

        // find out if a sheng resource with given parameters already exists -
        // mysql-extension-like scheme is used to hash teng resources.
        if ( zend_hash_find( & EG( persistent_list ), hashKey,
            hashKeyLength + 1, (void **) & le) == FAILURE ) {

            // need to create a new object
            int err;
           // list_entry newLe;
            zend_rsrc_list_entry newLe;

            teng = new Teng_t( template_root, Teng_t::Settings_t() );
            
            

            // hash it up
            Z_TYPE( newLe ) = le_teng;
            newLe.ptr = teng;
            if ( zend_hash_update( &EG( persistent_list ), hashKey,
               // hashKeyLength + 1, (void *) & newLe, sizeof( list_entry ),
               hashKeyLength + 1, (void *) & newLe, sizeof( zend_rsrc_list_entry ),
                NULL ) == FAILURE ) {

                free( teng ); efree( hashKey ); RETURN_FALSE
            }


        } else {

            // sheng object already exists, reuse
            if ( Z_TYPE_P( le ) != le_teng ) RETURN_FALSE;
            teng = (Teng_t *) le->ptr;
        }

    }

    // register resource, done
    ZEND_REGISTER_RESOURCE( return_value, teng, le_teng );
}
/* }}} */

/* {{{ proto boolean teng_release( resource teng )
   Release a previously allocated teng_resource. */
ZEND_FUNCTION(teng_release)
{
    Teng_t * teng;
    zval * teng_resource;

    if ( ZEND_NUM_ARGS() != 1 )
        WRONG_PARAM_COUNT

    if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC,
        "r", & teng_resource  ) != SUCCESS )
        RETURN_FALSE;

    // check that resource can be fetched
    ZEND_FETCH_RESOURCE( teng, Teng_t *, & teng_resource, -1, "teng",
        le_teng );

    // delete resource from list
    zend_list_delete( Z_RESVAL_P( teng_resource ) );

    // done
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto resource teng_create_data_root([ array data ])
   Create a data tree root. */
ZEND_FUNCTION( teng_create_data_root )
{
    zval * data;
    Fragment_t * fragment;
    FragmentResource_t * fragmentRes;
    set<zval *> recursive;

    // parase parameters
    data = NULL;

    if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC,
        "|a", & data  ) != SUCCESS )
        RETURN_FALSE;

    // create (and optionally populate) new fragment
    fragment = new Fragment_t();
    if ( data != NULL ) {

        if ( populateFragmentFromPHPArray( * fragment, data, recursive )
            != SUCCESS )
            RETURN_FALSE;
    }

    // register and return new resource
    fragmentRes = new FragmentResource_t( fragment );
    ZEND_REGISTER_RESOURCE( return_value, fragmentRes, le_fragment );
}
/* }}} */


/* {{{ proto resource teng_release_data( resource fragment )
   Release a previously allocated data tree root. */
ZEND_FUNCTION( teng_release_data )
{
    zval * fragment_resource;
    FragmentResource_t * fragmentRes;

    if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC,
        "r", & fragment_resource  ) != SUCCESS )
        RETURN_FALSE;

    // check that resource can be fetched
    ZEND_FETCH_RESOURCE( fragmentRes, FragmentResource_t *,
        & fragment_resource, -1, "teng-fragment", le_fragment );

    if ( ! fragmentRes->isRoot() ) {

        zend_error( E_WARNING, "Not a data root resource." );
        RETURN_FALSE;
    }

    // delete resource from list
    zend_list_delete( Z_RESVAL_P( fragment_resource ) );

    // done
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto resource teng_add_fragment( resource parent, string name,
    array data )
    Add fragment to a data tree. */
ZEND_FUNCTION( teng_add_fragment )
{
    zval * parent_resource, * data, * root;
    char * name;
    int slen;
    FragmentResource_t * parentFragmentRes, * newFragmentRes;
    Fragment_t * newFragment;
    set<zval *> recursive;

    // parse parameters
    data = NULL;

    if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC,
        "rs|a", & parent_resource, & name, & slen, & data  ) != SUCCESS )
        RETURN_FALSE;

    // fetch parent resource
    ZEND_FETCH_RESOURCE( parentFragmentRes, FragmentResource_t *,
        & parent_resource, -1, "teng-fragment", le_fragment );

    // create (and optionally populate) new fragment
    newFragment = & parentFragmentRes->fragment->addFragment( name );
    if ( data != NULL ) {

        if ( populateFragmentFromPHPArray( * newFragment, data, recursive )
            != SUCCESS )
            RETURN_FALSE;
    }

    // register and return new resource
    newFragmentRes = new FragmentResource_t( newFragment,
        parentFragmentRes->rootFragmentRes );
    ZEND_REGISTER_RESOURCE( return_value, newFragmentRes, le_fragment );

    // insert new resource id into root fragments descendants
    parentFragmentRes->rootFragmentRes->descendants.insert(
        Z_RESVAL_P( return_value ) );

}
/* }}} */


/* {{{ proto string teng_page_string( resource teng, string template_path [,
     array/resource data [, array options ] ] )
   generate a string from file template. */
ZEND_FUNCTION( teng_page_string )
{
    Teng_t * teng;
    zval * teng_resource;
    char * template_path;
    int template_path_len;
    zval * data, * options;
    string output;

    // parse parameters
    do {


        data = NULL; options = NULL;

        if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC,
            "rs|za", & teng_resource, & template_path, & template_path_len,
            & data, & options ) != SUCCESS )
            RETURN_FALSE;

    } while ( 0 );

    // make C++ string from supplied data
    string templatePath(template_path, template_path_len);

    // generate page
    if ( generate_page( teng_resource, &templatePath, NULL, data,
		options, output TSRMLS_CC) != SUCCESS )
        RETURN_FALSE;

    // return output - the cast is nasty, but safe as long as the string
    // is duplicated.
    RETURN_STRINGL(const_cast<char*>(output.data()), output.length(), true);
}
/* }}} */


/* {{{ proto string teng_page_string_from_string( resource teng,
    string template_path [,array/resource data [, array options ] ] )
   generate a string from file template. */
ZEND_FUNCTION( teng_page_string_from_string )
{
    Teng_t * teng;
    zval * teng_resource;
    char * template_string;
    int template_string_len;
    zval * data, * options;
    string output;

    // parse parameters
    do {

        data = NULL; options = NULL;

        if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC,
            "rs|za", & teng_resource, & template_string,
            & template_string_len,
            & data, & options ) != SUCCESS )
            RETURN_FALSE;

    } while ( 0 );

    // make C++ string from supplied data
    string templateString(template_string, template_string_len);

    // generate page
    if ( generate_page( teng_resource, NULL, &templateString, data,
		options, output TSRMLS_CC) != SUCCESS )
        RETURN_FALSE;

    // return output - the cast is nasty, but safe as long as the string
    // is duplicated.
    RETURN_STRINGL(const_cast<char*>(output.data()), output.length(), true);
}
/* }}} */

/* {{{ proto string teng_dict_lookup( resource teng, key [,
    string dict [, string lang [, string config ] ] ] )
    Lookup a dynamic literal translation in a dictionary. */
ZEND_FUNCTION( teng_dict_lookup )
{
    zval * teng_resource;
    Teng_t * teng;
    char * key, * dict, * lang, *config;
    string result;

    // parse arguments
    do {

        int slen;

        dict = TENG_G( default_dict );
        lang = TENG_G( default_lang );
        config = TENG_G( default_config );

        if ( zend_parse_parameters( ZEND_NUM_ARGS() TSRMLS_CC,
            "rs|sss", & teng_resource, & key, & slen,
            & dict, & slen, & lang, & slen, &config, &slen ) != SUCCESS )
            RETURN_FALSE;

        if ( dict == NULL ) {
            zend_error( E_WARNING, "No dictionary to lookup from." );
            RETURN_FALSE;
        }

    } while( 0 );

    // fetch teng resource
    ZEND_FETCH_RESOURCE( teng, Teng_t *, & teng_resource, -1, "teng",
        le_teng );

    // perform lookup
    if ( teng->dictionaryLookup(
        config ? config : string(), dict, lang ? lang : string(),
        key, result ) != 0 ) {

        zend_error( E_NOTICE, "Dictionary lookup failed, dict='%s', "
            "lang='%s', key='%s'.", dict, lang, key );
        RETURN_FALSE;
    }

    // return output - the cast is nasty, but safe as long as the string
    // is duplicated.
    RETURN_STRINGL(const_cast<char*>(result.data()), result.length(), true);
}
/* }}} */

/* {{{ proto array teng_supported_content_types()
    Get list of content types known to teng. */
ZEND_FUNCTION( teng_list_content_types )
{
    vector<pair<string,string> > contentTypes;

    // obtain supported content types
    Teng_t::listSupportedContentTypes( contentTypes );

    // populate return value
    if ( array_init( return_value ) != SUCCESS )
        RETURN_FALSE;

    for ( vector< pair<string, string> >::const_iterator it
        = contentTypes.begin(); it != contentTypes.end(); ++it )
        add_assoc_string( return_value,
            (char *) it->first.c_str(), (char *) it->second.c_str(), 1 );

    // done
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
