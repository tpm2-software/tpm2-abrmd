# SPDX-License-Identifier: BSD-2

set -exo pipefail

pushd "$1"

if [ -z "$TPM2TSS_BRANCH" ]; then
    echo "TPM2TSS_BRANCH is unset, please specify TPM2TSS_BRANCH"
    exit 1
fi

# Install tpm2-tss
if [ ! -d tpm2-tss ]; then

  git clone --depth=1 -b "${TPM2TSS_BRANCH}" "https://github.com/tpm2-software/tpm2-tss.git"
  pushd tpm2-tss
  ./bootstrap
  ./configure --enable-debug --disable-esys --disable-esapi --disable-fapi
  make -j$(nproc)
  make install
  popd
else
  echo "tpm2-tss already installed, skipping..."
fi

popd

exit 0
