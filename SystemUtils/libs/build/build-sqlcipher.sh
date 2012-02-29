#!/bin/sh

SQLCIPHER_VERSION=2.0.3

pushd ${ROOTDIR}/tmp
if [ ! -e "sqlcipher-${SQLCIPHER_VERSION}.tar.gz" ]
then
	curl $PROXY -o "sqlcipher-${SQLCIPHER_VERSION}.tar.gz" -L "https://nodeload.github.com/sqlcipher/sqlcipher/tarball/v${SQLCIPHER_VERSION}"
fi
tar zxvf "sqlcipher-${SQLCIPHER_VERSION}.tar.gz"

# Build
pushd *-sqlcipher-*
./configure --prefix="${ROOTDIR}" --disable-tcl --enable-threadsafe --enable-cross-thread-connections --enable-tempstore=no CFLAGS="-DSQLITE_HAS_CODEC" LDFLAGS="-lcrypto"
make install
popd

rm -fr *-sqlcipher-*
popd
