#!/bin/bash


if test "x$PREFIX_OPENSSL" != "x"; then
   echo $PREFIX_OPENSSL 
   exit 0
fi

OPENSSLDIR=/usr/local/ssl 

if test -d ${OPENSSLDIR}; then
   echo ${OPENSSLDIR}
else
   echo /usr/
fi   
