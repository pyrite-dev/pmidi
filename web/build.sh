#!/bin/sh
if [ -d ../buildwasm ]; then
	( cd ../buildwasm && make -j4 && cp lib/turbosynthwasm.js ../web/ )
elif [ -d ../build ]; then
	( cd ../build && make -j4 && cp lib/turbosynthwasm.js ../web/ )
fi
( cd ../patches && zip -rv ../web/ct2mgm.zip ct2mgm*)
