[![Build Status](https://travis-ci.org/01org/tpm2-abrmd.svg?branch=master)](https://travis-ci.org/01org/tpm2-abrmd)

# TPM2 Access Broker & Resource Manager
This is a system daemon implementing the TPM2 access broker (TAB) & Resource
Manager (RM) spec from the TCG. The daemon (tpm2-abrmd) is implemented using
Glib and the GObject system. In this documentation and in the code we use
`tpm2-abrmd` and `tabrmd` interchangeably.

## Build & Install
Instructions to build and install this software are available in the
[INSTALL.md](INSTALL.md) file.

## tpm2-abrmd
`tpm2-abrmd` is a daemon. It should be started as part of the OS boot process.
Communication between the daemon and clients using the TPM is done with a
combination of DBus and Unix pipes. DBus is used for discovery, session
management and the 'cancel', 'setLocality', and 'getPollHandles' API calls
(mostly these aren't yet implemented). Pipes are used to send and receive
TPM commands and responses (respectively) between client and server.

The daemon owns the com.intel.tss2.Tabrmd name on dbus. It can be configured
to connect to either the system or the session bus. Configuring name
selection would be a handy feature but that's future work.

Check out the man page TPM2-ABRMD(8) for the currently supported options.

## libtcti-tabrmd
This repository also hosts a client library for interacting with this daemon.
It is intended for use with the SAPI library (libsapi) like any other TCTI.
The initialization function for this library is hard coded to connect to the
tabrmd on the system bus as this is the most common configuration.

Check out the man page TCTI-TABRMD(7) and TSS2_TCTI_TABRMD_INIT(3).

# Related Specifications
* [TPM2 Software Stack Access Broker and Resource Manager](http://www.trustedcomputinggroup.org/wp-content/uploads/TSS-TAB-and-Resource-Manager-00-91-PublicReview.pdf)
* [TPM2 Software Stack System API and TPM2 Command Transmission Interface](http://www.trustedcomputinggroup.org/wp-content/uploads/TSS-system-API-01.pdf)
