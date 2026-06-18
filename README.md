![Linux Build Status](https://github.com/tpm2-software/tpm2-abrmd/workflows/Linux%20Build%20Status/badge.svg)
![FreeBSD Build Status](https://github.com/tpm2-software/tpm2-abrmd/workflows/FreeBSD%20Build%20Status/badge.svg)
[![Coverity Scan](https://img.shields.io/coverity/scan/3997.svg)](https://scan.coverity.com/projects/01org-tpm2-abrmd)
[![codecov](https://codecov.io/gh/tpm2-software/tpm2-abrmd/branch/master/graph/badge.svg)](https://codecov.io/gh/tpm2-software/tpm2-abrmd)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/tpm2-software/tpm2-abrmd.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/tpm2-software/tpm2-abrmd/context:cpp)

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
It is intended for use with the SAPI library (libtss2-sapi) like any other
TCTI. The initialization function for this library is hard coded to connect to
the tabrmd on the system bus as this is the most common configuration.

Check out the man page TSS2-TCTI-TABRMD(7) and TSS2_TCTI_TABRMD_INIT(3).

## tpm2-abrmd vs in-kernel RM
The current implementations are mostly equivalent with a few differences.
Both provide isolation between objects & sessions created by different
connections which is the core functionality required by applications. The
reason we have both is that the in-kernel RM was added in version 4.12 and
we have TPM2 users in environments with kernels going back to the
3.x series. So the user space RM will be around to support those users.

# Related Specifications
* [TPM2 Software Stack Access Broker and Resource Manager](https://trustedcomputinggroup.org/wp-content/uploads/TSS-TAB-and-Resource-Manager-ver1.0-rev16_Public_Review.pdf)
* [TPM2 Software Stack System API and TPM2 Command Transmission Interface](http://www.trustedcomputinggroup.org/wp-content/uploads/TSS-system-API-01.pdf)
