#!/bin/sh
libvers="3.0.49-FreeBSD8.2-x86_64"
rm -rf /quadstor/bin
rm -rf /quadstor/lib
rm -rf /quadstor/sbin
rm -rf /quadstor/etc

mkdir -p /quadstor/etc
mkdir -p /quadstor/etc/iet
mkdir -p /quadstor/bin
mkdir -p /quadstor/sbin
mkdir -p /quadstor/lib/modules

cd /quadstor/quadstor/
sh buildcluster.sh clean
sh buildcluster.sh 
sh buildcluster.sh install

install -m 755 /quadstor/quadstor/target-mode/iscsi/etc/initd/initd.bsd /quadstor/etc/iscsit
install -m 755 /quadstor/quadstor/target-mode/iscsi/etc/targets.allow /quadstor/etc/iet/targets.allow.sample
install -m 755 /quadstor/quadstor/target-mode/iscsi/etc/initiators.allow /quadstor/etc/iet/initiators.allow.sample
install -m 755 /quadstor/quadstor/target-mode/iscsi/etc/ietd.conf /quadstor/etc/iet/ietd.conf.sample

install -m 755 /quadstor/quadstor/target-mode/iscsi/kernel/iscsit.ko /quadstor/lib/modules/
install -m 755 /quadstor/quadstor/target-mode/iscsi/usr/ietd /quadstor/sbin/
install -m 755 /quadstor/quadstor/target-mode/iscsi/usr/ietadm /quadstor/bin/

#install source
rm -rf /quadstor/src/
cd /quadstor/quadstor/target-mode/iscsi/kernel && make -f Makefile.iet clean
cd /quadstor/quadstor/target-mode/iscsi/usr && gmake clean

mkdir -p /quadstor/src/target-mode/iscsi/kernel
mkdir -p /quadstor/src/target-mode/iscsi/usr
mkdir -p /quadstor/src/target-mode/iscsi/include
cp /quadstor/quadstor/target-mode/iscsi/include/*.h /quadstor/src/target-mode/iscsi/include/
cp /quadstor/quadstor/target-mode/iscsi/kernel/*.[ch] /quadstor/src/target-mode/iscsi/kernel/
cp /quadstor/quadstor/target-mode/iscsi/kernel/Makefile.iet.dist /quadstor/src/target-mode/iscsi/kernel/Makefile
cp /quadstor/quadstor/target-mode/iscsi/usr/*.[ch] /quadstor/src/target-mode/iscsi/usr/
cp /quadstor/quadstor/target-mode/iscsi/usr/Makefile.dist /quadstor/src/target-mode/iscsi/usr/Makefile

cd /quadstor/quadstor/build/
rm /quadstor/quadstor/build/pkg-plist
echo "quadstor/bin/ietadm" >> /quadstor/quadstor/build/pkg-plist
echo "quadstor/sbin/ietd" >> /quadstor/quadstor/build/pkg-plist
echo "quadstor/lib/modules/iscsit.ko" >> /quadstor/quadstor/build/pkg-plist
echo "quadstor/etc/iscsit" >> /quadstor/quadstor/build/pkg-plist
cd / && find quadstor/etc/iet/* >> /quadstor/quadstor/build/pkg-plist

for i in `cd / && find quadstor/src/`;do
	if [ -d $i ]; then
		continue
	fi
	echo $i >> /quadstor/quadstor/build/pkg-plist
done

cd /quadstor/quadstor/build/
rm quadstor-itf-*.tbz
pkg_create -c pkg-comment -d pkg-info-itf -f pkg-plist -p / -I pkg-post-itf.sh -k pkg-preun.sh -K pkg-postun-itf.sh quadstor-itf-$libvers
