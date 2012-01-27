#!/bin/bash

set -x
set -e

pushd $ROOT_DIR/linux/bin

./test

popd
