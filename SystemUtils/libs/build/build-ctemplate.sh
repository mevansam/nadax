#!/bin/sh

pushd ../
INSTALLDIR=$(pwd)
popd

CTEMPLATE_VERSION=0.99

curl $PROXY -O "http://google-ctemplate.googlecode.com/files/ctemplate-${CTEMPLATE_VERSION}.tar.gz"
tar zxvf "ctemplate-${CTEMPLATE_VERSION}.tar.gz"
rm "ctemplate-${CTEMPLATE_VERSION}.tar.gz"

# Build
pushd "ctemplate-${CTEMPLATE_VERSION}"
./configure --prefix="${INSTALLDIR}"
make install
rm -fr ${INSTALLDIR}/lib/pkgconfig
rm -f ${INSTALLDIR}/lib/libctemplate_nothreads.0.dylib
rm -f ${INSTALLDIR}/lib/libctemplate_nothreads.dylib
rm -f ${INSTALLDIR}/lib/libctemplate_nothreads.la
rm -f ${INSTALLDIR}/lib/libctemplate.0.dylib
rm -f ${INSTALLDIR}/lib/libctemplate.dylib
rm -f ${INSTALLDIR}/lib/libctemplate.la
popd

rm -fr "ctemplate-${CTEMPLATE_VERSION}"
