#!/bin/sh

pushd ../
INSTALLDIR=$(pwd)
popd

BOOST_VERSION=1.46.1

curl $PROXY -O "http://surfnet.dl.sourceforge.net/project/boost/boost/${BOOST_VERSION}/boost_${BOOST_VERSION//./_}.tar.gz"
tar zxvf "boost_${BOOST_VERSION//./_}.tar.gz"

pushd "boost_${BOOST_VERSION//./_}"
./bootstrap.sh
./bjam --includedir=${INSTALLDIR}/include --libdir=${INSTALLDIR}/lib install
popd

rm "boost_${BOOST_VERSION//./_}.tar.gz"
rm -rf "boost_${BOOST_VERSION//./_}"
rm ${INSTALLDIR}/lib/*.dylib
