#!/bin/sh

# to run this from $(srcdir) you'll probably have to set
# PATH=$(srcdir)/test/integration/:${PATH}

# create 5 instances 
LOOPS=${1:-5}
for i in $(seq 1 ${LOOPS}); do
    # 
    create-keys > test/integration/tpm2-key-hierarchy_$(printf "%04d" ${i}).log 2>&1 &
done

# wait for all background jobs to finish
wait
