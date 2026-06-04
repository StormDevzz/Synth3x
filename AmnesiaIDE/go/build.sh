#!/usr/bin/env bash
# Build Go shared library for AmnesiaIDE bridge
set -e
cd "$(dirname "$0")"
echo "Building Go bridge library..."
go build -buildmode=c-shared -o gh.so ./main
echo "Done: go/gh.so + go/gh.h"
