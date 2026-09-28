#!/bin/sh
if [ -d ../buildwasm ]; then
	( cd ../buildwasm && make -j4 && cp lib/turbosynthwasm.js ../web/ )
elif [ -d ../build ]; then
	( cd ../build && make -j4 && cp lib/turbosynthwasm.js ../web/ )
fi
( cd ../patches/florestan && zip -rv ../../web/florestan.zip *)
( cd ../patches/eawpats && zip -rv ../../web/eawpats.zip *)
