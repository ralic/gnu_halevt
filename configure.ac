AC_INIT([halevt],[0.1.4])
AC_CONFIG_SRCDIR([src/manager.c])
AC_CONFIG_HEADER([src/config.h])
AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE

AC_PROG_CC
AM_PROG_CC_C_O

AC_PATH_PROG(MAN2HTML, man2html, "no")
AM_CONDITIONAL(USE_MAN2HTML, [test x$MAN2HTML != xno])

AC_PATH_PROG(MAKEINFO, makeinfo, "no")
AM_CONDITIONAL(USE_MAKEINFO, [test x$MAKEINFO != xno])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])

PKG_CHECK_MODULES(LIBXML, libxml-2.0, have_libxml=true, have_libxml=false)
if test "x$have_libxml" = "xfalse" ; then
        AC_MSG_ERROR([No LibXML library found, check config.log for failed attempts])
fi
AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LIBS)

PKG_CHECK_MODULES(BOOLSTUFF, boolstuff-0.1 >= 0.1.12, have_boolstuff=true, have_boolstuff=false)
if test "x$have_boolstuff" = "xfalse" ; then
        AC_MSG_ERROR([No boolstuff library found, check config.log for failed attempts])
fi
AC_SUBST(BOOLSTUFF_CFLAGS)
AC_SUBST(BOOLSTUFF_LIBS)

PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1 >= 0.3 , have_dbus_glib=true, have_dbus_glib=false)
if test "x$have_dbus_glib" = "xfalse" ; then
   AC_MSG_ERROR([No Dbus-Glib found, check config.log for failed attempts])
fi
AC_SUBST(DBUS_GLIB_CFLAGS)
AC_SUBST(DBUS_GLIB_LIBS)

# DBus is pulled as a dependency of DBUS_GLIB, but we also need it 
# separately for halevt-mount which doesn't depend on DBUS_GLIB
PKG_CHECK_MODULES(DBUS, dbus-1, have_dbus=true, have_dbus=false)
if test "x$have_dbus" = "xfalse" ; then
   AC_MSG_ERROR([No DBus found, check config.log for failed attempts])
fi
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(HAL, hal >= 0.5 , have_hal=true, have_hal=false)
if test "x$have_hal" = "xfalse" ; then
   AC_MSG_ERROR([No Hal found, check config.log for failed attempts])
fi
AC_SUBST(HAL_CFLAGS)
AC_SUBST(HAL_LIBS)

halevt_default_user=halevt
AC_ARG_ENABLE([default-user],
            [AS_HELP_STRING([--enable-default-user],
              [default user name to run halevt as])],
            [halevt_default_user=$enableval],
            [])
halevt_default_group=halevt
AC_ARG_ENABLE([default-group],
            [AS_HELP_STRING([--enable-default-group],
              [default group name to run halevt as])],
            [halevt_default_group=$enableval],
            [])

AC_DEFINE_UNQUOTED([USER], ["$halevt_default_user"], [Default user.])
AC_DEFINE_UNQUOTED([GROUP], ["$halevt_default_group"], [Default group.])

HALEVT_USER_NAME=$halevt_default_user
HALEVT_GROUP_NAME=$halevt_default_group

AC_SUBST(HALEVT_USER_NAME)
AC_SUBST(HALEVT_GROUP_NAME)

halevt_piddir='${localstatedir}/run/${PACKAGE}'
AC_ARG_WITH([piddir],
            [AS_HELP_STRING([--with-piddir],
              [directory used for the system pid file])],
            [halevt_piddir=$withval],
            [])
AC_SUBST(halevt_piddir)

AC_CONFIG_FILES([Makefile  po/Makefile.in halevt-mount/po/Makefile.in\
              src/Makefile \
              examples/Makefile \
              doc/Makefile \
              doc/references/Makefile \
              scripts/Makefile \
              halevt-mount/Makefile
])
AC_OUTPUT
