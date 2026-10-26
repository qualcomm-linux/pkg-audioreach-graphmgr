pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = agmserver.pc
EXTRA_DIST = $(pkgconfig_DATA)

h_sources = ./inc/agm-dbus-utils.h \
            ./inc/agm_server_wrapper_dbus.h

AM_CPPFLAGS = -I $(srcdir)/inc -I $(top_srcdir)/service/inc/public
AM_CPPFLAGS += -I $(top_srcdir)/service/inc/public/agm
AM_CPPFLAGS += -D__unused=__attribute__\(\(__unused__\)\)

lib_LTLIBRARIES = libagmserverwrapper.la
libagmserverwrapper_la_SOURCES = ./src/agm-dbus-utils.cpp ./src/agm_server_wrapper_dbus.cpp
libagmserverwrapper_la_CPPFLAGS = $(AM_CPPFLAGS) $(DBUS_CFLAGS)
libagmserverwrapper_la_LIBADD = -lagm -L$(top_builddir)/service/.libs
libagmserverwrapper_la_LDFLAGS = -ldl -lpthread @OSAL_LIBS@ -shared -avoid-version $(DBUS_LIBS)

bin_PROGRAMS = agm_server

agm_server_SOURCES = ./src/agm-server-daemon.cpp
agm_server_CPPFLAGS = $(AM_CPPFLAGS) -Wno-write-strings
agm_server_LDADD = libagmserverwrapper.la
agm_server_LDFLAGS = -ldl -shared -avoid-version

if USE_G_STR_FUNC
AM_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif

if USE_SYSLOG
AM_CPPFLAGS += -DAGM_USE_SYSLOG
endif

if USE_GLIB
AM_CPPFLAGS += $(GLIB_CFLAGS)
libagmserverwrapper_la_LIBADD += $(GLIB_LIBS)
agm_server_LDADD += $(GLIB_LIBS)
endif
