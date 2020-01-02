AUTOMAKE_OPTIONS = foreign



# check for system
if LINUX
    AM_CPPFLAGS = -Wl,--no-as-needed
else
    AM_CPPFLAGS = -Wl,-undefined,dynamic_lookup
endif


AM_CPPFLAGS += -W  -Wall -g -std=c99 -fPIC

lib_LTLIBRARIES = afxd.la
afxd_la_LDFLAGS = -module -avoid-version -shared
afxd_la_SOURCES =  afxd.c

EXTRA_DIST = README.md includes