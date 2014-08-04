#!/bin/sh
find include/client include/shared/mir_toolkit -type f | sort | xargs sha1sum
