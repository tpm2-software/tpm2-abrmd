# Change Log
All notable changes to this project are documented in this file.

The format is based on [Keep a CHANGELOG](http://keepachangelog.com/)

## Unreleased
### Added
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
### Fixed
- Deconflict command line options: -t for TCTI selection, -a to fail if
transient objects are already loaded in the TPM.
- Clients can hold multiple TCTI connections again (fixed regression).
- Syslog log handler now only shows info & debug messages when
G_MESSAGES_DEBUG is set to 'all'.
