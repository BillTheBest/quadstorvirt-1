#!/bin/sh
cmod=`/sbin/kldstat | grep coredev`
if [ "$cmod" = "" ]; then
	return
fi

/etc/rc.d/quadstor stop > /dev/null 2>&1
cmod=`/sbin/kldstat | grep coredev`
if [ "$cmod" = "" ]; then
	return
fi

/etc/rc.d/quadstor onestop > /dev/null 2>&1
cmod=`/sbin/kldstat | grep coredev`
if [ "$cmod" = "" ]; then
	return
fi

echo "Unable to shutdown QUADStor service cleanly. Please restart the system and try again"
exit 1
