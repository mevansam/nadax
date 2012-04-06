#!/bin/sh

MINIZIP_VERSION=101h

pushd ${ROOTDIR}/tmp

if [ ! -e "unzip${MINIZIP_VERSION}.zip" ]
then
  curl $PROXY -O "http://www.winimage.com/zLibDll/unzip${MINIZIP_VERSION}.zip"
fi

# Extract source
rm -rf "unzip${MINIZIP_VERSION}"

mkdir "unzip${MINIZIP_VERSION}"
pushd "unzip${MINIZIP_VERSION}"
unzip "../unzip${MINIZIP_VERSION}.zip"

cp -f ${ROOTDIR}/build/Makefile.minizip .
make -f Makefile.minizip install CC="cc" CFLAGS="-I${ROOTDIR}/include -Dunix" LDFLAGS=" -shared -nostdlib -L/usr/lib -lc -lz" RANLIB="ranlib" PREFIX="${ROOTDIR}"

popd
rm -rf unzip${MINIZIP_VERSION}
