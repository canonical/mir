#!/bin/sh
find include/server include/platform include/shared | egrep "\.h$" | xargs sha1sum
