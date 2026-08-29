#!/bin/sh
if [ -d ../buildwasm ]; then
	( cd ../buildwasm && make -j4 )
elif [ -d ../build ]; then
	( cd ../build && make -j4 )
fi
( cd ../patches && zip -rv ../web/ct2mgm.zip ct2mgm*)
