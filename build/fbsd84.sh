#!/bin/sh
set -x
curdir=`pwd`
sh buildinit.sh bsd
cd /quadstor/quadstor/pgsql && gmake install
cd $curdir
sed -i -e "s/FreeBSD8.2/FreeBSD8.4/" createpkg.sh
sed -i -e "s/FreeBSD8.2/FreeBSD8.4/" createclient.sh
sed -i -e "s/FreeBSD8.2/FreeBSD8.4/" createitf.sh
sed -i -e "s/FreeBSD 8.2/FreeBSD 8.4/" pkg-post-itf.sh
sed -i -e "s/FreeBSD 8.2/FreeBSD 8.4/" pkg-post.sh
sh createpkg.sh && sh createclient.sh && sh createitf.sh
