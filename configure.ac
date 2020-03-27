# SPDX-License-Identifier: BSD-2-Clause
AC_INIT([tpm2-abrmd],
        [m4_esyscmd_s([cat ./VERSION])],
        [https://github.com/tpm2-software/tpm2-abrmd/issues],
        [],
        [https://github.com/tpm2-software/tpm2-abrmd])
AC_CONFIG_MACRO_DIR([m4])
AX_IS_RELEASE(dash-version)
AX_CHECK_ENABLE_DEBUG([info])
AC_PROG_CC
AC_PROG_LN_S
AC_USE_SYSTEM_EXTENSIONS
LT_INIT()
PKG_INSTALLDIR()
AM_INIT_AUTOMAKE([foreign subdir-objects])
# enable "silent-rules" option by default
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AX_RECURSIVE_EVAL([$sbindir], [SBINDIR])
AC_SUBST([SBINDIR])

AC_CONFIG_FILES([Makefile dist/tss2-tcti-tabrmd.pc dist/tpm2-abrmd.service dist/tpm2-abrmd.preset])

# propagate configure arguments to distcheck
AC_SUBST([DISTCHECK_CONFIGURE_FLAGS],[$ac_configure_args])

AX_PTHREAD([], [AC_MSG_ERROR([requires pthread])])
AC_ARG_ENABLE([unit],
              [AS_HELP_STRING([--enable-unit],
                   [build cmocka unit tests])],,
              [enable_unit=no])
AS_IF([test "x$enable_unit" != xno],
      [PKG_CHECK_MODULES([CMOCKA],
                         [cmocka >= 1.0])])
AM_CONDITIONAL([UNIT], [test "x$enable_unit" != xno])

# -dl or -dld
AC_SEARCH_LIBS([dlopen], [dl dld], [], [
  AC_MSG_ERROR([unable to find the dlopen() function])
])
PKG_CHECK_MODULES([GIO], [gio-unix-2.0])
PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([GOBJECT], [gobject-2.0])
PKG_CHECK_MODULES([TSS2_SYS],[tss2-sys >= 2.0.0])
PKG_CHECK_MODULES([TSS2_MU],[tss2-mu])
PKG_CHECK_MODULES([TSS2_TCTILDR],[tss2-tctildr])
PKG_CHECK_MODULES([TSS2_RC],[tss2-rc])
AC_ARG_VAR([GDBUS_CODEGEN],[The gdbus-codegen executable.])
AC_CHECK_PROG([GDBUS_CODEGEN], [gdbus-codegen], [gdbus-codegen])
AS_IF([test ! -x "$(which $GDBUS_CODEGEN)"],
      [AC_MSG_ERROR([*** gdbus-codegen is required to build tpm2-abrmd])])

# Check OS and set library and compile flags accordingly
case "${host_os}" in
    *bsd* | *BSD*)
        HOSTOS='BSD'
        ;;
    *)
        #Assume linux
        HOSTOS='Linux'
        ;;
esac
AC_SUBST([HOSTOS])

AX_CODE_COVERAGE
m4_ifdef([_AX_CODE_COVERAGE_RULES],
         [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [true])],
         [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [false])])
AX_ADD_AM_MACRO_STATIC([])

# disable helgrind and drd, they hate GAsyncQueue
AX_VALGRIND_DFLT([sgcheck], [off])
AX_VALGRIND_DFLT([helgrind], [off])
AX_VALGRIND_DFLT([drd], [off])
AX_VALGRIND_CHECK

# allow
AC_ARG_ENABLE([dlclose],
  [AS_HELP_STRING([--disable-dlclose],
    [Some versions of libc cause a sigsegv on exit, this disables the dlclose and works around that bug])],
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
                           [Directory for systemd service files]),,
            [with_systemdsystemunitdir=${libdir}/systemd/system])
AS_IF([test "x$with_systemdsystemunitdir" != xno],
      [AC_SUBST([systemdsystemunitdir],
      [$with_systemdsystemunitdir])])
AM_CONDITIONAL(HAVE_SYSTEMD, [test -n "$with_systemdsystemunitdir" -a "x$with_systemdsystemunitdir" != xno ])

# systemd preset directory
AC_ARG_WITH([systemdpresetdir],
            AS_HELP_STRING([--with-systemdpresetdir=DIR],
                           [Directory for systemd preset files]),,
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
            [AS_HELP_STRING([--with-dbuspolicydir=DIR],[D-Bus policy directory])],,
            [with_dbuspolicydir=${sysconfdir}/dbus-1/system.d])
AX_NORMALIZE_PATH([with_dbuspolicydir])
AC_SUBST([dbuspolicydir], [$with_dbuspolicydir])

m4_define([PATH_PROG],
    [
        AC_PATH_PROG([$1],[$2],[no])
        AS_IF([test "x$$1" = "xno"],
              [AC_MSG_ERROR([missing required command: $2])])
    ])
# selinux reference policy
AC_ARG_WITH(
    [sepolicy],
    AS_HELP_STRING([--with-sepolicy=@<:@/usr/share/selinux/devel/include@:>@],
        [build SELinux policy module]),
    [
        AM_PATH_PYTHON([3])
        PATH_PROG([BZIP2],[bzip2])
        PATH_PROG([CHECKMODULE],[checkmodule])
        PATH_PROG([M4],[m4])
        PATH_PROG([SED],[sed])
        PATH_PROG([SEMODULE_PACKAGE],[semodule_package])
        AS_IF([test "x$with_sepolicy" = xyes],
              with_sepolicy=/usr/share/selinux/devel/include)
        AS_IF(
            [test -d $with_sepolicy],
            [
                sepol_support=$(find ${with_sepolicy}/support -type f \
                                    -name '*.spt' | sort | tr '\n' ' ')
                sepol_interfaces=$(find $with_sepolicy -type f -name '*.if' | \
                                       sort | tr '\n' ' ')
            ],
            [AC_MSG_ERROR([$with_sepolicy isn't a directory])])
        AC_SUBST([SEPOL_INTERFACES],[$sepol_interfaces])
        AC_SUBST([SEPOL_SUPPORT],[$sepol_support])
        AC_SUBST(
            [SEPOL_M4PARAM],
            ["-D enable_mcs -D distro_redhat -D hide_broken_symptoms \
              -D mls_num_sens=16 -D mls_num_cats=1024 -D mcs_num_cats=1024"])
    ],[with_sepolicy=no])
AM_CONDITIONAL([WITH_SEPOLICY], [test "x$with_sepolicy" != xno])

#
# Real TPM hardware
#
AC_ARG_ENABLE([test-hwtpm],
              [AS_HELP_STRING([--enable-test-hwtpm],
                  [enable the integration test on a real tpm hardware])],
              [enable_integration=$enableval],
              [enable_test_hwtpm=no])
AM_CONDITIONAL([HWTPM], [test "x$enable_test_hwtpm" != xno])
#
# enable integration tests and check for simulator binary
#
AC_ARG_ENABLE([integration],
    [AS_HELP_STRING([--enable-integration],
        [build and execute integration tests])],,
    [enable_integration=no])
AS_IF([test "x$enable_integration" = "xyes"],
    [AS_IF([test "x$enable_test_hwtpm" = "xno"],
        [AC_CHECK_PROG([tpm_server], [tpm_server], [yes], [no])
         AS_IF([test "x$tpm_server" != "xyes"],
             [AC_MSG_ERROR([Integration tests enabled but tpm_server not found, try setting PATH])],
             [AC_MSG_NOTICE([Integration tests will be executed against the TPM2 simulator.])])
         AS_IF([test "$HOSTOS" = "Linux"],
           [AC_CHECK_PROG(ss, [ss], [yes], [no])],
           [AC_CHECK_PROG(ss, [sockstat], [yes], [no])])
         AS_IF([test "x$ss" != "xyes"],
             [AC_MSG_ERROR([Integration tests require the sockstat/ss executable to be installed.])])],
        [AC_MSG_NOTICE([Integration tests will be executed against the TPM device.])])])
AM_CONDITIONAL([ENABLE_INTEGRATION],[test "x$enable_integration" = "xyes"])

AC_ARG_ENABLE([defaultflags],
              [AS_HELP_STRING([--disable-defaultflags],
                              [Disable default preprocessor, compiler, and linker flags.])],,
              [enable_defaultflags=yes])
AS_IF([test "x$enable_defaultflags" = "xyes"],
      [
      # preprocessor / compiler / linker flags
      #   these macros are defined in m4/flags.m4
      AS_IF([test "x$enable_debug" = "xno"],
            [AX_ADD_FORTIFY_SOURCE
             ADD_COMPILER_FLAG([-O2])])
      ADD_COMPILER_FLAG([-Wall])
      ADD_COMPILER_FLAG([-Wextra])
      AS_IF([test "x$ax_is_release" = "xno"],
            [ADD_COMPILER_FLAG([-Werror])])
      ADD_COMPILER_FLAG([-std=gnu99])
      ADD_COMPILER_FLAG([-Wformat])
      ADD_COMPILER_FLAG([-Wformat-security])
      ADD_COMPILER_FLAG([-Wno-missing-braces])
      ADD_COMPILER_FLAG([-fdata-sections])
      ADD_COMPILER_FLAG([-ffunction-sections])
      ADD_TOOLCHAIN_FLAG([-fstack-protector-all])
      ADD_COMPILER_FLAG([-fpic])
      ADD_COMPILER_FLAG([-fPIC])
      ADD_COMPILER_FLAG([-Wstrict-overflow=5])
      ADD_LINK_FLAG([-Wl,--gc-sections])
      ADD_LINK_FLAG([-Wl,--no-undefined])
      ADD_LINK_FLAG([-Wl,-z,noexecstack])
      ADD_LINK_FLAG([-Wl,-z,now])
      ADD_LINK_FLAG([-Wl,-z,relro])
      ])

AC_SUBST([PATH])

AC_OUTPUT
