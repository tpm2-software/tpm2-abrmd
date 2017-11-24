TARGET?=tabrmd
MODULES?=${TARGET:=.pp.bz2}
SHARE?=/usr/share

all: ${TARGET:=.pp.bz2}

%.pp.bz2: %.pp
	@echo Compressing $^ -\> $@
	bzip2 -9 $^

%.pp: %.te
	make -f ${SHARE}/selinux/devel/Makefile $@

clean:
	rm -f *~  *.tc *.pp *.pp.bz2
	rm -rf tmp *.tar.gz

install-policy: all
	semodule -i ${TARGET}.pp.bz2

install: install-policy
	install -D -m 644 ${TARGET}.pp.bz2 ${DESTDIR}${SHARE}/selinux/packages/${TARGET}.pp.bz2
	install -D -m 644 ${TARGET}.if ${DESTDIR}${SHARE}/selinux/devel/include/contrib/${TARGET}.if
