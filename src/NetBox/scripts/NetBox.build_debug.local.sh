#!/usr/bin/env bash

set -eo pipefail

# C:\VS2019\VC\Auxiliary\Build\vcvarsall.bat x86_amd64 && bash ./build_debug.local.sh

#BUILD_TYPE=Debug
BUILD_TYPE=Release

OPENSSL_ARCHIVE="https://www.openssl.org/source/openssl-1.0.2s.tar.gz"
OPENSSL_ROOT_DIR="D:/Projects/openssl-1.0.2s"
OPENSSL_BUILD_DIR="$OPENSSL_ROOT_DIR/VS2019x64-$BUILD_TYPE"
# OPENSSL_INSTALL_DIR="$OPENSSL_ROOT_DIR/VS2019x64-$BUILD_TYPE"
OPENSSL_LIBRARY_DIR="$OPENSSL_ROOT_DIR/VS2019x64-$BUILD_TYPE"

NETBOX_ROOT_DIR="L:/Projects/FarPlugins/NetBox/src/NetBox"
NETBOX_INSTALL_DIR="D:/Projects/NetBox-VS2019x64-$BUILD_TYPE"
NETBOX_BUILD_DIR="D:/Projects/NetBox-VS2019x64-$BUILD_TYPE"


if [ -z "$VS2019INSTALLDIR" ] ; then
  VS2019INSTALLDIR="C:/VS2019"
fi
#echo "VS2019INSTALLDIR: $VS2019INSTALLDIR"
if [ -d "$NETBOX_BUILD_DIR" ] ; then
  rm -rf "$NETBOX_BUILD_DIR"
fi

download_archives() {

 echo
}

. ./build_debug.sh

#setup_openssl
setup_netbox
