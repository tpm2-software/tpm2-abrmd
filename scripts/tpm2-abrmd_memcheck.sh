#!/bin/sh

sudo -u tss \
G_DEBUG=fatal-criticals,gc-friendly \
valgrind --tool=memcheck --show-leak-kinds=definite,indirect \
         --leak-check=full \
./src/tpm2-abrmd --fail-on-loaded-trans --tcti=socket
