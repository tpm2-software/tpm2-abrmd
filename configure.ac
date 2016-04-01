m4_define([tss2tab_major_version], [0])
m4_define([tss2tab_minor_version], [1])
m4_define([tss2tab_version_string], [tss2tab_major_version.tss2tab_minor_version])

AC_INIT([tss2tab], [tss2tab_version_string])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC
LT_INIT()
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_FILES([Makefile])
PKG_CHECK_MODULES([DBUS], [dbus-1])
PKG_CHECK_MODULES([GIO], [gio-unix-2.0])
AC_OUTPUT
