#!/bin/bash

BUILDDIR=$1
SRCDIR=$2
CASPARCG_BIN=$3
CEF_PATH=$4

mkdir -p ${BUILDDIR}/staging/{bin,lib}

STAGINGDIR=${BUILDDIR}/staging
BINDIR=${STAGINGDIR}/bin
LIBDIR=${STAGINGDIR}/lib

cp ${CASPARCG_BIN} ${BINDIR}

cp ${SRCDIR}/shell/casparcg.config ${STAGINGDIR}
cp ${SRCDIR}/shell/run.sh ${STAGINGDIR}

if [ -z "$CEF_PATH" ];
then
    :
else
    cp -r ${CEF_PATH}/Resources/* ${BINDIR}
    cp ${CEF_PATH}/Release/*.so ${LIBDIR}
    cp ${CEF_PATH}/Release/*.bin ${BINDIR}
    cp -r ${CEF_PATH}/Release/swiftshader ${BINDIR}
fi
