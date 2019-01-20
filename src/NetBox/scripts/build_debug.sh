#! /bin/bash

set -eo pipefail

setup_netbox() {
  if [ -z "$NETBOX_ROOT_DIR" ] ; then
    NETBOX_ROOT_DIR=$(readlink -f "$PWD/..")
  fi
  if [ -z "$NETBOX_BUILD_DIR" ] ; then
    NETBOX_BUILD_DIR="$NETBOX_ROOT_DIR/../../../NetBox-VS2017x64-$BUILD_TYPE"
  fi

  local BUILD_DIR="$NETBOX_BUILD_DIR"
  mkdir -p $BUILD_DIR
  pushd $BUILD_DIR

  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DOPENSSL_ROOT="$OPENSSL_ROOT_DIR" \
    -DOPENSSL_LIBRARYDIR="$OPENSSL_LIBRARY_DIR" \
    -DCMAKE_INSTALL_PREFIX="$NETBOX_INSTALL_DIR" \
    -DOPT_USE_UNITY_BUILD=OFF \
    -DOPT_BUILD_OPENSSL=OFF \
    -G "Visual Studio 15 2017 Win64" \
    "$NETBOX_ROOT_DIR"

  #cmake --build . --target install
  #msbuild netbox.sln

  popd
}

main() {
  setup_netbox
}

#main
