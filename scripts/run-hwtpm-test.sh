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
    run-hwtpm-test.sh --tabrmd-bin=FILE
                        TEST-SCRIPT [TEST-SCRIPT-ARGUMENTS]
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

# This function takes a PID as a parameter and determines whether or not the
# process is currently running. If the daemon is running 0 is returned. Any
# other value indicates that the daemon isn't running.
daemon_status ()
{
    local pid=$1

    if [ $(kill -0 "${pid}" 2> /dev/null) ]; then
        echo "failed to detect running daemon with PID: ${pid}";
        return 1
    fi
    return 0
}

# This is a generic function to start a daemon, setup the environment
# variables, redirect output to a log file, store the PID of the daemon
# in a file and disconnect the daemon from the parent shell.
daemon_start ()
{
    local daemon_bin="$1"
    local daemon_opts="$2"
    local daemon_log_file="$3"
    local daemon_pid_file="$4"
    local daemon_env="$5"
    local valgrind_bin="$6"
    local valgrind_flags="$7"

    env ${daemon_env} ${valgrind_bin} ${valgrind_flags} ${daemon_bin} ${daemon_opts} > ${daemon_log_file} 2>&1 &
    local ret=$?
    local pid=$!
    if [ ${ret} -ne 0 ]; then
        echo "failed to start daemon: \"${daemon_bin}\" with env: \"${daemon_env}\""
        exit ${ret}
    fi
    sleep 1
    daemon_status "${pid}"
    if [ $? -ne 0 ]; then
        echo "daemon died after successfully starting in background, check " \
             "log file: ${daemon_log_file}"
        return 1
    fi
    echo ${pid} > ${daemon_pid_file}
    echo "successfully started daemon: ${daemon_bin} with PID: ${pid}"
    return 0
}
# function to start the dbus-daemon
# This dbus-daemon creates a session message bus used by the
# communication between tpm2-abrmd and testcase. The dbus info
# is told to testcase through DBUS_SESSION_BUS_ADDRESS and
# DBUS_SESSION_BUS_PID.
dbus_daemon_start ()
{
    local dbus_log_file="$1"
    local dbus_pid_file="$2"
    local dbus_opts="--session --print-address 3 --nofork --nopidfile"
    local dbus_addr_file=`mktemp`
    local dbus_env="DBUS_VERBOSE=1" 

    exec 3<>$dbus_addr_file
    daemon_start dbus-daemon "${dbus_opts}" "${dbus_log_file}" "${dbus_pid_file}" \
        "${dbus_env}"
    local ret=$?
    if [ $ret -eq 0 ]; then
        export DBUS_SESSION_BUS_ADDRESS=`cat "${dbus_addr_file}"`
        export DBUS_SESSION_BUS_PID=`cat "${dbus_pid_file}"`
    fi
    rm -f $dbus_addr_file
    return $ret
}
# function to start the tabrmd
# This is little more than a call to the daemon_start function with special
# command line options and an environment string.
tabrmd_start ()
{
    local tabrmd_bin=$1
    local tabrmd_name=$2
    local tabrmd_log_file=$3
    local tabrmd_pid_file=$4
    local tabrmd_env="G_MESSAGES_DEBUG=all"
    local tabrmd_opts="--tcti=device --session --dbus-name=${tabrmd_name} --fail-on-loaded-trans"
    
    daemon_start "${tabrmd_bin}" "${tabrmd_opts}" "${tabrmd_log_file}" \
        "${tabrmd_pid_file}" "${tabrmd_env}" "${VALGRIND}" "${LOG_FLAGS}"
}
# function to stop a running daemon
# This function takes a single parameter: a file containing the PID of the
# process to be killed. The PID is extracted and the daemon killed.
daemon_stop ()
{
    local pid_file=$1
    local pid=0
    local ret=0

    if [ ! -f ${pid_file} ]; then
        echo "failed to stop daemon, no pid file: ${pid_file}"
        return 1
    fi
    pid=$(cat ${pid_file})
    daemon_status "${pid}"
    if [ $? -ne 0 ]; then
        echo "failed to detect running daemon with PID: ${pid}";
        return ${ret}
    fi
    kill ${pid}
    ret=$?
    if [ ${ret} -eq 0 ]; then
        wait ${pid}
        ret=$?
    else
        echo "failed to kill daemon process with PID: ${pid}"
    fi
    return ${ret}
}

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
tabrmd_start ${TABRMD_BIN} ${TABRMD_NAME} ${TABRMD_LOG_FILE} ${TABRMD_PID_FILE}
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
