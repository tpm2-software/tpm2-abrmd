Here are quick set of instructions to build, install and run the intal's TPM2-abrmd.

## 1. System User & Group Account 

(create user and group called tss with root level access)
Our current configuration assumes that the name for this user and group is tss as per the norm established by the trousers TPM 1.2 software stack.
As it is common security practice we encourage everyone to run the tpm2-abrmd as an unprivileged user. This requires creating a user account and group to use for this purpose. Generally this is something your distro will do for you since the steps to create such an account can be distro-specific.



## 2. Dependencies

To build and install the tpm2-abrmd software the following dependencies are required:

- GNU Autoconf
- GNU Autoconf archive
- GNU Automake
- GNU Libtool
- C compiler
- C Library Development Libraries and Header Files (for pthreads headers)
- cmocka unit test framework (optional)
- pkg-config
- glib 2.0 library and development files

NOTE: Different GNU/Linux distros package glib-2.0 differently and so additional packages may be required. The tabrmd requires the GObject and GIO D-Bus support from glib-2.0 so please be sure you have whatever packages your distro provides are installed for these features.


## 3.Building From Source

clone the git repo

```
git clone https://github.com/01org/tpm2-abrmd.git

cd tpm2-abrmd
```
### a. Bootstrapping the build

When building the tpm2-abrmd source code from the upstream git repo you will need to first run the bootstrap script to setup the autotools build files:

```
./bootstrap
```
### b. Configuring the build

Next the build must be configured. You can define the environment for your build using a combination of the following options:

specifying a config.site file through the CONFIG_SITE environment variable
by manually exporting variables to the ./configure scripts environment
by passing parameters directly to the configure script.
NOTE: Invoking the configure script with the --help option will display all supported options.

```
./configure --with-dbuspolicydir=/etc/dbus-1/system.d --with-udevrulesdir=/etc/udev/rules.d/  (for debian/ raspbian distribution)
```
#### Note: you must integrate your distribution's  Dbus and Udev installation directory path to configure script.

##### for D-Bus: 
The tpm2-abrmd claims the name com.intel.tss2.Tabrmd on the D-Bus system bus. This requires policy to allow the user account running the daemon to be allowed to claim this name. The build installs such a configuration to ${sysconfdir}/dbus-1/system.d.

Depending on how you've configured the source tree and how your specific flavor of GNU/Linux configures D-Bus your system may not be reading this configuration file. Please consult the documentation for your distro and the location of the installed D-Bus policy to be sure it is in the right place.

The dbus-daemon will also need to be instructed to read this configuration file (assuming it's installed in a location consulted by dbus-daemon) before the policy will be in effect. Consult the D-Bus manual (aka DBUS-DAEMON(1)) for instructions.
 
it is located at ```/etc/dbus-1/system.d (for debian/ raspbian distribution)``` so we have ```--with-dbuspolicydir=/etc/dbus-1/system.d```

##### for udev : 
The typical operation for the tpm2-abrmd is for it to communicate directly with the Linux TPM driver using libtcti-device from the TPM2.0-TSS project. This requires that the user account that's running the tpm2-abrmd have both read and write access to the TPM device node /dev/tpm[0-9].

This requires that udev be instructed to set the owner and group for this device node. We provide such a udev rule that is installed to ${libdir}/udev/rules.d per GNU conventions.
 
it is located at ```/etc/udev/rules.d/ (for debian/ raspbian distribution)``` so we have ```--with-udevrulesdir=/etc/udev/rules.d/```

If your distro stores these rules elsewhere you will need to tell the build about this location using the configure script.

### c. Test Execution

Developers modifying / adding to the code, or adding new test cases may want to build and execute the unit and integration tests. To do so you must provide some combination of the following options to the configure script:

--enable-unit will ensure the cmocka framework is present. When the source code is configured with this option make check will execute the unit tests.
--with-simulatorbin is used to provide the build with a path to the tpm simulator. The simulator is required for the integration tests to function and so make check will only execute the integration tests if this option is provided. NOTE this must be an absolute path.
After this, make check will execute the unit and / or integration tests.

### d. Compilation

Compiling the code requires running make. You may provide make whatever parameters required for your environment (e.g. to enable parallel builds) but the defaults should be sufficient. The maintainers build enabling parallel builds like so:
```
$ make -j$(nproc)
```
### e. Installation

Once successfully built the tpm2-abrmd daemon can be installed using the command:
```
$ sudo make install
```
This will install the executable to locations determined at configure time. See the output of ./configure --help for the available options. Typically you won't need to do much more than provide an alternative --prefix option at configure time, and maybe DESTDIR at install time if you're packaging for a distro.

NOTE: This is the only command that should be run as root.


## 4. Verify installation is correct 

you should see tpm-udev.rules when you run (based on your distribution path)

```
ls /etc/udev/rules.d/ 
```
also Once you have this udev rule installed in the right place for your distro you'll need to instruct udev to reload its rules and apply the new rule. Typically this can be accomplished with the following command:

```
$ sudo udevadm control --reload-rules && sudo udevadm trigger
```
but if this doesn't work on your distro you please consult your distro's documentation.

## 5. Running the Daemon

Once you've accomplished all of the above, and assuming your system has a TPM2 device, you should be able to execute the tpm2-abrmd using the following command:

```
$ sudo -u tss tpm2-abrmd
```
This is something you'll only be doing on a development system though. A "real" system will want the daemon to start on boot.

## 6. Running the Systemd

Just like the configuration files for D-Bus and udev we provide a systemd unit for the tpm2-abrmd. Just like D-Bus and udev the location of unit files is distro specific and so you may need to either manually place this file or provide a path to the ./configure script by way of the --with-systedsystemunitdir option.

Once the provided unit file is in the right place for your distro you should be able to tell systemd to reload it's configuration:

```
$ systemctl daemon-reload &&
```
Once systemd has loaded the unit file you should be able to use systemctl to perform the start / stop / status operations as expected. Systemd should also now start the daemon when the system boots.
