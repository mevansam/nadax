#!/bin/sh

PION_VERSION=4.0.11

pushd ${ROOTDIR}/tmp
if [ ! -e "pion-net-${PION_VERSION}.tar.gz" ]
then
	curl $PROXY -O "http://pion.org/files/pion-net-${PION_VERSION}.tar.gz"
fi
tar zxvf "pion-net-${PION_VERSION}.tar.gz"

# Build
pushd pion-net-${PION_VERSION}
./configure LDFLAGS="-L${ROOTDIR}/lib" --prefix="${ROOTDIR}" --enable-static --disable-shared --with-boost="${ROOTDIR}" --with-zlib --with-bzlib --with-openssl --with-ostream-logging
make
make install
popd

rm -fr pion-net-${PION_VERSION}
popd
