# SPDX-License-Identifier: BSD-2

set -exo pipefail

pushd "$1"
TSS_VERSION=`git ls-remote --refs --tags https://github.com/tpm2-software/tpm2-tss.git | cut --delimiter='/' --fields=3 | tail --lines=1`

if [ -z "$TPM2TSS_BRANCH" ]; then
    echo "TPM2TSS_BRANCH is unset, please specify TPM2TSS_BRANCH"
    exit 1
fi

# Install tpm2-tss
if [ ! -d tpm2-tss ]; then
  if [ -z "$GIT_FULL_CLONE" ]; then
		echo "Doing shallow clone of tss"
		git_extra_flags="--depth=1"
	else
		echo "Doing deep clone of tss"
	fi
  git clone $git_extra_flags -b "${TSS_VERSION}" "https://github.com/tpm2-software/tpm2-tss.git"
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
