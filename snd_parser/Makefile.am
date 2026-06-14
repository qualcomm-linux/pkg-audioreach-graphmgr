pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = sndparser.pc
EXTRA_DIST = $(pkgconfig_DATA)

AUTOMAKE_OPTIONS = subdir-objects

h_sources  = inc/snd-card-def.h

lib_include_HEADERS = $(h_sources)
lib_includedir = $(includedir)/sndparser/

AM_CFLAGS = -I $(srcdir)/inc -I $(top_srcdir)/service/inc/public
AM_CFLAGS += -Wno-unused-parameter
AM_CFLAGS += -DCARD_DEF_FILE=\"/etc/card-defs.xml\"
AM_CFLAGS += -DCARD_DEF_FILE_NATIVE=\"/etc/card-defs-native.xml\"
AM_CFLAGS += -Wl,-z,defs

lib_LTLIBRARIES      = libsndcardparser.la
libsndcardparser_la_SOURCES   = src/snd-card-parser.c
libsndcardparser_la_LIBADD = -lexpat -lpthread
libsndcardparser_la_LDFLAGS   = -shared -version-number @LT_VERSION_NUMBER@
libsndcardparser_la_CFLAGS = $(AM_CFLAGS)
if USE_GLIB
libsndcardparser_la_LIBADD += $(GLIB_LIBS)
libsndcardparser_la_CFLAGS += $(GLIB_CFLAGS) -include glib.h
endif
if USE_G_STR_FUNC
libsndcardparser_la_CFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif
