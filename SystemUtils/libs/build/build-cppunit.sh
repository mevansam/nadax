#!/bin/sh

CPPUNIT_VERSION=1.12.1

pushd ${ROOTDIR}/tmp
if [ ! -e "cppunit-${CPPUNIT_VERSION}.tar.gz" ]
then
	curl $PROXY -O "http://voxel.dl.sourceforge.net/project/cppunit/cppunit/${CPPUNIT_VERSION}/cppunit-${CPPUNIT_VERSION}.tar.gz"
fi
tar zxvf "cppunit-${CPPUNIT_VERSION}.tar.gz"

# Build
pushd cppunit-${CPPUNIT_VERSION}
./configure --prefix="${ROOTDIR}"
make install
popd

rm -fr cppunit-${CPPUNIT_VERSION}
popd
