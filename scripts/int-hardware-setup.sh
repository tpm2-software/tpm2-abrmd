#!/usr/bin/env bash
#;**********************************************************************;
# Copyright (c) 2017, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#;**********************************************************************;
set -u
set +o nounset

# default int-test-funcs script, overridden in TEST_FUNCTIONS env variable
TEST_FUNC_LIB=${TEST_FUNC_LIB:-scripts/int-test-funcs.sh}
if [ -e ${TEST_FUNC_LIB} ]; then
    . ${TEST_FUNC_LIB}
else
    echo "Error: Unable to locate support test function library: " \
         "${TEST_FUNC_LIB}"
    exit 1
fi

usage_error ()
{
    echo "$0: $*" >&2
    print_usage >&2
    exit 2
}
print_usage ()
{
    cat <<END
Usage:
    int-hardware-setup.sh --tabrmd-bin=FILE TEST-SCRIPT [TEST-SCRIPT-ARGUMENTS]
The '--tabrmd-bin' option is mandatory.
END
}
TABRMD_BIN=""
while test $# -gt 0; do
    case $1 in
    --help) print_usage; exit $?;;
    -r|--tabrmd-bin) TABRMD_BIN=$2; shift;;
    -r=*|--tabrmd-bin=*) TABRMD_BIN="${1#*=}";;
    --) shift; break;;
    -*) usage_error "invalid option: '$1'";;
     *) break;;
    esac
    shift
done

# Once option processing is done, $@ should be the name of the test executable
# followed by all of the options passed to the test executable.
TEST_BIN=$(realpath "$1")
TEST_DIR=$(dirname "$1")
TEST_NAME=$(basename "${TEST_BIN}")

# sanity tests
if [ `id -u` != "0" ]; then
    echo "need the root privilege to launch tabrmd for the integration test"
    exit 1
fi
if [ ! -x "${TABRMD_BIN}" ]; then
    echo "no tarbmd binary provided or not executable"
    exit 1
fi
if [ ! -x "${TEST_BIN}" ]; then
    echo "no test binary provided or not executable"
    exit 1
fi

# start an instance of the dbus-daemon for the test, have it use a random port
DBUS_LOG_FILE=${TEST_BIN}_dbus.log
DBUS_PID_FILE=${TEST_BIN}_dbus.pid
echo "Starting dbus-daemon"
dbus_daemon_start ${DBUS_LOG_FILE} ${DBUS_PID_FILE}
PID=$(cat ${DBUS_PID_FILE})
echo "dbus-daemon PID: ${PID}"
# start an instance of the tpm2-abrmd daemon for the test, use port from above
TABRMD_LOG_FILE=${TEST_BIN}_tabrmd.log
TABRMD_PID_FILE=${TEST_BIN}_tabrmd.pid
TABRMD_NAME=com.intel.tss2.Tabrmd${PID}
tabrmd_start ${TABRMD_BIN} 0 ${TABRMD_NAME} ${TABRMD_LOG_FILE} ${TABRMD_PID_FILE}
if [ $? -ne 0 ]; then
    echo "failed to start tabrmd with name ${TABRMD_NAME}"
fi

# execute the test script and capture exit code
env G_MESSAGES_DEBUG=all TABRMD_TEST_BUS_TYPE=session TABRMD_TEST_BUS_NAME="${TABRMD_NAME}" TABRMD_TEST_TCTI_RETRIES=10 $@
ret_test=$?

# teardown tabrmd
daemon_stop ${TABRMD_PID_FILE}
ret_tabrmd=$?
rm -rf ${TABRMD_PID_FILE}

# teardown dbus-daemon
daemon_stop ${DBUS_PID_FILE}
ret_dbus=$?
rm -rf ${DBUS_PID_FILE}

# handle exit codes
if [ $ret_test -ne 0 ]; then
    echo "Execution of $@ failed: $ret_test"
    exit $ret_test
fi
if [ $ret_tabrmd -ne 0 ]; then
    echo "Execution of tabrmd failed: $ret_tabrmd"
    exit $ret_tabrmd
fi
if [ $ret_dbus -ne 0 ]; then
    echo "Execution of dbus-daemon failed: $ret_dbus"
    exit $ret_dbus
fi

exit 0
