#!/bin/sh
while [ ! -d .git ]; do
	cd ..
done
clang-format --verbose -i */*.c */*.h
