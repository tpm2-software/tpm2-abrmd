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
# function from the gnu.org docs
AC_DEFUN([MY_ARG_WITH],
         [AC_ARG_WITH(m4_translit([[$1]], [_], [-]),
                      [AS_HELP_STRING([--with-m4_translit([$1], [_], [-])],
                                      [use $1 (default is $2)])],
                      [use_[]$1=$withval],
                      [use_[]$1=$2])
         ])
MY_ARG_WITH([tcti_device], [yes])
AS_IF([test "x$use_tcti_device" != xno],
      [PKG_CHECK_MODULES([TCTI_DEVICE],
                         [tcti-device],
                         [AC_DEFINE([HAVE_TCTI_DEVICE],
                                    [1])])])
AM_CONDITIONAL([TCTI_DEVICE], [test "x$use_tcti_device" != xno])
MY_ARG_WITH([tcti_socket], [yes])
AS_IF([test "x$use_tcti_socket" != xno],
      [PKG_CHECK_MODULES([TCTI_SOCKET],
                         [tcti-socket],
                         [AC_DEFINE([HAVE_TCTI_SOCKET],
                                    [1])])])
AM_CONDITIONAL([TCTI_SOCKET], [test "x$use_tcti_socket" != xno])
AC_OUTPUT
