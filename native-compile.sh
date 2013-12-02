#!/bin/sh
# Native build script for Mir
#
set -e

BUILD_DIR=build-linux-x86
if [ ! -e ${BUILD_DIR} ]; then
    mkdir ${BUILD_DIR}
    ( cd ${BUILD_DIR} && cmake ..)
fi

cmake --build ${BUILD_DIR}

GTEST_OUTPUT=xml:./
${BUILD_DIR}/bin/mir_acceptance_tests
${BUILD_DIR}/bin/mir_integration_tests
${BUILD_DIR}/bin/mir_unit_tests
