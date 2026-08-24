#!/bin/sh
while [ ! -d .git ]; do
	cd ..
done
clang-format --verbose -i `find src lib glsrc "(" -name "*.c" -or -name "*.h" ")" -and -not -name "miniaudio.*" -and -not -name "stb_*.*"`
