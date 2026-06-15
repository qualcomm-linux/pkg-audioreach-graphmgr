pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmalsaplugin.pc
EXTRA_DIST = $(pkgconfig_DATA)

library_includedir = $(includedir)

AM_CFLAGS = -Wno-unused-parameter
AM_CFLAGS += -I $(top_srcdir)/include -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include
AM_CFLAGS += -I $(top_srcdir)/service/inc/public -I $(top_srcdir)/snd_parser/inc
AM_CFLAGS += $(GLIB_CFLAGS)
AM_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\) -D__LINUX__
AM_CFLAGS += -I $(top_srcdir)/../../snd_parser/inc

if USE_SYSLOG
AM_CFLAGS += -DAGM_USE_SYSLOG
endif

if USE_G_STR_FUNC
AM_CFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif


if BUILD_ALSALIB

alsaplugindir = $(libdir)/alsa-lib
alsaplugin_LTLIBRARIES      = libasound_module_pcm_agm.la
libasound_module_pcm_agm_la_SOURCES   = src/agm_io_plugin.c
libasound_module_pcm_agm_la_CFLAGS = $(AM_CFLAGS)
libasound_module_pcm_agm_la_LDFLAGS = -avoid-version -shared \
    -L$(top_builddir)/snd_parser/.libs \
    -L$(PKG_CONFIG_SYSROOT_DIR)/usr/lib
if AGM_NO_IPC
libasound_module_pcm_agm_la_CFLAGS += -DAGM_NO_IPC
libasound_module_pcm_agm_la_LDFLAGS += -L$(top_builddir)/service/.libs
libasound_module_pcm_agm_la_LIBADD  = -lasound -lsndcardparser -lagm
else
libasound_module_pcm_agm_la_LDFLAGS += -L$(top_builddir)/ipc/DBus/agm_client/.libs
libasound_module_pcm_agm_la_LIBADD  = -lasound -lsndcardparser -lagmclient
endif

alsaplugin_LTLIBRARIES      += libasound_module_ctl_agm.la
libasound_module_ctl_agm_la_SOURCES   = src/agm_ctl_plugin.c
libasound_module_ctl_agm_la_CFLAGS = $(AM_CFLAGS)

libasound_module_ctl_agm_la_LDFLAGS = -avoid-version -shared \
    -L$(top_builddir)/snd_parser/.libs \
    -L$(PKG_CONFIG_SYSROOT_DIR)/usr/lib
if AGM_NO_IPC
libasound_module_ctl_agm_la_CFLAGS += -DAGM_NO_IPC
libasound_module_ctl_agm_la_LDFLAGS += -L$(top_builddir)/service/.libs
libasound_module_ctl_agm_la_LIBADD  = -lasound -lsndcardparser -lagm
else
libasound_module_ctl_agm_la_LDFLAGS += -L$(top_builddir)/ipc/DBus/agm_client/.libs
libasound_module_ctl_agm_la_LIBADD  = -lasound -lsndcardparser -lagmclient
endif

endif
