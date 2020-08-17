# Change Log
All notable changes to this project are documented in this file.

The format is based on [Keep a CHANGELOG](http://keepachangelog.com/)

### 2.3.3 - 2020-08-10
### Fixed:
  - Fixed handle resource leak exhausting TPM resources.

## 2.3.2 - 2020-05-18
### Added
- Added cirrus CI specific config files to enable FreeBSD builds.
### Changed
- Changed test scripts to be more portable.
- Changed include header paths specific to FreeBSD.

## 2.3.1 - 2020-01-10
### Fixed
- Provide meaningful exit codes on initialization failures.
- Prevent systemd from starting the daemon before udev changes ownership
of the TPM device node.
- Prevent systemd from starting the daemon if there is no TPM device
node.
- Prevent systemd from restarting the daemon if it fails.
- Add SELinux policy to allow daemon to resolve names.
- Add SELinux policy boolean (disabled by default) to allow daemon to
connect to all unreserved ports.

## 2.3.0 - 2019-11-13
### Added
- Add '--enable-debug' flag to configure script to simplify debug builds.
This relies on the AX_CHECK_ENABLE_DEBUG autoconf archive macro.
### Canged
- Replaced custom dynamic TCTI loading code with libtss2-tctildr from
upstream tpm2-tss repo.
### Fixed
- Explicitly set '-O2' optimization when using FORTIFY_SOURCE as required.

## 2.2.0 - 2019-07-17
### Added
- New configuration option `--disable-defaultflags/ added. This is
for use for packaging for targets that do not support the default
compilation / linking flags.
### Changed
- Use private dependencies properly in pkg-config metadata for TCTI.
- Refactor daemon main module to enable better handling of error
conditions and enable more thorough unit testing.
- Updated dependencies to ensure compatibility with pkg-config fixes
in tpm2-tss.
### Fixed
- Bug causing TCTI to block when used by libtss2-sys built with partial
reads enabled.
- Removed unnecessary libs / flags for pthreads in the TCTI pkg-config.
- Output from configure script now accurately describes the state of the
flags that govern the integration tests.

## 2.1.1 - 2019-03-08
### Fixed
- Unit tests accessing dbus have been fixed to use mock functions. Unit
tests no longer depend on dbus.
- Race condition between client connections and dbus proxy object
creation by registering bus name after instantiation of the proxy object.

## 1.3.3 - 2019-03-08
### Fixed
- Race condition between client connections and dbus proxy object
creation by registering bus name after instantiation of the proxy object.

## 2.1.0 - 2019-02-06
### Added
- `-Wstrict-overflow=5` now used in default CFLAGS.
- Handling of `TPM2_RC_CONTEXT_GAP` on behalf of users.
- Convert `TPM2_PT_CONTEXT_GAP_MAX` response from lower layer to
`UINT32_MAX`
### Changed
- travis-ci now uses 'xenial' builder
- Significant refactoring of TCTI handling code.
- `--install` added to ACLOCAL_AMFLAGS to install aclocal required macros
instead of using the default symlinks
- Launch `dbus-run-session` in the automake test environment to
automagically set up a dbus session bus instance when one isn't present.
### Fixed
- Bug caused by unloading of `libtss2-tcti-tabrmd.so` on dlclose. GLib
does not support reloading a second time.
- Bug causing `-fstack-protector-all` to be used on systems with core
libraries (i.e. libc) that do not support it. This caused failures at
link-time.
- Unnecessary symbols from libtest utility library no longer included in
TCTI library.

## 2.0.3 - 2018-10-31
### Fixed
- Update build to account for upstream change to glib '.pc' files
described in: https://gitlab.gnome.org/GNOME/glib/issues/1521

## 2.0.2 - 2018-09-10
### Fixed
- Merge fixes from 1.3.2
- --enable-integration option to configure script now works as documented.

## 1.3.2 - 2018-09-10
### Fixed
- Format specifier with wrong size in util module.
- Initialize TCTI context to 0 before setting values. This will cause all
  members that aren't explicitly initialized by be 0.

## 2.0.1 - 2018-07-25
### Fixed
- Default shared object name for dynamic TCTI loading now includes major
version number suffix.
- Race condition in between session mgmt and connection teardown logic.
- Leaked references to Connection objects in SessionList when calculating
the number of sessions belonging to a Connection.
- Replace old '01org' URLs in configure script.

## 2.0.0 - 2018-06-22
### Added
- Integration test script and build support to execute integration tests
against a physical TPM2 device on the build platform.
- Implementation of dynamic TCTI initialization mechanism.
- configure option `--enable-integration` to enable integration tests.
The simulator executable must be on PATH.
- Support for version 2.0 of tpm2-tss libraries.
### Changed
- 'max-transient-objects' command line option renamted to 'max-transients'.
- Added -Wextra for more strict checks at compile time.
- Install location of headers to $(includedir)/tss2.
### Fixed
- Added missing checks for NULL parameters identified by the check-build.
- Bug in session continuation logic.
- Off by one error in HandleMap.
- Memory leak and uninitialized variable issues in unit tests.
### Removed
- Command line option --fail-on-loaded-trans.
- udev rules for TPM device node. This now lives in the tpm2-tss repo.
- Remove legacy TCTI initialization functions.
- configure option `--with-simulatorbin`.

## 1.3.1 - 2018-03-18
### Fixed
- Distribute systemd preset template instead of the generated file.

## 1.3.0 - 2018-03-02
### Added
- New configure option (--test-hwtpm) to run integration tests against a
physical TPM2 device on the build platform.
- Install systemd service file to allow on-demand systemd unit activation.
### Changed
- Converted some inappropriate uses of g_error to critical / warning instead.
- Removed use of gen_require from SELinux policy, use dbus_stub instead.
- udev rules now give tss group read / write access to the TPM device node.
- udev rules now give tss user and group read / write access to kernel RM
node.
### Fixed
- Memory leak on an error path in the AccessBroker.

## 1.2.0 - 2017-12-08
### Added
- Check SAPI library is < 2.0.0 (API change upstream).
- Abstract class for IPC frontend implementation. Port dbus code from main
module to class inheriting from the IpcFrontend.
- SELinux policy module to work around policy in Fedora.
- Limit maximum number of active sessions per connection with '--max-sessions'.
- Flush all transient objects and sessions on daemon start with '--flush-all'.
- Allow passing of sessions across connections with ContextSave / Load.
### Changed
- Set valgrind leak-check flag to 'full'.
- Client / server communication uses PF_LOCAL sockets instead of pipes.
- bootstrap script now creates VERSION file from 'git describe'. Autoconf gets
version string from it, automake distributes it in 'distcheck'.
- Test harness upgraded to simulator version 974.
- Unit tests upgraded to the 1.x cmocka API.
- Replace use of thread in CommandSource with GMainLoop.
- Replace use of file descriptors with GIO streams.
- Separate 'dispose' and 'finalize' functions in each object.
- Move creation of FDs from connection_new to calling context (dependency
inversion).
### Fixed
- Unref the GUnixFDList returned by GIO / dbus in the TCTI init function.
This fixes a memory leak in the TCTI library.

## 1.1.1 - 2017-08-25
### Added
- Systemd 'preset' file and corresponding options to the configure script.
- Option to configure script to allow the addition of a string prefix to
the udev rules file name.
### Changed
- Replace use of sigaction with g_unix_signal_* stuff from glib.
- Rewrite of INSTALL.md including info on custom configure script options.
- Default value for --with-simulatorbin configure option has been removed.
New default behavior is to disable integration tests.
- CommandSource will no longer reject commands without parameters.
- Unit tests updated to use cmocka v1.0.0 API.
- Integration tests now run daemon under valgrind memcheck and fail when
errors are found.
- CommandSource now tracks max FD in set of client FDs to prevent unnecessary
iterations over FD_SETSIZE fds.
### Fixed
- Release tarballs now include essential files missing from 1.1.0 release.
- Robustness fixes in CommandSource.
- Stability fixes in Tpm2Command handling that could result in crashes.
- int-log-compiler.sh now fails if required binaries not found.
- check-valgrind target now depends on check_PROGRAMS to ensure daemon is
built before tests are run.
- NULL deref bug in TCTI.
- Mishandling of short reads in util module.
- Race condition on daemon shutdown that could cause deadlock.
- Several logic errors & data initialization for more strict compiler
versions.

## 1.1.0 - 2017-07-01
### Added
- Integration test harness supporting parallel execution using automake
- Environment variables TABRMD_TEST_BUS_TYPE and TABRMD_TEST_BUS_NAME to
control D-Bus type and name selection respectively in the integration test
harness.
- tss2_tcti_tabrmd_init_full function to libtcti-tabrmd to allow for selection
of D-Bus bus type and name used by tpm2-abrmd instance.
- Command line option --dbus-name to control the name claimed by the daemon
on the D-Bus.
- Command line option --prng-seed-file to allow configuration of seed source.
The default is /dev/urandom. The only use of the PRNG in the daemon is to
differentiate between the connections held by a single client.
infrastructure.
- Lots of new unit and integration tests.
- Automated coverity static analysis scans.
### Changed
- New configuration option to specify location of simulator binary (required
for integration tests).
- Lots of documentation updates (README.md / INSTALL.md)
- Travis-CI now executes all tests under valgrind / memcheck.
### Fixed
- Deconflict command line options: -t for TCTI selection, -a to fail if
transient objects are already loaded in the TPM.
- Clients can hold multiple TCTI connections again (fixed regression).
- Syslog log handler now only shows info & debug messages when
G_MESSAGES_DEBUG is set to 'all'.
- Free memory in error path in integration test harness.
- distcheck make target now works.

## 1.0.0 - 2017-05-21
### Added
- Everything - initial release.
