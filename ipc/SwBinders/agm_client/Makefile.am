AM_CPPFLAGS = -I $(srcdir) \
              -I $(PKG_CONFIG_SYSROOT_DIR)/usr/include/agm/ \
              -I $(top_srcdir)/ipc/SwBinders/agm_server/inc/ \
              -I $(top_srcdir)/service/inc/public/
AM_CPPFLAGS += -DDYNAMIC_LOG_ENABLED
lib_LTLIBRARIES      = libagmclientwrapper.la
libagmclientwrapper_ladir = $(libdir)
libagmclientwrapper_la_SOURCES  = src/agm_client_wrapper.cpp
libagmclientwrapper_la_CPPFLAGS = $(AM_CPPFLAGS)
libagmclientwrapper_la_LDFLAGS  = -ldl -lbinder -shared -avoid-version -lutils -lrt
libagmclientwrapper_la_LIBADD   = -L$(top_builddir)/ipc/SwBinders/agm_server/.libs -lagmproxy -laudio_log_utils

if CUTILS_SUPPORTED
libagmclientwrapper_la_LIBADD += -lcutils -llog
endif
