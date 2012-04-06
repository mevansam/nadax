#!/bin/sh

CTEMPLATE_VERSION=2.0

pushd ${ROOTDIR}/tmp
if [ ! -e "ctemplate-${CTEMPLATE_VERSION}.tar.gz" ]
then
	curl $PROXY -O "http://ctemplate.googlecode.com/files/ctemplate-${CTEMPLATE_VERSION}.tar.gz"
fi
tar zxvf "ctemplate-${CTEMPLATE_VERSION}.tar.gz"

pushd ctemplate-${CTEMPLATE_VERSION}
./configure --prefix="${ROOTDIR}"
make install
popd

rm -fr "ctemplate-${CTEMPLATE_VERSION}"
popd
