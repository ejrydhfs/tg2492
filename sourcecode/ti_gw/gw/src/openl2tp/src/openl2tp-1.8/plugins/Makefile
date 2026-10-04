CFLAGS	:= $(CFLAGS.optimize) -MMD -Wall \
	-isystem ../include \
	-I. -I.. -I../usl -fPIC $(EXTRA_CFLAGS)
LDFLAGS	:= -shared

PLUGINS.c:=	ppp_unix.c ppp_null.c ipsec.c event_sock.c

ifneq (,$(findstring -DL2TP_TEST,$(CFLAGS)))
PLUGINS.c+=	ppp_test.c
endif

# ARRIS CHANGE START - we only need ppp_unix.so
PLUGINS.so=	$(PLUGINS.c:%.c=%.so)
PLUGINS.so=     ppp_unix.so
# ARRIS CHANGE END

SRC.generated=

.PHONY:		all clean install TAGS generated-files

all:		generated-files $(PLUGINS.so)

clean:
		$(RM) $(PLUGINS.so) $(wildcard *.o) $(wildcard *.d) $(SRC.generated)

%.so: %.c
		$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $<

install:	all
		# ARRIS CHANGE START
		#$(INSTALL) -d $(DESTDIR)$(SYS_LIBDIR)/openl2tp
		#$(INSTALL) $(PLUGINS.so) $(DESTDIR)$(SYS_LIBDIR)/openl2tp
		#$(INSTALL) -m 0644 event_sock.h $(DESTDIR)$(SYS_LIBDIR)/openl2tp
		$(INSTALL) -d $(DESTDIR)$(SYS_LIBDIR)
		$(INSTALL) $(PLUGINS.so) $(DESTDIR)$(SYS_LIBDIR)
		# ARRIS CHANGE END


generated-files:	$(SRC.generated)

include $(wildcard *.d) /dev/null
