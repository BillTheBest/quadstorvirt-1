set -e
os=`uname`

if [ "$QUADSTOR_ROOT" = "" ]; then
	QUADSTOR_ROOT=`cd ../../.. && pwd`
fi

if [ "$QUADSTOR_ROOT" = "" ]; then
	QUADSTOR_INSTALL_ROOT="/quadstor"
fi

if [ "$1" = "install" ]; then
	mkdir -p $QUADSTOR_INSTALL_ROOT/lib/modules
	cp -f iscsit.ko $QUADSTOR_INSTALL_ROOT/lib/modules/
	exit 0
fi

if [ "$os" = "FreeBSD" ]; then
	make -f Makefile.iet $1
else
	make $1
fi
