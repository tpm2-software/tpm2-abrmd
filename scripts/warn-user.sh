#!/bin/sh

# This is a quick script that parses the log files from 'make check' to
# extract the number of unit tests in each test binary. It keeps a running
# total of the tests dumping this count to stdout before exiting.

printf "\033[1;31m"
echo "***********************************************************"
echo "                         ATTENTION"
echo "If this test suite is executed against a TPM it may cause"
echo "damage to the TPM (NV storage and private key wear out etc)."
echo
echo "***********************************************************"
printf "\033[0m"
