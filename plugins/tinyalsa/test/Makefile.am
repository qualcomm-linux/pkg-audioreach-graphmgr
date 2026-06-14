pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmtest.pc
EXTRA_DIST = $(pkgconfig_DATA)

AM_CFLAGS = -Wno-unused-parameter -Wno-unused-result
AM_CFLAGS += -I$(top_srcdir)/service/inc/public
AM_CFLAGS += $(GLIB_CFLAGS) -include glib.h @KVH2XML_CFLAGS@

if USE_G_STR_FUNC
AM_CFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif

AM_CFLAGS += -DBACKEND_CONF_FILE=\"/etc/backend_conf.xml\"
lib_LTLIBRARIES      = libagmmixer.la
libagmmixer_la_SOURCES   = agmmixer.c
libagmmixer_la_CFLAGS = $(AM_CFLAGS)
libagmmixer_la_LDFLAGS   = -shared -version-number @LT_VERSION_NUMBER@
libagmmixer_la_LIBADD   = -ltinyalsa -ldl -lexpat @GLIB_LIBS@

bin_PROGRAMS = agmplay
agmplay_SOURCES  = agmplay.c

agmplay_la_CFLAGS = $(AM_CFLAGS)
agmplay_LDADD    = -ltinyalsa libagmmixer.la

bin_PROGRAMS += agmcap
agmcap_SOURCES  = agmcap.c

agmcap_la_CFLAGS = $(AM_CFLAGS)
agmcap_LDADD    = -ltinyalsa libagmmixer.la

# install xml files under /etc
root_etcdir      = "/etc"
root_etc_SCRIPTS = backend_conf.xml
install-data-hook:
	chmod  go-x $(DESTDIR)$(root_etcdir)/backend_conf.xml
