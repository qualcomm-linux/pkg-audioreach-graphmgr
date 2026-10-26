pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmserver.pc
EXTRA_DIST = $(pkgconfig_DATA)

h_sources  = $(srcdir)/inc/ipc_interface.h
h_sources += $(srcdir)/inc/agm_death_notifier.h
h_sources += $(srcdir)/inc/agm_callback.h

AM_CPPFLAGS = -I $(srcdir) -I ${srcdir}/inc \
              -I $(top_srcdir)/service/inc/public/ \
              -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/agm/ \
              -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/qti-agm-service

AM_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\)
AM_CPPFLAGS += -DDYNAMIC_LOG_ENABLED
library_include_HEADERS = $(h_sources)
library_includedir = $(includedir)/qti-agm-service/

lib_LTLIBRARIES = libagmserverwrapper.la
libagmserverwrapper_la_SOURCES = ${srcdir}/src/agm_death_notifier.cpp ${srcdir}/src/agm_server_wrapper.cpp ${srcdir}/src/agm_callback.cpp

libagmserverwrapper_la_CPPFLAGS = $(AM_CPPFLAGS)
libagmserverwrapper_la_LDFLAGS = -ldl -lbinder -shared -avoid-version -lutils -lpthread

lib_LTLIBRARIES += libagmproxy.la
libagmproxy_la_SOURCES = ${srcdir}/src/ipc_proxy_server.cpp
libagmproxy_la_CPPFLAGS = $(AM_CPPFLAGS)
libagmproxy_la_LIBADD = libagmserverwrapper.la
libagmproxy_la_LDFLAGS = -ldl -lbinder -shared -avoid-version -lutils -lpthread

bin_PROGRAMS = agm_server
agm_server_SOURCES  = ${srcdir}/src/agm_server_daemon.cpp

agm_server_CPPFLAGS = $(AM_CPPFLAGS)
agm_server_LDADD = libagmproxy.la -laudio_log_utils
libagmserverwrapper_la_LIBADD = -L$(top_builddir)/service/.libs -lagm -laudio_log_utils
agm_server_LDFLAGS = -ldl -lbinder -shared -avoid-version -lutils

if CUTILS_SUPPORTED
AM_CPPFLAGS += -DAGM_USE_CUTILS
libagmserverwrapper_la_LIBADD += -lcutils -llog
libagmproxy_la_LIBADD += -lcutils -llog
agm_server_LDADD += -lcutils -llog
endif

if USE_SYSLOG
AM_CPPFLAGS += -DAGM_USE_SYSLOG
endif
