#!/bin/sh

ICU_VERSION=4.8.1.1

pushd ${ROOTDIR}/tmp
if [ ! -e "icu4c-${ICU_VERSION//./_}-src.tgz" ]
then
	curl $PROXY -O "http://download.icu-project.org/files/icu4c/4.8.1.1/icu4c-${ICU_VERSION//./_}-src.tgz"
fi
tar zxvf "icu4c-${ICU_VERSION//./_}-src.tgz"

# Build
pushd "icu/source"
./configure --prefix="${ROOTDIR}" --enable-static --disable-shared
make
make install
popd

rm -fr icu
popd
