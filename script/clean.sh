#!/bin/bash

set -x
set -e

rm -rf linux/obj/*
rm -rf linux/bin/*
rm -rf linux/lib/*

pushd vscript/test
rm -f core.*
popd
