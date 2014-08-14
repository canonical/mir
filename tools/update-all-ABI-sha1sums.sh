#!/bin/sh

find include/client include/shared/mir_toolkit \
    -type f | sort | xargs sha1sum > client-ABI-sha1sums

# Server headers include both platform and common headers. TODO: Script to
# identify which headers, so we can know which mircommon/mirplatform header
# changes are also breaking the server ABI.
find include/server include/platform include/shared \
    -type f | sort | xargs sha1sum > server-ABI-sha1sums

# Platform headers include mircommon headers. TODO: Script to identify which
# headers so we can know which mircommon header changes should also result in a
# bump to MIRPLATFORM_ABI.
find include/platform include/shared \
    -type f | sort | xargs sha1sum > platform-ABI-sha1sums

find include/shared \
    -type f | sort | xargs sha1sum > common-ABI-sha1sums
