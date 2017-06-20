#!/usr/bin/env sh
#;**********************************************************************;
# Copyright (c) 2015, Intel Corporation
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
    int-log-compiler.sh --simulator-bin=FILE --tabrmd-bin=FILE
                        TEST-SCRIPT [TEST-SCRIPT-ARGUMENTS]
The '--simulator-bin' and '--tabrmd-bin' options are mandatory.
END
}
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

    env ${daemon_env} nohup ${daemon_bin} ${daemon_opts} > ${daemon_log_file} 2>&1 &
    local ret=$?
    local pid=$!
    if [ ${ret} -ne 0 ]; then
        echo "failed to start daemon: \"${daemon_bin}\" with env: \"${daemon_env}\""
        exit ${ret}
    fi
    if [ $(kill -0 "${pid}" 2> /dev/null) ]; then
        echo "daemon died after successfully starting in background, check " \
             "log file: ${daemon_log_file}"
        exit 1
    fi
    echo ${pid} > ${daemon_pid_file}
    disown ${pid}
    echo "successfully started daemon: ${daemon_bin} with PID: ${pid}"
}
# function to start the simulator
# This also that we have a private place to store the NVChip file. Since we
# can't tell the simulator what to name this file we must generate a random
# directory under /tmp, move to this directory, start the simulator, then
# return to the old pwd.
simulator_start ()
{
    local sim_bin="$1"
    local sim_port="$2"
    local sim_log_file="$3"
    local sim_pid_file="$4"
    local sim_tmp_dir="$5"
    # simulator port is a random port between 1024 and 65535

    cd ${sim_tmp_dir}
    daemon_start "${sim_bin}" "-port ${sim_port}" "${sim_log_file}" \
        "${sim_pid_file}" ""
    cd -
}
# function to start the tabrmd
# This is little more than a call to the daemon_start function with special
# command line options and an environment string.
tabrmd_start ()
{
    local tabrmd_bin=$1
    local tabrmd_port=$2
    local tabrmd_name=$3
    local tabrmd_log_file=$4
    local tabrmd_pid_file=$5

    local tabrmd_env="G_MESSAGES_DEBUG=all"
    local tabrmd_opts="--tcti=socket --tcti-socket-port=${tabrmd_port} --session --dbus-name=${tabrmd_name} --fail-on-loaded-trans"

    daemon_start "${tabrmd_bin}" "${tabrmd_opts}" "${tabrmd_log_file}" \
        "${tabrmd_pid_file}" "${tabrmd_env}"
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
    kill -0 ${pid}
    ret=$?
    if [ ${ret} -ne 0 ]; then
        echo "failed to detect running daemon with PID: ${pid}";
        return ${ret}
    fi
    kill ${pid}
    ret=$?
    if [ ${ret} -ne 0 ]; then
        echo "failed to kill daemon process with PID: ${pid}"
    fi
    return ${ret}
}

# Once option processing is done, $@ should be the name of the test executable
# followed by all of the options passed to the test executable.
TEST_BIN=$(realpath "$1")
TEST_DIR=$(dirname "$1")
TEST_NAME=$(basename "${TEST_BIN}")

# start an instance of the simulator for the test, have it use a random port
SIM_LOG_FILE=${TEST_BIN}_simulator.log
SIM_PID_FILE=${TEST_BIN}_simulator.pid
SIM_TMP_DIR=$(mktemp --directory --tmpdir=/tmp tpm_server_XXXXXX)
SIM_PORT=$(awk -v min=1024 -v max=65535 'BEGIN{srand(); print int(min+rand()*(max-min+1))}')
simulator_start ${SIM_BIN} ${SIM_PORT} ${SIM_LOG_FILE} ${SIM_PID_FILE} ${SIM_TMP_DIR}

# start an instance of the tpm2-abrmd daemon for the test, use port from above
TABRMD_LOG_FILE=${TEST_BIN}_tabrmd.log
TABRMD_PID_FILE=${TEST_BIN}_tabrmd.pid
TABRMD_NAME=com.intel.tss2.Tabrmd${SIM_PORT}
tabrmd_start ${TABRMD_BIN} ${SIM_PORT} ${TABRMD_NAME} ${TABRMD_LOG_FILE} ${TABRMD_PID_FILE}

# execute the test script
env G_MESSAGES_DEBUG=all TABRMD_TEST_BUS_TYPE=session TABRMD_TEST_BUS_NAME="${TABRMD_NAME}" $@
ret=$?

# This sleep is sadly necessary: If we kill the tabrmd w/o sleeping for a
# second after the test finishes the simulator will die too. Bug in the
# simulator?
sleep 1
# handle response code
# teardown
daemon_stop ${TABRMD_PID_FILE}
rm -rf ${TABRMD_PID_FILE}
daemon_stop ${SIM_PID_FILE}
rm -rf ${SIM_TMP_DIR} ${SIM_PID_FILE}
exit $ret
