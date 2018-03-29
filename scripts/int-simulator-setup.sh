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

# default int-test-funcs script, overriden in TEST_FUNCTIONS env variable
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
    int-simulator-setup.sh --simulator-bin=FILE --tabrmd-bin=FILE
        TEST-SCRIPT [TEST-SCRIPT-ARGUMENTS]
The '--simulator-bin' and '--tabrmd-bin' options are mandatory.
END
}
SIM_BIN=""
TABRMD_BIN=""
while test $# -gt 0; do
    case $1 in
    --help) print_usage; exit $?;;
    -s|--simulator-bin) SIM_BIN=$2; shift;;
    -s=*|--simulator-bin=*) SIM_BIN="${1#*=}";;
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
if [ ! -x "${SIM_BIN}" ]; then
    echo "no simulator binary provided or not executable"
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

# start an instance of the simulator for the test, have it use a random port
SIM_LOG_FILE=${TEST_BIN}_simulator.log
SIM_PID_FILE=${TEST_BIN}_simulator.pid
SIM_TMP_DIR=$(mktemp --directory --tmpdir=/tmp tpm_server_XXXXXX)
PORT_MIN=1024
PORT_MAX=65534
BACKOFF_FACTOR=2
BACKOFF=1
for i in $(seq 10); do
    SIM_PORT_DATA=$(od -A n -N 2 -t u2 /dev/urandom | awk -v min=${PORT_MIN} -v max=${PORT_MAX} '{print ($1 % (max - min)) + min}')
    SIM_PORT_CMD=$((${SIM_PORT_DATA}+1))
    echo "Starting simulator on port ${SIM_PORT_DATA}"
    simulator_start ${SIM_BIN} ${SIM_PORT_DATA} ${SIM_LOG_FILE} ${SIM_PID_FILE} ${SIM_TMP_DIR}
    sleep 1 # give daemon time to bind to ports
    PID=$(cat ${SIM_PID_FILE})
    echo "simulator PID: ${PID}";
    netstat -ltpn 2> /dev/null | grep "${PID}" | grep -q "${SIM_PORT_DATA}"
    ret_data=$?
    netstat -ltpn 2> /dev/null | grep "${PID}" | grep -q "${SIM_PORT_CMD}"
    ret_cmd=$?
    if [ \( $ret_data -eq 0 \) -a \( $ret_cmd -eq 0 \) ]; then
        echo "Simulator with PID ${PID} bound to port ${SIM_PORT_DATA} and " \
             "${SIM_PORT_CMD} successfully.";
        break
    fi
    echo "Port conflict? Cleaning up PID: ${PID}"
    kill "${PID}"
    BACKOFF=$((${BACKOFF}*${BACKOFF_FACTOR}))
    echo "Failed to start simulator: port ${SIM_PORT_DATA} or " \
         "${SIM_PORT_CMD} probably in use. Retrying in ${BACKOFF}."
    sleep ${BACKOFF}
    if [ $i -eq 10 ]; then
        echo "Failed to start simulator after $i tries. Giving up.";
        exit 1
    fi
done
# start an instance of the tpm2-abrmd daemon for the test, use port from above
TABRMD_LOG_FILE=${TEST_BIN}_tabrmd.log
TABRMD_PID_FILE=${TEST_BIN}_tabrmd.pid
TABRMD_NAME=com.intel.tss2.Tabrmd${SIM_PORT_DATA}
tabrmd_start ${TABRMD_BIN} ${SIM_PORT_DATA} ${TABRMD_NAME} ${TABRMD_LOG_FILE} ${TABRMD_PID_FILE}
if [ $? -ne 0 ]; then
    echo "failed to start tabrmd with name ${TABRMD_NAME}"
fi

# execute the test script and capture exit code
env G_MESSAGES_DEBUG=all TABRMD_TEST_TCTI_CONF="bus_name=${TABRMD_NAME},bus_type=session" TABRMD_TEST_TCTI_RETRIES=10 $@
ret_test=$?

# This sleep is sadly necessary: If we kill the tabrmd w/o sleeping for a
# second after the test finishes the simulator will die too. Bug in the
# simulator?
sleep 1

# teardown tabrmd
daemon_stop ${TABRMD_PID_FILE}
ret_tabrmd=$?
rm -rf ${TABRMD_PID_FILE}

# teardown simulator, ignore exit code (it's 143?)
daemon_stop ${SIM_PID_FILE}
rm -rf ${SIM_TMP_DIR} ${SIM_PID_FILE}

# handle exit codes
if [ $ret_test -ne 0 ]; then
    echo "Execution of $@ failed: $ret_test"
    exit $ret_test
fi
if [ $ret_tabrmd -ne 0 ]; then
    echo "Execution of tabrmd failed: $ret_tabrmd"
    exit $ret_tabrmd
fi

exit 0
