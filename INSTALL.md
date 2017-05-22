To build and install the tpm2-abrmd software the following dependencies are
required:
GNU Autoconf
GNU Autoconf archive
GNU Automake
GNU Libtool
C compiler
C Library Development Libraries and Header Files (for pthreads headers)
cmocka unit test framework
pkg-config
glib 2.0 library and development files

When building the tpm2-abrmd source code from the upstream git repo you will
need to first run the 'bootstrap' script to setup the autotools build files:
$ ./bootstrap
This setp is not required when building from a release tarball.

Next the build must be configured. You may optionally define the environment
for you build using a config.site file. The default for the project is kept
at ./lib/default_config.site. Pass this environment to the build through the
CONFIG_SITE variable:
$ ./bootstrap
$ CONFIG_SITE=$(pwd)/lib/default_config.site ./configure

You may also customize the config.site to your needs (please read the GNU
documentation for config.site files) or use your platform / distro default
by leaving the CONFIG_SITE environment variable undefined.

Then compile the code:
$ make

Once successfully built the tpm2-abrmd daemon can be installed with:
$ sudo make install

This will install the executable to locations determined at configure time.
See the output of ./configure --help for the available options. Typically
you won't need to do much more than provide an alternative --prefix option
at configure time, and maybe DESTDIR at install time if you're packaging
for a distro.
