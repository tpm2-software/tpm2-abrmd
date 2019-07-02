This is a quick set of instructions to build, install and run the tpm2-abrmd.

# Dependencies
To build and install the tpm2-abrmd software the following dependencies are
required:
* GNU Autoconf
* GNU Autoconf archive
* GNU Automake
* GNU Libtool
* C compiler
* C Library Development Libraries and Header Files (for pthreads headers)
* pkg-config
* glib and gio 2.0 libraries and development files
* libtss2-sys, libtss2-mu and TCTI libraries from https://github.com/tpm2-software/tpm2-tss

**NOTE**: Different GNU/Linux distros package glib-2.0 differently and so
additional packages may be required. The tabrmd requires the GObject and
GIO D-Bus support from glib-2.0 so please be sure you have whatever packages
your distro provides are installed for these features.

The following dependencies are required only if the test suite is being built
and executed.
* cmocka unit test framework
* Microsoft / IBM Software TPM2 simulator version 974 as packaged by IBM
for Linux:
https://downloads.sourceforge.net/project/ibmswtpm2/ibmtpm974.tar
* Alternately, run the test suite on a real TPM hardware, with a safety
attention described below.

# System User & Group
`tpm2-abrmd` must run as user `tss` or `root`.
As is common security practice we encourage *everyone* to run the `tpm2-abrmd`
as an unprivileged user. This requires creating a user account and group to
use for this purpose. Our current configuration assumes that the name for this
user and group is `tss` per the norm established by the `trousers` TPM 1.2
software stack.

This account and the associated group must be created before running the
daemon. The following command should be sufficient to create the `tss`
account:
```
$ sudo useradd --system --user-group tss
```

You may wish to further restrict this user account based on your needs. This
topic however is beyond the scope of this document.

To run tpm2-abrmd as root, which is not recommended, use the `--allow-root`
option.

# Obtaining the Source Code
As is always the case, you should check for packages available through your
Linux distro before you attempt to download and build the tpm2-abrmd from
source code directly. If you need a newer version than provided by your
Distro of choice then you should download our latest stable release here:
https://github.com/01org/tpm2-abrmd/releases/latest.

The latest unstable development work can be obtained from the Git VCS here:
https://github.com/01org/tpm2-abrmd.git. This method should be used only by
developers and should be assumed to be unstable.

The remainder of this document assumes that you have:
* obtained the tpm2-abmrd source code using a method described above
* extracted the source code if necessary
* set your current working directory to be the root of the tpm2-abrmd source
tree

## Bootstrap the Build (git only)
If you're looking to contribute to the project then you will need to build
from the project's git repository. Building from git requires some additional
work to "bootstrap" the autotools machinery. This is accomplished by
executing the `bootstrap script:
```
$ ./bootstrap
```

If you're building from a release source tarball you should skip this step.

## Configure the Build
The source code for must be configured before the tpm2-abrmd can be built. In
the most simple case you may run the `configure script without any options:
```
$ ./configure
```

If your system is capable of compiling the source code then the `configure`
script will exit with a status code of `0`. Otherwise an error code will be
returned.

### Custom `./configure` Options
In many cases you'll need to provide the `./configure` script with additional
information about your environment. Typically you'll either be telling the
script about some location to install a component, or you'll be instructing
the script to enable some additional feature or function. We'll cover each
in turn.

Invoking the configure script with the `--help` option will display
all supported options.

The default values for GNU installation directories are documented here:
https://www.gnu.org/prep/standards/html_node/Directory-Variables.html

### D-Bus Policy: `--with-dbuspolicydir`
The `tpm2-abrmd` claims a name on the D-Bus system bus. This requires policy
to allow the `tss` user account to claim this name. By default the build
installs this configuration file to `${sysconfdir}/dbus-1/system.d`. We allow
this to be overridden using the `--with-dbuspolicydir` option.

Using Debian (and it's various derivatives) as an example we can instruct the
build to install the dbus policy configuration in the right location with the
following configure option:
```
--with-dbuspolicydir=/etc/dbus-1/system.d
```

### Systemd
In most configurations the `tpm2-abrmd` daemon should be started as part of
the boot process. To enable this we provide a systemd unit as well as a
systemd preset file.

#### Systemd Uint: `--with-systemdsystemunitdir`
By default the build installs this file to `${libdir}/systemd/system. Just
like D-Bus the location of unit files is distro specific and so you may need
to configure the build to install this file in the appropriate location.

Again using Debian as an example we can instruct the build to install the
systemd unit in the right location with the following configure option:
```
--with-systemdsystemunitdir=/lib/systemd/system
```

#### Systemd Preset Dir: `--with-systemdpresetdir=DIR`
By default the build installs the systemd preset file for the tabrmd to
`${libdir}/systemd/system-preset`. If you need to install this file to a
different directory pass the desired path to the `configure` script using this
option. For example:
```
--with-systemdpresetdir=/lib/systemd/system-preset
```

#### Systemd Preset Default: `--with-systemdpresetdisable`
The systemd preset file will enable the tabrmd by default, causing it to be
started by systemd on boot. If you wish for the daemon to be disabled by
default some reason you may use this option to the `configure` script to do
so.


#### `--datarootdir`
To override the system data directory, used for
${datadir}/dbus-1/system-services/com.intel.tss2.Tabrmd.service,
use the `--datarootdir` option.
Using Debian as an example we can instruct the build to install the
DBUS service files in the right location with the following configure option:
```
--datarootdir=/usr/share
```

### Enable Unit Tests: `--enable-unit`
When provided to the `./configure` script this option will attempt to detect
whether or not the cmocka unit testing library is installed. If not then the
configure step will fail. If it is available then the unit tests will be
enabled in the build and they will be built and executed as part of the test
harness:
```
$ ./configure --enable-unit
```

If the `./configure` script finds the cmocka framework then executing `make
check` will cause the unit tests to be built and executed.

### Integration Tests:
In addition to unit tests we provide a collection of integration tests.
Integration tests differ from unit tests in that they require a running
instance of the `tpm2-abrmd` daemon. Integration tests exercise a specific
behavior of the `tpm2-abrmd` and ensure that the daemon behaves as expected.

In order for the `tpm2-abrmd` daemon to run it must have a TPM2 device to
talk to. This can be done in two ways:
1) Using an implementation of the TPM2 in software the TPM2 device can be
simulated. This is the most flexible, fast and safest way to test the
`tpm2-abrmd`.
2) Using the TPM2 device on your computer (if it has one). This method is
*NOT* recommended and may cause damage / wear-out of the TPM and its
resources. This option is provided for developers and integrators who know
what they're doing and who understand the associated risks: You have been
warned.

The next two subsections describe these two configurations:

### Run Integration Tests: `--enable-integration`
If the configure script is passed the `--enable-integration` option then the
test harness will execute the integration tests against the software TPM2
simulator. The configure script will check for the existance of the software
TPM2 simulator executable `tpm_server` on the PATH. An instance of the
simulator will be created for each test executable to allow the parallel
execution of test cases.

This is the recommended integration test configuration. It requires that you
first download and compile the TPM2 software simulator as documented by the
simulators maintainers. Once you have the `tpm_server` built, you must ensure
that it is discoverable via the PATH environment variable when the
`./configure` script is run.

**NOTE**: The `--with-simulatorbin` option does not change the default for
tpm2-abrmd, which is to use TPM hardware.
To run tpm2-abrmd with the simulator, use:
```
$ sudo -u tss /usr/local/sbin/tpm2-abrmd --tcti=mssim
```

### Run Integration Tests with hardware TPM2: `--enable-test-hwtpm`
It's possible to run the integration tests against "real" TPM2 hardware, not
just against the simulator. This does however come with some restrictions.
Firstly your TPM2 hardware must be properly initialized by your platform and
the kernel properly configured to load the appropriate driver. How to do all
of this is beyond the scope of this document and so we assume the reader has
already done this.

Once your TPM2 hardware is all set up you may configure tpm2-abrmd build to
run the integration tests against the TPM2 device by passing the
`./configure` script the `--enable-test-hwtpm` option:
```
$ ./configure --enable-test-hwtpm
```
Providing this option will enable the integration tests much like the
`--enable-integration` but the configure script will not check for the
simulator executable.

The test harness can then be run with the typical `make check` though there
are some limitations due to the use of the hardware TPM:
1) The test harness does not support parallel execution of tests in this
configuration. When each test is executed a fresh instance of the daemon is
started and since there's only one instance of the TPM2 device.
2) The tests must be executed as a privileged user. This is required as the
daemon will interact with the TPM2 device through the `/dev/tpm0` device node
and access to it typically requires elevated privileges. The easiest approach,
which is also the most risky, is to run tests in this configuration as the
`root` user.
3) To simplify the test harness in this configuration the tpm2-abrmd is run
on the DBus system bus (it's run on the session bus when tests are executed
against the simulator) and the default DBus name for the daemon
(com.intel.tss2.Tabrmd) is used. This implies that any existing instance of
the daemon that may have been started when your system boots *must* be stopped
before the test harness can run tests against your TPM hardware.

Finally executing tests against real TPM2 hardware is exponentially slower
than running them against the software simulator. People often refer to the
TPM2 as a "crypto decelerator" for a reason :) As such we recommend that tests
be executed individually until you're confident your configuration works as
expected. An example follows:
```
sudo make check TESTS=test/integration/auth-session-start-flush.int
```

This example runs only the `auth-session-start-flush.int` test which isn't
particularly compute-intensive since it doesn't create any keys or carry out
any signing or encryption operations other than those done by the TPM2
device as part of its session management operations.

*WARNING*: If this test suite is executed against a TPM2 it may result in the
TPM2 device being damaged or destroyed. You have been warned ... again.

# Compilation
Compiling the code requires running `make`. You may provide `make` whatever
parameters required for your environment (e.g. to enable parallel builds) but
the defaults should be sufficient:
```
$ make
```

# Installation
Once successfully built the `tpm2-abrmd` daemon it should be installed using
the command:
```
$ sudo make install
```

This will install the executable to locations determined at configure time.
See the output of `./configure --help` for the available options. Typically
you won't need to do much more than provide an alternative `--prefix` option
at configure time, and maybe `DESTDIR` at install time if you're packaging
for a distro.

**NOTE**: It may be necessary to run ldconfig (as root) to update the run-time
bindings before executing a program that links against the tabrmd library:
```
$ sudo ldconfig
```

# Post-install
After installing the compiled software and configuration all components with
new configuration (Systemd and D-Bus) must be prompted to reload their configs.
This can be accomplished by restarting your system but this isn't strictly
necessary and is generally considered bad form.

Instead each component can be instructed to reload its config manually. The
following sections describe this process for each.

## D-Bus
The dbus-daemon will also need to be instructed to read this configuration
file (assuming it's installed in a location consulted by dbus-daemon) before
the policy will be in effect. This is typically accomplished by sending the
`dbus-daemon` the `HUP` signal like so:
```
$ sudo pkill -HUP dbus-daemon
```

If this doesn't work on your distro please consult your distro's documentation
for DBUS-DAEMON(1).

## Systemd
Assuming that the `tpm2-abrmd` unit was installed in the correct location for
your distro Systemd must be instructed to reload it's configuration. This is
accomplished with the following command:
```
$ systemctl daemon-reload
```

Once systemd has loaded the unit file you should be able to use `systemctl`
to perform the start / stop / status operations as expected. Systemd should
also now start the daemon when the system boots.

## SELinux
On systems using SELinux (for example, check with the `getenforce` utility) the
default service policy is too restrictive. The repository contains an SELinux
policy module which, when applied as below, allows the daemon to operate on
systems with SELinux in enforcing mode.

First, ensure SELinux policy development files are installed (i.e on Fedora they
can be installed with `dnf install selinux-policy-devel`).
Then build and install the policy module:

```
$ pushd selinux
$ make -f /usr/share/selinux/devel/Makefile tabrmd.pp
$ sudo semodule -i tabrmd.pp
$ sudo restorecon /path/to/installed/tpm2-abrmd
```

Once the module is installed you can ensure the daemon is correctly labelled by
checking with:

```
$ ls -Z /path/to/installed/tpm2-abrmd
system_u:object_r:tabrmd_exec_t:s0 /usr/local/sbin/tpm2-abrmd
```

## Sanity Tests
Once the `tpm2-abrmd` installed and running it's possible to query the
dbus-daemon to see evidence of its presence. The following command will list
all names on the system bus. We filter for the expected name to keep the
output manageable:
```
dbus-send --system --dest=org.freedesktop.DBus --type=method_call --print-reply /org/freedesktop/DBus org.freedesktop.DBus.ListNames
```
that should return output like:
```
method return time=1524690245.245721 sender=org.freedesktop.DBus -> destination=:1.190 serial=3 reply_serial=2
   array [
      ...
      string "com.intel.tss2.Tabrmd"
      ...
   ]
```
This lets us know that the daemon has claimed the expected name on the system
bus, but for many this isn't enough. How do we know that the daemon isn't hung
/ unresponsive? The most simple way to do this is to use the 'introspection'
interface to query the daemon for the functions / methods that it supports.
Every dbus daemon supports this interface:
```
dbus-send --system --dest=com.intel.tss2.Tabrmd --type=method_call --print-reply /com/intel/tss2/Tabrmd/Tcti org.freedesktop.DBus.Introspectable.Introspect
```
that should return output like:
```
method return time=1524690897.749245 sender=:1.192 -> destination=:1.193 serial=7 reply_serial=2
   string "<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
                      "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<!-- GDBus 2.50.3 -->
<node>
  <interface name="org.freedesktop.DBus.Properties">
    <method name="Get">
      <arg type="s" name="interface_name" direction="in"/>
      <arg type="s" name="property_name" direction="in"/>
      <arg type="v" name="value" direction="out"/>
    </method>
    <method name="GetAll">
      <arg type="s" name="interface_name" direction="in"/>
      <arg type="a{sv}" name="properties" direction="out"/>
    </method>
    <method name="Set">
      <arg type="s" name="interface_name" direction="in"/>
      <arg type="s" name="property_name" direction="in"/>
      <arg type="v" name="value" direction="in"/>
    </method>
    <signal name="PropertiesChanged">
      <arg type="s" name="interface_name"/>
      <arg type="a{sv}" name="changed_properties"/>
      <arg type="as" name="invalidated_properties"/>
    </signal>
  </interface>
  <interface name="org.freedesktop.DBus.Introspectable">
    <method name="Introspect">
      <arg type="s" name="xml_data" direction="out"/>
    </method>
  </interface>
  <interface name="org.freedesktop.DBus.Peer">
    <method name="Ping"/>
    <method name="GetMachineId">
      <arg type="s" name="machine_uuid" direction="out"/>
    </method>
  </interface>
  <interface name="com.intel.tss2.TctiTabrmd">
    <method name="CreateConnection">
      <arg type="ah" name="fds" direction="out"/>
      <arg type="t" name="id" direction="out"/>
    </method>
    <method name="Cancel">
      <arg type="t" name="id" direction="in"/>
      <arg type="u" name="return_code" direction="out"/>
    </method>
    <method name="SetLocality">
      <arg type="t" name="id" direction="in"/>
      <arg type="y" name="locality" direction="in"/>
      <arg type="u" name="return_code" direction="out"/>
    </method>
  </interface>
</node>
"
```
