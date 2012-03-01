#!/bin/sh

SOCI_VERSION=3.1.0

pushd ${ROOTDIR}/tmp

if [ ! -e "soci-${SOCI_VERSION}.zip" ]
then
	curl $PROXY -O "http://surfnet.dl.sourceforge.net/project/soci/soci/soci-${SOCI_VERSION}/soci-${SOCI_VERSION}.zip"
fi
unzip "soci-${SOCI_VERSION}.zip"

if [ ! -d $${ROOTDIR}/include/soci ]
then
	mkdir -p ${ROOTDIR}/include/soci
fi
if [ ! -d ${ROOTDIR}/lib ]
then
	mkdir -p ${ROOTDIR}/lib
fi

pushd soci-${SOCI_VERSION}/core
make -f Makefile.basic
cp -f *.a ${ROOTDIR}/lib
cp -f *.h ${ROOTDIR}/include/soci
popd
pushd soci-${SOCI_VERSION}/backends/sqlite3
make -f Makefile.basic
cp -f *.a ${ROOTDIR}/lib
cp -f *.h ${ROOTDIR}/include/soci
popd

rm -fr soci-${SOCI_VERSION}
popd
