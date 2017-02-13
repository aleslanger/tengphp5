dnl $Id: config.m4,v 1.2 2004-09-24 12:43:34 vasek Exp $
dnl config.m4 for teng php extension

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(teng, for teng support,
[  --with-teng[=DIR]       Include Teng support where DIR is Teng install
                          prefix.  ])


# enable teng extension
if test "$PHP_TENG" != "no"; then

  LIBNAME=teng
  LIBSYMBOL=teng_library_present

  # check for libteng
  AC_LANG_CPLUSPLUS

  # --with-teng -> check with-path
  SEARCH_PATH="/usr/local /usr"
  SEARCH_FOR="/include/teng.h"
  if test -r $PHP_TENG/; then # path given as parameter
    TENG_DIR=$PHP_TENG
  else # search default path list
    AC_MSG_CHECKING([for teng files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        TENG_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

  if test -z "$TENG_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the teng distribution])
  fi

  # --with-teng -> add include path
  PHP_ADD_INCLUDE($TENG_DIR/include)

  # --with-teng -> check for lib and symbol
  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $TENG_DIR/lib, TENG_SHARED_LIBADD)
   AC_DEFINE(HAVE_TENGLIB,1,[ ])
   ],[
   AC_MSG_ERROR([wrong teng lib version or lib not found])
   ],[
   -L$TENG_DIR/lib -lm -ldl
  ])

  PHP_ADD_INCLUDE( $PHP_TENG )
  PHP_ADD_LIBRARY(stdc++,1, TENG_SHARED_LIBADD)
  PHP_SUBST(TENG_SHARED_LIBADD)
  PHP_EXTENSION(teng,$ext_shared)
  PHP_REQUIRE_CXX()
fi
