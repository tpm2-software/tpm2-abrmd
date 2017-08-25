# Change Log
All notable changes to this project are documented in this file.

The format is based on [Keep a CHANGELOG](http://keepachangelog.com/)

## Unreleased
### Added
- Check SAPI library is < 2.0.0 (API change upstream).

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
