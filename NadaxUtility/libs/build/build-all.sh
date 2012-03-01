#!/bin/sh

if [ -z $1 ]
then
	export PROXY=""
else
	export PROXY="-x $2"
fi

pushd $(dirname $0)/..
export ROOTDIR=$(pwd)
popd

rm -fr ../bin
rm -fr ../include
rm -fr ../lib

./build-cppunit.sh
./build-boost.sh
./build-sqlcipher.sh
./build-soci.sh

rm -fr ../lib/pkgconfig
rm -fr ../share
