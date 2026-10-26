#                                               -*- Autoconf -*-
# configure.ac -- Autoconf script for agm
#

# Process this file with autoconf to produce a configure script

# Requires autoconf tool later than 2.61
AC_PREREQ(2.69)

AC_INIT([agm],1.1.1)
# Does not strictly follow GNU Coding standards
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
LT_INIT
AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_AR
AC_PROG_CXX

m4_define([LT_MAJOR], [1])
m4_define([LT_MINOR], [1])
m4_define([LT_PATCH], [1])

AC_SUBST([LT_VERSION], LT_MAJOR.LT_MINOR.LT_PATCH)
AC_SUBST([LT_VERSION_NUMBER], LT_MAJOR:LT_MINOR:LT_PATCH)

AC_ARG_WITH([syslog],
    AS_HELP_STRING([use syslog (default is no)]),
    [with_syslog=$withval],
    [with_syslog=no])
AM_CONDITIONAL([USE_SYSLOG], [test "x${with_syslog}" = "xyes"])

AC_ARG_WITH([are_on_apps],
    AS_HELP_STRING([--with-are-on-apps],[Enable ARE (SPF) on APPS support (default is no)]),
    [with_are_on_apps=$withval],
    [with_are_on_apps=no])
AM_CONDITIONAL([USE_ARE_ON_APPS], [test "x${with_are_on_apps}" = "xyes"])

AC_ARG_WITH([use_default_acdb_path],
    AS_HELP_STRING([--with-use-default-acdb-path],[ Use default ACDB path specified by ACDB_PATH macro. (default is no)]),
    [with_use_default_acdb_path=$withval],
    [with_use_default_acdb_path=no])
AM_CONDITIONAL([USE_DEFAULT_ACDB_PATH], [test "x${with_use_default_acdb_path}" = "xyes"])

AC_ARG_WITH([card-def-native],
    AS_HELP_STRING([--with-card-def-native=PATH],
        [path to native card-defs xml (default /etc/card-defs-native.xml)]),
    [card_def_native=$withval],
    [card_def_native="/etc/card-defs-native.xml"])
AC_SUBST([CARD_DEF_FILE_NATIVE], [$card_def_native])

# Optional ALSALIB build support
AC_ARG_ENABLE([alsalib],
  [AS_HELP_STRING([--enable-alsalib], [Enable ALSA AGM plug-in build (default: no)])],
  [enable_alsalib=$enableval],
  [enable_alsalib=no])
AM_CONDITIONAL([BUILD_ALSALIB], [test "x$enable_alsalib" = "xyes"])

AC_ARG_WITH([cutils],
    AS_HELP_STRING([Use cutils (default is no)]),
    [with_cutils=$withval],
    [with_cutils=no])
AM_CONDITIONAL([CUTILS_SUPPORTED], [test "x${with_cutils}" = "xyes"])

AC_ARG_WITH([glib],
      AC_HELP_STRING([--with-glib],
         [enable glib, Build against glib. Use this when building for HLOS systems which use glib]))

if (test "x${with_glib}" = "xyes"); then
        PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= 2.16, dummy=yes,
                                AC_MSG_ERROR(GThread >= 2.16 is required))
        PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.16, dummy=yes,
                                AC_MSG_ERROR(GLib >= 2.16 is required))
        GLIB_CFLAGS="$GLIB_CFLAGS $GTHREAD_CFLAGS"
        GLIB_LIBS="$GLIB_LIBS $GTHREAD_LIBS"

        AC_SUBST(GLIB_CFLAGS)
        AC_SUBST(GLIB_LIBS)
fi

AM_CONDITIONAL(USE_GLIB, test "x${with_glib}" = "xyes")

AC_CHECK_FUNCS([strlcpy])
AM_CONDITIONAL(USE_G_STR_FUNC,
    [test "x${with_glib}" = "xyes" && ! test "x$ac_cv_func_strlcpy" = "xyes"])

AC_ARG_WITH([no_ipc],
    AS_HELP_STRING([--with-no-ipc],[disable ipc communication to AGM (default is no)]),
    [with_no_ipc=$withval],
    [with_no_ipc=no])
AM_CONDITIONAL([AGM_NO_IPC], [test "x${with_no_ipc}" = "xyes"])

AC_ARG_WITH([ipc_dbus],
    AS_HELP_STRING([--with-dbus],[enables dbus ipc communication to AGM (default is no)]),
    [with_ipc_dbus=$withval],
    [with_ipc_dbus=no])
AM_CONDITIONAL([AGM_IPC_DBUS], [test "x${with_ipc_dbus}" = "xyes"])

AC_ARG_WITH([ipc_swbinder],
    AS_HELP_STRING([--with-sw-binder], [enable SwBinder based ipc communication to AGM (default is no)]),
    [with_ipc_swbinder=$withval],
    [with_ipc_swbinder=no])
AM_CONDITIONAL([AGM_IPC_SWBINDER], [test "x${with_ipc_swbinder}" = "xyes"])

AC_ARG_WITH([armemlog],
    AS_HELP_STRING([use armemlog (default is no)]),
    [with_armemlog=$withval],
    [with_armemlog=no])
AM_CONDITIONAL([MEMLOGGER_SUPPORTED], [test "x${with_armemlog}" = "xyes"])

PKG_CHECK_MODULES([SPF], [spf])
AC_SUBST(SPF_CFLAGS)

PKG_CHECK_MODULES([KVH2XML], [kvh2xml])
AC_SUBST([KVH2XML_CFLAGS])

PKG_CHECK_MODULES([GSL], [gsl])
AC_SUBST([GSL_LIBS])
AC_SUBST([GSL_CFLAGS])

PKG_CHECK_MODULES([OSAL], [ar_osal])
AC_SUBST([OSAL_LIBS])
AC_SUBST([OSAL_CFLAGS])

PKG_CHECK_MODULES([ATS], [ats])
AC_SUBST([ATS_LIBS])
AC_SUBST([ATS_CFLAGS])

if (test "x${with_are_on_apps}" = "xyes"); then
    PKG_CHECK_MODULES([GPR], [gpr])
    AC_SUBST([GPR_LIBS])
    AC_SUBST([GPR_CFLAGS])
fi

# tinycompress is not available on all platforms (e.g. Raspberry Pi), so
# probe for it optionally instead of failing configure when it is absent.
PKG_CHECK_MODULES([TINYCOMPRESS], [tinycompress], [have_tinycompress=yes], [have_tinycompress=no])
AM_CONDITIONAL([HAVE_TINYCOMPRESS], [test "x${have_tinycompress}" = "xyes"])
AC_SUBST([TINYCOMPRESS_CFLAGS])
AC_SUBST([TINYCOMPRESS_LIBS])

# Check dependency only if with_no_ipc is not set
if (test "x${with_ipc_dbus}" = "xyes"); then
PKG_CHECK_MODULES(DBUS, [ dbus-1 >= 1.4.12 ], dummy=yes,
                 AC_MSG_ERROR(dbus-1 >= 1.4.12 is required))

AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)
fi

AC_CONFIG_FILES(
Makefile
service/Makefile
service/agm.pc
ipc/DBus/agm_client/Makefile
ipc/DBus/agm_client/agmclient.pc
ipc/DBus/agm_server/Makefile
ipc/DBus/agm_server/agmserver.pc
snd_parser/Makefile
snd_parser/sndparser.pc
plugins/tinyalsa/Makefile
plugins/tinyalsa/agmplugin.pc
plugins/tinyalsa/test/Makefile
plugins/tinyalsa/test/agmtest.pc
plugins/alsalib/Makefile
plugins/alsalib/agmalsaplugin.pc
ipc/SwBinders/agm_server/Makefile
ipc/SwBinders/agm_server/agmserver.pc
ipc/SwBinders/agm_client/Makefile
ipc/SwBinders/agm_client/agmclient.pc
)

AC_OUTPUT
