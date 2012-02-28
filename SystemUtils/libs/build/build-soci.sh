#!/bin/sh

pushd ../
INSTALLDIR=$(pwd)
popd

if [ ! -d soci ]
then
	git clone git://soci.git.sourceforge.net/gitroot/soci/soci soci
else
	pushd soci
	git pull
	popd

	rmdir -f soci/build/make
fi

if [ ! -d ${INSTALLDIR}/include/soci ]
then
	mkdir -p ${INSTALLDIR}/include/soci
fi
if [ ! -d ${INSTALLDIR}/lib ]
then
	mkdir -p ${INSTALLDIR}/lib
fi

pushd soci/src/core
make -f Makefile.basic
cp -f *.a ../../../../lib
cp -f *.h ../../../../include/soci
popd
pushd soci/src/backends/sqlite3
make -f Makefile.basic
cp -f *.a ../../../../../lib
cp -f *.h ../../../../../include/soci
popd

rm -fr soci
