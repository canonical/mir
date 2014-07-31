#!/bin/sh
find include/client include/shared/mir_toolkit | egrep "\.h$" | xargs sha1sum
