#!/usr/bin/env bash
# Build Go shared library for AmnesiaIDE GitHub bridge
set -e
cd "$(dirname "$0")"
echo "Building Go bridge library..."
go build -o gh.so -buildmode=c-shared gh.go
echo "Done: go/gh.so + go/gh.h"
