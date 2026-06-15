pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmplugin.pc
EXTRA_DIST = $(pkgconfig_DATA)

library_includedir = $(includedir)

AM_CFLAGS = -Wno-unused-parameter
AM_CFLAGS += -I $(top_srcdir)/service/inc/public -I $(top_srcdir)/snd_parser/inc
AM_CFLAGS += $(GLIB_CFLAGS) -include glib.h
if USE_G_STR_FUNC
AM_CFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif
AM_CFLAGS += -DAGM_USE_SYSLOG
AM_CFLAGS += @KVH2XML_CFLAGS@
AM_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
AM_CFLAGS += -Wl,-z,defs

lib_LTLIBRARIES      = libagm_pcm_plugin.la
libagm_pcm_plugin_la_SOURCES = src/agm_pcm_plugin.c
libagm_pcm_plugin_la_CFLAGS  = $(AM_CFLAGS)
libagm_pcm_plugin_la_LDFLAGS = -shared -version-number @LT_VERSION_NUMBER@ -L$(top_builddir)/snd_parser/.libs
libagm_pcm_plugin_la_LIBADD  = -ltinyalsa -lsndcardparser

if AGM_NO_IPC
libagm_pcm_plugin_la_CFLAGS += -DAGM_NO_IPC
libagm_pcm_plugin_la_LDFLAGS += -L$(top_builddir)/service/.libs
libagm_pcm_plugin_la_LIBADD += -lagm
else
libagm_pcm_plugin_la_LDFLAGS += -L$(top_builddir)/ipc/DBus/agm_client/.libs
libagm_pcm_plugin_la_LIBADD += -lagmclient
endif

#lib_LTLIBRARIES      += libtinycompress_module_agm.la
#libtinycompress_module_agm_la_SOURCES = src/agm_compress_plugin.c
#libtinycompress_module_agm_la_CFLAGS  = $(AM_CFLAGS)
#libtinycompress_module_agm_la_LDFLAGS = -shared -version-number @LT_VERSION_NUMBER@ -L$(top_builddir)/snd_parser/.libs
#libtinycompress_module_agm_la_LIBADD  = -ltinyalsa -ltinycompress -lpthread -lsndcardparser
#if AGM_NO_IPC
#libtinycompress_module_agm_la_CFLAGS += -DAGM_NO_IPC
#libtinycompress_module_agm_la_LDFLAGS += -L$(top_builddir)/service/.libs
#libtinycompress_module_agm_la_LIBADD += -lagm
#else
#libtinycompress_module_agm_la_LDFLAGS += -L$(top_builddir)/ipc/DBus/agm_client/.libs
#libtinycompress_module_agm_la_LIBADD += -lagmclient
#endif

lib_LTLIBRARIES      += libagm_mixer_plugin.la
libagm_mixer_plugin_la_SOURCES = src/agm_mixer_plugin.c
libagm_mixer_plugin_la_CFLAGS  = $(AM_CFLAGS)
libagm_mixer_plugin_la_CFLAGS  += -D__unused=__attribute__\(\(__unused__\)\)
libagm_mixer_plugin_la_LDFLAGS = -shared -version-number @LT_VERSION_NUMBER@ -L$(top_builddir)/snd_parser/.libs
libagm_mixer_plugin_la_LIBADD  = -ltinyalsa -lsndcardparser $(GLIB_LIBS)
if AGM_NO_IPC
libagm_mixer_plugin_la_CFLAGS += -DAGM_NO_IPC
libagm_mixer_plugin_la_LDFLAGS += -L$(top_builddir)/service/.libs
libagm_mixer_plugin_la_LIBADD += -lagm
else
libagm_mixer_plugin_la_LIBADD += -lagmclient
libagm_mixer_plugin_la_LDFLAGS += -L$(top_builddir)/ipc/DBus/agm_client/.libs
endif
