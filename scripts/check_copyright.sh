#!/bin/sh

status=0

for f in "$@"; do
    if ! grep -q "Copyright" "$f"; then
        echo "ERROR: Missing copyright header: $f"
        status=1
    fi

    if ! grep -q "@author" "$f"; then
        echo "ERROR: Missing or invalid author in header: $f"
        status=1
    fi

done

exit $status