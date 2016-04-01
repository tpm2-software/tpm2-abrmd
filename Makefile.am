ACLOCAL_AMFLAGS = -I m4

sbin_PROGRAMS   = src/tabd

src_tabd_CFLAGS  = $(DBUS_CFLAGS) $(GIO_CFLAGS)
src_tabd_LDADD   = $(DBUS_LIBS) $(GIO_LIBS)
src_tabd_SOURCES = src/tabd.c
