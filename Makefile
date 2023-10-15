#/******************************************************************************
# * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
# * Plugins.
# *
# * See the README file for copyright information and how to reach the author.
# *****************************************************************************/
BINARY = w_scan_cpp

WIRBELSCAN_VERSION = wirbelscan-2023.10.15

# original git repo seems to be stale.
#SATIP_GIT_ADDR = https://github.com/rofafor/vdr-plugin-satip

SATIP_GIT_ADDR = https://github.com/wirbel-at-vdr-portal/vdr-plugin-satip.git



#/******************************************************************************
# * if you are still running a distro, not beeing able to use all valid
# * standard ASCII Characters (32-126) for package names, and prefer to modify
# * packages instead of fixing your broken distro, you may overwrite the
# * package_name here.
# *****************************************************************************/
package_name ?= $(BINARY)



#/******************************************************************************
# * dependencies, add variables here, and checks in target .dependencies
# *****************************************************************************/
LIBJPEG=libjpeg
FREETYPE2=freetype2
FONTCONFIG=fontconfig
LIBPUGIXML=pugixml
LIBREPFUNC=librepfunc
LIBREPFUNC_MINVERSION=1.1.0

# /* require either PKG_CONFIG_PATH to be set, or, a working pkg-config */
HAVE_LIBJPEG              =$(shell if $(PKG_CONFIG) --exists                                   $(LIBJPEG);    then echo "1"; else echo "0"; fi )
HAVE_FREETYPE2            =$(shell if $(PKG_CONFIG) --exists                                   $(FREETYPE2);  then echo "1"; else echo "0"; fi )
HAVE_FONTCONFIG           =$(shell if $(PKG_CONFIG) --exists                                   $(FONTCONFIG); then echo "1"; else echo "0"; fi )
HAVE_LIBPUGIXML           =$(shell if $(PKG_CONFIG) --exists                                   $(LIBPUGIXML); then echo "1"; else echo "0"; fi )
HAVE_LIBREPFUNC           =$(shell if $(PKG_CONFIG) --exists                                   $(LIBREPFUNC); then echo "1"; else echo "0"; fi )
HAVE_LIBREPFUNC_MINVERSION=$(shell if $(PKG_CONFIG) --atleast-version=$(LIBREPFUNC_MINVERSION) $(LIBREPFUNC); then echo "1"; else echo "0"; fi )


#/******************************************************************************
# * if you prefer verbose non-coloured build messages, remove the '@' here:
# *****************************************************************************/
CC  = @gcc
CXX = @g++


CFLAGS   += -g -O3 -fPIC -Werror=overloaded-virtual
CXXFLAGS += -g -O3 -fPIC -Wall -Wextra -Werror=overloaded-virtual -Wno-unused-parameter -Wfatal-errors



#/******************************************************************************
# * programs, override if on different paths.
# *****************************************************************************/
CD              ?= cd
CP              ?= cp
CHMOD           ?= chmod
GIT             ?= git
INSTALL         ?= install
INSTALL_PROGRAM ?= $(INSTALL) -m 755
INSTALL_DATA    ?= $(INSTALL) -m 644
LN              ?= ln
MAKE            ?= make
MKDIR           ?= mkdir
MKDIR_P         ?= ${MKDIR} -p
PKG_CONFIG      ?= pkg-config
RM              ?= rm
SED             ?= sed
SHELL            = /bin/sh
STRIP           ?= strip
TAR             ?= tar
WGET            ?= wget



#/******************************************************************************
# * directories:
# * HINT: http://115.28.130.42/make/Makefile-Conventions.html
# *****************************************************************************/
tmpdir ?= /tmp

#------------------------
# A prefix used in constructing the default values of the variables listed below.
# You may want (or not..) to change/override this to /usr .
prefix ?= /usr/local

#------------------------
# used for directories that contain machine-specific files, such as executables
# and subroutine libraries), while $(prefix) is used directly for other.
# directories. 
exec_prefix ?= $(prefix)

#------------------------
# directory for installing executable programs that users can run.
bindir = $(exec_prefix)/bin

#------------------------
# directory for installing executable programs that can be run from the shell,
# but are only generally useful to system administrators.
sbindir = $(exec_prefix)/sbin

#------------------------
# directory for installing executable programs to be run by other programs
# rather than by users.
libexecdir = $(exec_prefix)/libexec

#------------------------
# root of the directory tree for read-only architecture-independent data files
# datadir's default value is based on this variable; so are infodir, mandir,
# and others. 
datarootdir = $(prefix)/share

#------------------------
# directory for installing idiosyncratic read-only architecture-independent
# data files for this program. This is usually the same place as 'datarootdir',
# but we use the two separate variables so that you can move these program-
# specific files without altering the location for Info files, man pages, etc.
# ->  Most packages install their data under $(datadir)/package-name/.
datadir = $(datarootdir)

#------------------------
# read-only data files that pertain to a single machine
sysconfdir = $(prefix)/etc

#------------------------
# directory for installing header files to be included by user programs with
# the #include (..) preprocessor directive
includedir = $(prefix)/include

#------------------------
# architecture-independent data files which the programs modify while they run
sharedstatedir = $(prefix)/com

#------------------------
# data files which the programs modify while they run
localstatedir  = $(prefix)/var

#------------------------
# documentation files (other than Info)
docdir = $(datarootdir)/doc/$(package_name)

#------------------------
# the Info files for this package
infodir = $(datarootdir)/info

#------------------------
# object files and libraries of object code. No executables.
libdir = $(exec_prefix)/lib

#------------------------
# locale-specific message catalogs for this package
localedir = $(datarootdir)/locale

#------------------------
# Unix-style man pages 
mandir = $(datarootdir)/man
man1dir = $(mandir)/man1
man2dir = $(mandir)/man2
man3dir = $(mandir)/man3
man4dir = $(mandir)/man4
man5dir = $(mandir)/man5

#------------------------
# no build outside of sources yet. Sorry.
# would be something like:
#      mkdir ./build && cd build && make -j8 install
# -> Anyone..?
srcdir = $(shell pwd)


#------------------------
vdrdir = $(srcdir)/vdr
vdrlibsidir =$(vdrdir)/libsi
pluginsrcdir = $(vdrdir)/PLUGINS/src
pluginlibdir = $(vdrdir)/PLUGINS/lib

WIRBELSCAN_TARBALL = vdr-$(WIRBELSCAN_VERSION).tgz
WIRBELSCAN_DL_ADDR = https://www.gen2vdr.de/wirbel/wirbelscan/$(WIRBELSCAN_TARBALL)







#/******************************************************************************
# * targets starting from here:
# *****************************************************************************/

APIVERSION   = $(shell sed -ne '/define APIVERSION/s/^.*"\(.*\)".*$$/\1/p' $(vdrdir)/config.h)
DEFINES   = -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -DPLUGIN_NAME_I18N='"cmd"'
DEFINES  += -DAPIVERSION='"$(APIVERSION)"'
DEFINES  += -DSTATIC_PLUGINS
LIBS      = -ljpeg -pthread -lcap -ldl -lrt $(shell $(PKG_CONFIG) --libs freetype2 fontconfig)
LIBS     += $(shell curl-config --libs)
LIBS     += $(shell $(PKG_CONFIG) --libs-only-l $(LIBPUGIXML))
LIBS     += $(shell $(PKG_CONFIG) --libs-only-l $(LIBREPFUNC))
INCLUDES  = -I$(srcdir)
INCLUDES += -I$(vdrdir)
INCLUDES += -I$(pluginsrcdir)
INCLUDES += $(shell $(PKG_CONFIG) --cflags-only-I $(LIBJPEG))
INCLUDES += $(shell $(PKG_CONFIG) --cflags-only-I $(LIBPUGIXML))
INCLUDES += $(shell $(PKG_CONFIG) --cflags-only-I $(LIBREPFUNC))
INCLUDES += $(shell $(PKG_CONFIG) --cflags freetype2 fontconfig)
LDFLAGS  += -L$(srcdir) -L$(vdrdir) -L$(vdrlibsidir)
LDFLAGS  += $(shell $(PKG_CONFIG) --libs-only-L $(LIBJPEG))
LDFLAGS  += $(shell $(PKG_CONFIG) --libs-only-L $(LIBPUGIXML))
LDFLAGS  += $(shell $(PKG_CONFIG) --libs-only-L $(LIBREPFUNC))



SOURCES           := $(sort $(wildcard $(srcdir)/*.cpp))
VDR_SOURCES       := $(shell find $(vdrdir)  -maxdepth 1 ! -name "vdr.c" -name "*.c" 2>/dev/null | LC_ALL=C sort)
LIBSI_SOURCES     := $(sort $(wildcard $(vdrlibsidir)/*.c))
WIRBELSCAN_SOURCES = $(sort $(wildcard $(pluginsrcdir)/wirbelscan/*.cpp))
SATIP_SOURCES      = $(sort $(wildcard $(pluginsrcdir)/satip/*.c))



OBJS            = $(SOURCES:.cpp=.o)
VDR_OBJS        = $(VDR_SOURCES:.c=.o)
LIBSI_OBJS      = $(LIBSI_SOURCES:.c=.o)
WIRBELSCAN_OBJS = $(WIRBELSCAN_SOURCES:.cpp=.o)
SATIP_OBJS      = $(SATIP_SOURCES:.c=.o)


VERSION = $(shell date +%Y%m%d)
PACKAGE = $(package_name)-$(VERSION)
MACHINE = $(shell uname -m )









#/******************************************************************************
# * color definitions, RST=reset, CY=cyan, MG=magenta, BL=blue, (..)
# *****************************************************************************/
RST=\e[0m
CY=\e[1;36m
MG=\e[1;35m
BL=\e[1;34m
YE=\e[1;33m
RD=\e[1;31m
GN=\e[1;32m




#/******************************************************************************
# * targets starting from here:
# *****************************************************************************/

%.o: %.c
ifeq ($(CXX),@g++)
	@echo -e "${CY} CXX $@${RST}"
endif
	$(CXX) $(CFLAGS) $(CPPFLAGS) -Wno-parentheses -Wno-unused-parameter -c $(DEFINES) $(INCLUDES) -o $@ $<

%.o: %.cpp
ifeq ($(CXX),@g++)
	@echo -e "${BL} CXX $@${RST}"
endif
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

$(BINARY): .dependencies $(LIBSI_OBJS) $(VDR_OBJS) $(WIRBELSCAN_OBJS) $(SATIP_OBJS) $(OBJS)
ifeq ($(CXX),@g++)
	@echo -e "${GN} LINK $(BINARY)${RST}"
endif
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LIBSI_OBJS) $(VDR_OBJS) $(WIRBELSCAN_OBJS) $(SATIP_OBJS) $(OBJS) $(LIBS) -o $(BINARY)

install: $(BINARY)
	$(MKDIR_P) $(DESTDIR)$(bindir)
	$(MKDIR_P) $(DESTDIR)$(docdir)
	$(MKDIR_P) $(DESTDIR)$(man1dir)
	$(INSTALL_PROGRAM) $(BINARY) $(DESTDIR)$(bindir)
	$(INSTALL_DATA) CONTRIBUTORS COPYING HISTORY README $(DESTDIR)$(docdir)
	$(INSTALL_DATA) doc/w_scan_cpp.1 $(DESTDIR)$(man1dir)

uninstall:
	$(RM) -f $(DESTDIR)$(bindir)/$(BINARY)
	$(RM) -rf $(DESTDIR)$(docdir)
	$(RM) -f $(DESTDIR)$(man1dir)/w_scan_cpp.1

.PHONY: download
download: $(vdrdir) $(pluginsrcdir)/satip $(pluginsrcdir)/wirbelscan

$(vdrdir):
	$(GIT) clone git://git.tvdr.de/vdr.git
	@$(RM) -rf $(vdrdir)/.git
	$(MKDIR_P) $(pluginsrcdir)
	$(MKDIR_P) $(pluginlibdir)

$(pluginsrcdir)/satip.git:
	$(CD) $(pluginsrcdir) && $(GIT) clone $(SATIP_GIT_ADDR)
	$(CD) $(pluginsrcdir) && $(LN) -sf vdr-plugin-satip satip

$(pluginsrcdir)/satip: $(pluginsrcdir)/satip.git

$(pluginsrcdir)/wirbelscan:
	$(CD) $(pluginsrcdir) && $(WGET) $(WIRBELSCAN_DL_ADDR)
	$(CD) $(pluginsrcdir) && $(TAR) xf $(WIRBELSCAN_TARBALL) && $(RM) -f $(WIRBELSCAN_TARBALL)
	$(CD) $(pluginsrcdir) && $(LN) -s $(WIRBELSCAN_VERSION) wirbelscan


.PHONY: clean mrproper Version.h
clean:
	@$(RM) -f $(LIBSI_OBJS) $(VDR_OBJS) $(OBJS) $(WIRBELSCAN_OBJS) $(SATIP_OBJS) $(BINARY) .dependencies
	@$(RM) -rf $(vdrdir)/.git
	@$(RM) -rf $(pluginsrcdir)/vdr-plugin-satip/.git
	@$(RM) -rf MakeHeader.bin

mrproper: clean
	@$(RM) -r $(vdrdir)

Version.h:
	@echo "#pragma once" > Version.h
	@echo "#include <string>" >> Version.h
	@echo "/* AUTOMATICALLY GENERATED - DO NOT EDIT MANUALLY */" >> Version.h
	@echo "" >> Version.h
	@echo "const std::string version = \"$(VERSION)\";" >> Version.h
	@$(CHMOD) a-x Version.h

version: Version.h

dist: Version.h mrproper
	@-$(RM) -rf $(tmpdir)/$(PACKAGE)
	@$(MKDIR_P) $(tmpdir)/$(PACKAGE)
	@$(CP) -a * $(tmpdir)/$(PACKAGE)
	@$(TAR) cfj $(PACKAGE).tar.bz2 -C $(tmpdir) $(PACKAGE)
	@-$(RM) -rf $(tmpdir)/$(PACKAGE)
	@echo Distribution package created as $(PACKAGE).tar.bz2

dist-download: clean
	@$(TAR) cfj third_party_$(VERSION).tar.bz2 ./vdr
	@echo third party package created as third_party_$(VERSION).tar.bz2

binary: $(BINARY)
	@$(STRIP) $(BINARY)
	@-$(RM) -rf $(tmpdir)/$(PACKAGE)
	@$(MKDIR_P) $(tmpdir)/$(PACKAGE)
	DESTDIR=$(tmpdir)/$(PACKAGE) prefix=/usr $(MAKE) install
	@$(TAR) cfj $(PACKAGE)-binary-$(MACHINE).tar.bz2 -C $(tmpdir) $(PACKAGE)
	@-$(RM) -rf $(tmpdir)/$(PACKAGE)
	@echo binary package created as $(PACKAGE)-binary-$(MACHINE).tar.bz2


#/******************************************************************************
# * dependencies, check them here and provide message to user.
# *****************************************************************************/
MAKEDEP = $(CXX) -MM -MG

.dependencies: Makefile
ifeq ($(HAVE_LIBJPEG),0)
	@echo "ERROR: dependency not found: $(LIBJPEG)"
	exit 1
endif
ifeq ($(HAVE_FREETYPE2),0)
	@echo "ERROR: dependency not found: $(FREETYPE2)"
	exit 1
endif
ifeq ($(HAVE_FONTCONFIG),0)
	@echo "ERROR: dependency not found: $(FONTCONFIG)"
	exit 1
endif
ifeq ($(HAVE_LIBPUGIXML),0)
	@echo "ERROR: dependency not found: $(LIBPUGIXML)"
	exit 1
endif
ifeq ($(HAVE_LIBREPFUNC),0)
	@echo "ERROR: dependency not found: $(LIBREPFUNC) >= $(LIBREPFUNC_MINVERSION)"
	exit 1
endif
ifeq ($(HAVE_LIBREPFUNC_MINVERSION),0)
	@echo "ERROR: dependency $(LIBREPFUNC) older than $(LIBREPFUNC_MINVERSION)"
	exit 1
endif
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(SOURCES) > $@

-include .dependencies

#/******************************************************************************
# * debug
# *****************************************************************************/
printvars:
	@echo "BINARY             = $(BINARY)"
	@echo "package_name       = $(package_name)"
	@echo "WIRBELSCAN_VERSION = $(WIRBELSCAN_VERSION)"
	@echo "WIRBELSCAN_SOURCES = $(WIRBELSCAN_SOURCES)"
	@echo "WIRBELSCAN_OBJS    = $(WIRBELSCAN_OBJS)"
	@echo "SATIP_GIT_ADDR     = $(SATIP_GIT_ADDR)"
	@echo "HAVE_LIBJPEG       = $(HAVE_LIBJPEG)"
	@echo "HAVE_FREETYPE2     = $(HAVE_FREETYPE2)"
	@echo "HAVE_FONTCONFIG    = $(HAVE_FONTCONFIG)"
	@echo "HAVE_LIBPUGIXML    = $(HAVE_LIBPUGIXML)"
	@echo "HAVE_LIBREPFUNC    = $(HAVE_LIBREPFUNC)"
	@echo "HAVE_LIBREPFUNC_MINVERSION = $(HAVE_LIBREPFUNC_MINVERSION)"
	@echo "CC                 = $(CC)"
	@echo "CXX                = $(CXX)"
	@echo "CFLAGS             = $(CFLAGS)"
	@echo "CXXFLAGS           = $(CXXFLAGS)"
	@echo "CD                 = $(CD)"
	@echo "CP                 = $(CP)"
	@echo "CHMOD              = $(CHMOD)"
	@echo "GIT                = $(GIT)"
	@echo "INSTALL            = $(INSTALL)"
	@echo "INSTALL_PROGRAM    = $(INSTALL_PROGRAM)"
	@echo "INSTALL_DATA       = $(INSTALL_DATA)"
	@echo "LN                 = $(LN)"
	@echo "MAKE               = $(MAKE)"
	@echo "MKDIR              = $(MKDIR)"
	@echo "MKDIR_P            = $(MKDIR_P)"
	@echo "PKG_CONFIG         = $(PKG_CONFIG)"
	@echo "RM                 = $(RM)"
	@echo "SED                = $(SED)"
	@echo "SHELL              = $(SHELL)"
	@echo "STRIP              = $(STRIP)"
	@echo "TAR                = $(TAR)"
	@echo "WGET               = $(WGET)"
	@echo "tmpdir             = $(tmpdir)"
	@echo "prefix             = $(prefix)"
	@echo "exec_prefix        = $(exec_prefix)"
	@echo "bindir             = $(bindir)"
	@echo "sbindir            = $(sbindir)"
	@echo "libexecdir         = $(libexecdir)"
	@echo "datarootdir        = $(datarootdir)"
	@echo "datadir            = $(datadir)"
	@echo "sysconfdir         = $(sysconfdir)"
	@echo "includedir         = $(includedir)"
	@echo "sharedstatedir     = $(sharedstatedir)"
	@echo "localstatedir      = $(localstatedir)"
	@echo "docdir             = $(docdir)"
	@echo "infodir            = $(infodir)"
	@echo "libdir             = $(libdir)"
	@echo "localedir          = $(localedir)"
	@echo "mandir             = $(mandir)"
	@echo "man1dir            = $(man1dir)"
	@echo "man2dir            = $(man2dir)"
	@echo "man3dir            = $(man3dir)"
	@echo "man4dir            = $(man4dir)"
	@echo "man5dir            = $(man5dir)"
	@echo "srcdir             = $(srcdir)"
	@echo "vdrdir             = $(srcdir)/vdr"
	@echo "vdrlibsidir        = $(vdrlibsidir)"
	@echo "pluginsrcdir       = $(pluginsrcdir)"
	@echo "pluginlibdir       = $(pluginlibdir)"
	@echo "WIRBELSCAN_TARBALL = $(WIRBELSCAN_TARBALL)"
	@echo "WIRBELSCAN_DL_ADDR = $(WIRBELSCAN_DL_ADDR)"
	@echo "APIVERSION         = $(APIVERSION)"
	@echo "DEFINES            = $(DEFINES)"
	@echo "LIBS               = $(LIBS)"
	@echo "INCLUDES           = $(INCLUDES)"
	@echo "LDFLAGS            = $(LDFLAGS)"
	@echo "VERSION            = $(VERSION)"
	@echo "PACKAGE            = $(PACKAGE)"
	@echo "MACHINE            = $(MACHINE)"
	@echo "MAKEDEP            = $(MAKEDEP)"
