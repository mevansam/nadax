#!/bin/sh

pushd ../
INSTALLDIR=$(pwd)
popd

SQLCIPHER_VERSION=1.1.8

curl $PROXY -o "sqlcipher-${SQLCIPHER_VERSION}.tar.gz" -L "https://github.com/sjlombardo/sqlcipher/tarball/v${SQLCIPHER_VERSION}"
tar zxvf "sqlcipher-${SQLCIPHER_VERSION}.tar.gz"
rm "sqlcipher-${SQLCIPHER_VERSION}.tar.gz"

# Build
pushd sjlombardo-sqlcipher-*
./configure --prefix="${INSTALLDIR}" --disable-tcl --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC" LDFLAGS="-lcrypto"
make install
rm -fr ${INSTALLDIR}/lib/pkgconfig
rm -f ${INSTALLDIR}/lib/libsqlite3.0.dylib
rm -f ${INSTALLDIR}/lib/libsqlite3.dylib
rm -f ${INSTALLDIR}/lib/libsqlite3.la
popd

rm -fr sjlombardo-sqlcipher-*
