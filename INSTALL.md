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
* glib 2.0 library and development files
* libsapi and TCTI libraries from https://github.com/01org/TPM2.0-TSS

**NOTE**: Different GNU/Linux distros package glib-2.0 differently and so
additional packages may be required. The tabrmd requires the GObject and
GIO D-Bus support from glib-2.0 so please be sure you have whatever packages
your distro provides are installed for these features.

The following dependencies are required only if the test suite is being built
and executed.
* cmocka unit test framework
* Microsoft / IBM Software TPM2 simulator version 532 as packaged by IBM:
https://downloads.sourceforge.net/project/ibmswtpm2/ibmtpm532.tar

# System User & Group
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

# Obtaining the Source Code
As is always the case, you should check for packages available through your
Linux distro before you attepmt to download and build the tpm2-abrmd from
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

### D-Bus Policy: `--with-dbuspolicydir`
The `tpm2-abrmd` claims a name on the D-Bus system bus. This requires policy
to allow the `tss` user account to claim this name. By default the build
installs this configuration file to `${sysconfdir}/dbus-1/system.d`. We allow
this to be overriden using the `--with-dbuspolicydir` option.

Using Debian (and it's various derivatives) as an example we can instruct the
build to install the dbus policy configuration in the right location with the
following configure option:
```
--with-dbuspolicydir=/etc/dbus-1/system.d
```

### Systemd Uint: `--with-systedsystemunitdir`
In most configurations the `tpm2-abrmd` daemon should be started as part of
the boot process. To enable this we provide a systemd unit. By default the
build installs this file to `${libdir}/systemd/system`. Just like D-Bus
the location of unit files is distro specific and so you may need to
configure the build to install this file in the appropriate location.

Again using Debian as an example we can instruct the build to install the
systemd unit in the right location with the following configure option:
```
--with-systedsystemunitdir=/lib/systemd/system
```

### udev Rules: `--with-udevrulesdir`
The typical operation for the `tpm2-abrmd` is for it to communicate directly
with the Linux TPM driver using `libtcti-device` from the TPM2.0-TSS project.
This requires that the user account that's running the `tpm2-abrmd` have both
read and write access to the TPM device node `/dev/tpm[0-9]`.

This requires that `udev` be instructed to set the owner and group for this
device node when its created. We provide such a udev rule that is installed to
`${libdir}/udev/rules.d`. If your distro stores these rules elsewhere you will
need to tell the build about this location.

Using Debian as an example we can instruct the build to install the udev
rules in the right location with the following configure option:
```
--with-udevrulesdir=/etc/udev/rules.d
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

### Enable Integration Tests: `--with-simulatorbin`
In order for the integration tests to be run the test harness must have access
to the TPM2 simulator software (see list of dependencies above). To execute
the integration tests you must download and compile the software simulator
as documented on their sourceforge site and their source packages.

Once you have the `tpm_server` built you can inform the tpm2-abrmd build of
its location by passing an absolute path to the `./configure` script through
the `--with-simulatorbin` option:
```
$ ./configure --with-simulatorbin=/path/to/tpm_server
```

If the configure script is able to find the executable you provide through this
option then executing `make check` will cause the integration tests to be built
and executed.

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

# Post-install
After installing the compiled software and configuration all components with
new configuration (Systemd, D-Bus and udev) must be prompted to reload their
configs. This can be accomplished by restarting your system but this isn't
strictly necessary and is generally considered bad form.

Instead each component can be instructed to reload its config manually. The
following sections describe this process for each.

## udev
Once you have this udev rule installed in the right place for your distro
you'll need to instruct udev to reload its rules and apply the new rule.
Typically this can be accomplished with the following command:
```
$ sudo udevadm control --reload-rules && sudo udevadm trigger
```

If this doesn't work on your distro please consult your distro's
documentation for UDEVADM(8).

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
