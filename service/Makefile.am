pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agm.pc
EXTRA_DIST = $(pkgconfig_DATA)

h_sources = ./inc/public/agm/agm_api.h \
            ./inc/public/agm/agm_list.h \
            ./inc/public/agm/utils.h

AM_CFLAGS = -I $(srcdir)/inc/public \
            -I $(srcdir)/inc/private \
            -I $(top_srcdir)/snd_parser/inc \
            -I $(top_builddir)/src/system/core/include/log/

library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)/agm

agm_sources = ./src/graph.c \
              ./src/graph_module.c \
              ./src/device.c \
              ./src/device_hw_ep.c \
              ./src/metadata.c \
              ./src/session_obj.c \
              ./src/utils.c \
              ./src/agm.c

if MEMLOGGER_SUPPORTED
agm_sources += ./src/agm_memlogger.c
AM_CFLAGS += -DDYNAMIC_LOG_ENABLED
else
AM_CFLAGS += -Wl,-z,defs \
             @SPF_CFLAGS@ \
             @KVH2XML_CFLAGS@
endif

lib_LTLIBRARIES = libagm.la
libagm_la_SOURCES = $(agm_sources)
libagm_la_LIBADD = -ltinyalsa @OSAL_LIBS@ @GSL_LIBS@ @ATS_LIBS@ -lsndcardparser
libagm_la_LDFLAGS = -L$(top_builddir)/snd_parser/.libs

libagm_la_CFLAGS = $(AM_CFLAGS) -DACDB_PATH=\"/etc/acdbdata/\" -DACDB_DELTA_FILE_PATH="/etc/acdbdata/delta"
libagm_la_CFLAGS += -DCARD_STATE_UNSUPPORTED

if USE_DEFAULT_ACDB_PATH
libagm_la_CFLAGS += -DUSE_DEFAULT_ACDB_PATH
endif

if CUTILS_SUPPORTED
libagm_la_LIBADD += -lcutils -llog
AM_CFLAGS += -DAGM_USE_CUTILS
endif

if USE_SYSLOG
libagm_la_CFLAGS += -DAGM_USE_SYSLOG
endif

if MEMLOGGER_SUPPORTED
libagm_la_LIBADD += -laudio_log_utils -larmemlog
else
libagm_la_CFLAGS += -DAGM_MEMLOG_UNSUPPORTED
endif

if USE_GLIB
libagm_la_LIBADD += $(GLIB_LIBS)
libagm_la_CFLAGS += $(GLIB_CFLAGS) -include glib.h
endif

if USE_G_STR_FUNC
libagm_la_CFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif

if USE_ARE_ON_APPS
libagm_la_LIBADD += -lspf @GPR_LIBS@
libagm_la_CFLAGS += -DARE_ON_APPS @GPR_CFLAGS@
endif

libagm_la_CFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
libagm_la_LDFLAGS += -shared -version-number @LT_VERSION_NUMBER@
