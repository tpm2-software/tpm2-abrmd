AC_INIT([tpm2-abrmd],
        [m4_esyscmd_s([cat ./VERSION])],
        [https://github.com/01org/tpm2-abrmd/issues],
        [],
        [https://github.com/01org/tpm2-abrmd])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC
AC_PROG_LN_S
LT_INIT()
AM_INIT_AUTOMAKE([foreign subdir-objects])
# enable "silent-rules" option by default
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_FILES([Makefile])

# propagate configure arguments to distcheck
AC_SUBST([DISTCHECK_CONFIGURE_FLAGS],[$ac_configure_args])

AX_PTHREAD([], [AC_MSG_ERROR([requires pthread])])
AC_ARG_ENABLE([unit],
              [AS_HELP_STRING([--enable-unit],
                   [build cmocka unit tests (default is no)])],
              [enable_unit=$enableval],
              [enable_unit=no])
AS_IF([test "x$enable_unit" != xno],
      [PKG_CHECK_MODULES([CMOCKA],
                         [cmocka >= 1.0],
                         [AC_DEFINE([HAVE_CMOCKA],
                                    [1],
                                    [cmocka is available])])])
AM_CONDITIONAL([UNIT], [test "x$enable_unit" != xno])

# -dl or -dld
AC_SEARCH_LIBS([dlopen], [dl dld], [], [
  AC_MSG_ERROR([unable to find the dlopen() function])
])
PKG_CHECK_MODULES([DBUS], [dbus-1])
PKG_CHECK_MODULES([GIO], [gio-unix-2.0])
PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([GOBJECT], [gobject-2.0])
PKG_CHECK_MODULES([TSS2_SYS],[tss2-sys >= 2.0.0])
AC_ARG_VAR([GDBUS_CODEGEN],[The gdbus-codegen executable.])
AC_PATH_PROG([GDBUS_CODEGEN], [`$PKG_CONFIG --variable=gdbus_codegen gio-2.0`])
if test -z "$GDBUS_CODEGEN"; then
    AC_MSG_ERROR([*** gdbus-codegen is required to build tpm2-abrmd])
fi

AX_CODE_COVERAGE
# disable helgrind and drd, they hate GAsyncQueue
AX_VALGRIND_DFLT([sgcheck], [off])
AX_VALGRIND_DFLT([helgrind], [off])
AX_VALGRIND_DFLT([drd], [off])
AX_VALGRIND_CHECK

# allow
AC_ARG_ENABLE([dlclose],
  [AS_HELP_STRING([--disable-dlclose],
    [Some versions of libc cause a sigsegv on exit, this disables the dlclose and works around that bug])],
  []
  [AC_DEFINE([DISABLE_DLCLOSE], [1])]
)

# function from the gnu.org docs
AC_DEFUN([MY_ARG_WITH],
         [AC_ARG_WITH(m4_translit([[$1]], [_], [-]),
                      [AS_HELP_STRING([--with-m4_translit([$1], [_], [-])],
                                      [use $1 (default is $2)])],
                      [use_[]$1=$withval],
                      [use_[]$1=$2])
         ])

#
# systemd
#
AC_ARG_WITH([systemdsystemunitdir],
            AS_HELP_STRING([--with-systemdsystemunitdir=DIR],
                           [Directory for systemd service files]),
            [],
            [with_systemdsystemunitdir=${libdir}/systemd/system])
AS_IF([test "x$with_systemdsystemunitdir" != xno],
      [AC_SUBST([systemdsystemunitdir],
      [$with_systemdsystemunitdir])])
AM_CONDITIONAL(HAVE_SYSTEMD, [test -n "$with_systemdsystemunitdir" -a "x$with_systemdsystemunitdir" != xno ])

# systemd preset directory
AC_ARG_WITH([systemdpresetdir],
            AS_HELP_STRING([--with-systemdpresetdir=DIR],
                           [Directory for systemd preset files]),
            [],
            [with_systemdpresetdir=${libdir}/systemd/system-preset])
AC_SUBST([systemdpresetdir], [$with_systemdpresetdir])

# systemd preset default (enable / disable)
AC_ARG_WITH([systemdpresetdisable],
             AS_HELP_STRING([--with-systemdpresetdisable],
                            [Configure systemd preset to 'disable', default is 'enable']),
            [AC_SUBST([SYSTEMD_PRESET_DEFAULT],[disable])],
            [AC_SUBST([SYSTEMD_PRESET_DEFAULT],[enable])])

#
# dbus
#
AC_ARG_WITH([dbuspolicydir],
            [AS_HELP_STRING([--with-dbuspolicydir=DIR],[D-Bus policy directory])],
            [],
            [with_dbuspolicydir=${sysconfdir}/dbus-1/system.d])
AX_NORMALIZE_PATH([with_dbuspolicydir])
AC_SUBST([dbuspolicydir], [$with_dbuspolicydir])
#
# Real TPM hardware
#
AC_ARG_ENABLE([test-hwtpm],
              [AS_HELP_STRING([--enable-test-hwtpm],
                  [enable the integration test on a real tpm hardware (default is no)])],
              [enable_hwtpm=$enableval
               enable_integration=$enableval],
              [enable_hwtpm=no])
AM_CONDITIONAL([HWTPM], [test "x$enable_hwtpm" != xno])
#
# enable integration tests and check for simulator binary
#
AC_ARG_ENABLE([integration],
    [AS_HELP_STRING([--enable-integration],
        [build and execute integration tests (default is no)])],
    [enable_integration=$enableval],
    [enable_integration=no])
AS_IF([test \( "x$enable_integration" = "xyes" \) -a \( "x$enable_hwtpm" = "xno" \)],
    [AC_CHECK_PROG([tpm_server], [tpm_server], [yes], [no])
     AS_IF([test "x$tpm_server" != "xyes"],
         [AC_MSG_ERROR([Integration tests enabled but tpm_server not found, try setting PATH])])
     AC_SUBST([ENABLE_INTEGRATION],[$enable_integration])
     AC_MSG_NOTICE([Integration tests will be executed against the TPM2 simulator.])],
    [AC_MSG_NOTICE([Integration tests will be executed against the TPM device.])])
AM_CONDITIONAL([ENABLE_INTEGRATION],[test "x$enable_integration" = "xyes"])

# preprocessor / compiler / linker flags
#   these macros are defined in m4/flags.m4
AX_ADD_COMPILER_FLAG([-Wall])
AX_ADD_COMPILER_FLAG([-Wextra])
AX_ADD_COMPILER_FLAG([-Werror])
AX_ADD_COMPILER_FLAG([-std=gnu99])
AX_ADD_COMPILER_FLAG([-Wformat])
AX_ADD_COMPILER_FLAG([-Wformat-security])
AX_ADD_COMPILER_FLAG([-Wno-missing-braces])
# work around for Glib usage of function pointers type casting
#   https://bugzilla.gnome.org/show_bug.cgi?id=793272
AX_ADD_COMPILER_FLAG([-Wno-cast-function-type])
AX_ADD_COMPILER_FLAG([-fdata-sections])
AX_ADD_COMPILER_FLAG([-ffunction-sections])
AX_ADD_COMPILER_FLAG([-fstack-protector-all])
AX_ADD_COMPILER_FLAG([-fpic])
AX_ADD_COMPILER_FLAG([-fPIC])
AX_ADD_PREPROC_FLAG([-D_GNU_SOURCE])
AX_ADD_PREPROC_FLAG([-U_FORTIFY_SOURCE])
AX_ADD_PREPROC_FLAG([-D_FORTIFY_SOURCE=2])
AX_ADD_LINK_FLAG([-Wl,--gc-sections])
AX_ADD_LINK_FLAG([-Wl,--no-undefined])
AX_ADD_LINK_FLAG([-Wl,-z,noexecstack])
AX_ADD_LINK_FLAG([-Wl,-z,now])
AX_ADD_LINK_FLAG([-Wl,-z,relro])

AC_SUBST([PATH])

AC_OUTPUT
