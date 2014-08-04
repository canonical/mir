#!/bin/sh
find include/server include/platform include/shared -type f | sort | xargs sha1sum
