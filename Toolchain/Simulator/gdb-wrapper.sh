#!/bin/bash
# GDB 15 reads DEBUGINFOD_URLS from the environment and blocks ~10s per loaded .so
# trying to download separate debug info. `set debuginfod enabled off` is processed
# after the initial library loads, so only unsetting the variable works.
unset DEBUGINFOD_URLS
exec /usr/bin/gdb "$@"
