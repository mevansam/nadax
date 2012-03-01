#!/bin/sh

BOOST_VERSION=1.48.0

pushd ${ROOTDIR}/tmp
if [ ! -e "boost_${BOOST_VERSION//./_}.tar.gz" ]
then
	curl $PROXY -O "http://surfnet.dl.sourceforge.net/project/boost/boost/${BOOST_VERSION}/boost_${BOOST_VERSION//./_}.tar.gz"
fi
tar zxvf "boost_${BOOST_VERSION//./_}.tar.gz"

pushd "boost_${BOOST_VERSION//./_}"
./bootstrap.sh
./b2 link=static threading=multi --includedir=${ROOTDIR}/include --libdir=${ROOTDIR}/lib install
popd

rm -rf "boost_${BOOST_VERSION//./_}"
popd
