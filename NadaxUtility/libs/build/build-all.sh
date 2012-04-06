#!/bin/sh

if [ -z $1 ]
then
	export PROXY=""
else
	export PROXY="-x $1"
fi

pushd $(dirname $0)/..
export ROOTDIR=$(pwd)
popd

rm -fr ../bin
rm -fr ../sbin
rm -fr ../include
rm -fr ../lib

./build-cppunit.sh
./build-icu.sh
./build-minizip.sh
./build-ctemplate.sh
./build-boost.sh
./build-pion.sh
./build-sqlcipher.sh
./build-soci.sh

rm ../lib/*.la
rm ../lib/*.dylib
rm -fr ../lib/icu
rm -fr ../lib/pkgconfig
rm -fr ../share
