#!/bin/bash

set -x
set -e

pushd $ROOT_DIR/vscript/test

./test.pl

popd
