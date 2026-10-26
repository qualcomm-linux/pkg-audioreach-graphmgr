lib_LTLIBRARIES      = libagmclient.la
libagmclient_la_CPPFLAGS = -I $(top_srcdir)/service/inc/public
libagmclient_la_CPPFLAGS += -I $(top_srcdir)/service/inc/public/agm
libagmclient_ladir = $(libdir)
libagmclient_la_LDFLAGS = -ldl -lrt -shared -version-number @LT_VERSION_NUMBER@
libagmclient_la_SOURCES = src/agm_client_wrapper_dbus.cpp
libagmclient_la_LIBADD =

if USE_SYSLOG
libagmclient_la_CPPFLAGS += -DAGM_USE_SYSLOG
endif

if USE_GLIB
libagmclient_la_CPPFLAGS += $(GLIB_CFLAGS)
libagmclient_la_LIBADD += $(GLIB_LIBS) -lgobject-2.0 -lgio-2.0
endif

if USE_G_STR_FUNC
libagmclient_la_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif
