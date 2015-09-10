#!/bin/sh

#export CPPFLAGS=
if [ -f config.status ]; then
	make maintainer-clean
fi

#--enable-threads       \
#--disable-obex       \

./bootstrap && \
    CPPFLAGS=" -g3 -Wall " CFLAGS=" -g3 -Wall " ./configure --enable-maintainer-mode \
		--enable-debug \
		--prefix=/usr \
		--mandir=/usr/share/man \
		--enable-pie           \
    --prefix=/usr          \
    --enable-library       \
    --enable-test          \
    --disable-systemd      \
		--sysconfdir=/etc \
		--localstatedir=/var \
		--enable-manpages \
		--enable-experimental \
		--enable-android \
		--enable-sixaxis \
		--disable-datafiles $*
