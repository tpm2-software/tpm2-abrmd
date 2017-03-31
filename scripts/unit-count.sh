#!/bin/sh

# This is a quick script that parses the log files from 'make check' to
# extract the number of unit tests in each test binary. It keeps a running
# total of the tests dumping this count to stdout before exiting.

ls -1 test/*.log |
{
    VAL=0;
    while read FILE; do
        TMP=$(sed -e 's&^.*[[:space:]]\+\([0-9]\+\)[[:space:]]\+test(s) run\.$&\1&;tx;d;:x' ${FILE});
        VAL=$(expr "${VAL}" + "${TMP}");
    done;
    echo "Total number of unit tests: ${VAL}"
}
