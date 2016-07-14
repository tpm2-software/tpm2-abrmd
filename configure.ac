m4_define([tss2tab_major_version], [0])
m4_define([tss2tab_minor_version], [1])
m4_define([tss2tab_version_string], [tss2tab_major_version.tss2tab_minor_version])

AC_INIT([tss2tab], [tss2tab_version_string])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC
LT_INIT()
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_FILES([Makefile])
AX_PTHREAD([], [AC_MSG_ERROR([requires pthread])])
PKG_CHECK_MODULES([CMOCKA], [cmocka])
PKG_CHECK_MODULES([DBUS], [dbus-1])
PKG_CHECK_MODULES([GIO], [gio-unix-2.0])
PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([GOBJECT], [gobject-2.0])
PKG_CHECK_MODULES([SAPI],[sapi])
AC_ARG_VAR([GDBUS_CODEGEN],[The gdbus-codegen executable.])
AC_PATH_PROG([GDBUS_CODEGEN], [`$PKG_CONFIG --variable=gdbus_codegen gio-2.0`])
if test -z "$GDBUS_CODEGEN"; then
    AC_MSG_ERROR([*** gdbus-codegen is required to build tss2-tabd])
fi
# disable helgrind and drd, they hate GAsyncQueue
AX_VALGRIND_DFLT([helgrind], [off])
AX_VALGRIND_DFLT([drd], [off])
AX_VALGRIND_CHECK
AC_OUTPUT
