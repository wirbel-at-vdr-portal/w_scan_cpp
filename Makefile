#/******************************************************************************
# * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
# * Plugins.
# *
# * See the README file for copyright information and how to reach the author.
# *****************************************************************************/
BINARY = w_scan_cpp

WIRBELSCAN_VERSION = wirbelscan-2021.03.07

SATIP_GIT_ADDR = https://github.com/rofafor/vdr-plugin-satip




#/******************************************************************************
# * if you are still running a distro, not beeing able to use all valid
# * standard ASCII Characters (32-126) for package names, and prefer to modify
# * packages instead of fixing your broken distro, you may overwrite the
# * package_name here.
# *****************************************************************************/
package_name ?= $(BINARY)






#/******************************************************************************
# * if you prefer verbose non-coloured build messages, remove the '@' here:
# *****************************************************************************/
CC  = @gcc
CXX = @g++


CFLAGS    = -g -O3 -fPIC -Werror=overloaded-virtual
CXXFLAGS  = -g -O3 -fPIC -Wall -Wextra -Werror=overloaded-virtual -Wfatal-errors



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
# datadir’s default value is based on this variable; so are infodir, mandir,
# and others. 
datarootdir = $(prefix)/share

#------------------------
# directory for installing idiosyncratic read-only architecture-independent
# data files for this program. This is usually the same place as ‘datarootdir’,
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
man1dir = $(datarootdir)/man1
man2dir = $(datarootdir)/man2
man3dir = $(datarootdir)/man3
man4dir = $(datarootdir)/man4
man5dir = $(datarootdir)/man5

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
LIBS      = -ljpeg -lpthread -lcap -ldl -lrt $(shell $(PKG_CONFIG) --libs freetype2 fontconfig)
LIBS     += $(shell curl-config --libs) -lpugixml
INCLUDES  = -I$(srcdir)
INCLUDES += -I$(vdrdir)
INCLUDES += -I$(pluginsrcdir)
INCLUDES +=  $(shell $(PKG_CONFIG) --cflags freetype2 fontconfig)
LDFLAGS  ?= -L$(srcdir) -L$(vdrdir) -L$(vdrlibsidir)




SOURCES           := $(wildcard $(srcdir)/*.cpp)
VDR_SOURCES       := $(shell find $(vdrdir)  -maxdepth 1 ! -name "vdr.c" -name "*.c" )
LIBSI_SOURCES     := $(wildcard $(vdrlibsidir)/*.c)
WIRBELSCAN_SOURCES = $(wildcard $(pluginsrcdir)/wirbelscan/*.c)
SATIP_SOURCES      = $(wildcard $(pluginsrcdir)/satip/*.c)



OBJS            = $(SOURCES:.cpp=.o)
VDR_OBJS        = $(VDR_SOURCES:.c=.o)
LIBSI_OBJS      = $(LIBSI_SOURCES:.c=.o)
WIRBELSCAN_OBJS = $(WIRBELSCAN_SOURCES:.c=.o)
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
	$(CXX) $(CFLAGS) -Wno-parentheses -Wno-unused-parameter -c $(DEFINES) $(INCLUDES) -o $@ $<

%.o: %.cpp
ifeq ($(CXX),@g++)
	@echo -e "${BL} CXX $@${RST}"
endif
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

all: $(LIBSI_OBJS) $(VDR_OBJS) $(WIRBELSCAN_OBJS) $(SATIP_OBJS) $(OBJS)
ifeq ($(CXX),@g++)
	@echo -e "${GN} LINK $(BINARY)${RST}"
endif
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBSI_OBJS) $(VDR_OBJS) $(WIRBELSCAN_OBJS) $(SATIP_OBJS) $(OBJS) $(LIBS) -o $(BINARY)

install: all
	$(MKDIR_P) $(DESTDIR)$(bindir)
	$(MKDIR_P) $(DESTDIR)$(docdir)
	$(INSTALL_PROGRAM) $(BINARY) $(DESTDIR)$(bindir)
	$(INSTALL_DATA) COPYING HISTORY README $(DESTDIR)$(docdir)

uninstall:
	$(RM) -f $(DESTDIR)$(bindir)/$(BINARY)
	$(RM) -rf $(DESTDIR)$(docdir)

.PHONY: download
download: $(vdrdir) $(pluginsrcdir)/satip $(pluginsrcdir)/wirbelscan

$(vdrdir):
	$(GIT) clone git://git.tvdr.de/vdr.git
	@$(RM) -rf $(vdrdir)/.git
	$(MKDIR_P) $(pluginsrcdir)
	$(MKDIR_P) $(pluginlibdir)

#/******************************************************************************
# * generate a header for satip.
# * see file ./vdr/PLUGINS/src/satip/satip.c and compare to
# * https://stackoverflow.com/questions/4857424/extract-lines-between-2-tokens-in-a-text-file-using-bash
# *****************************************************************************/
SATIP_HDR__FROM = trNOOP("SAT>IP Devices");
SATIP_HDR__TO  = cPluginSatip\:\:

$(pluginsrcdir)/satip/satip.h:
	$(SED) -n '/$(SATIP_HDR__FROM)/{:a;n;/$(SATIP_HDR__TO)/b;p;ba}' $(pluginsrcdir)/satip/satip.c > $(pluginsrcdir)/satip/satip.h
	$(SED) -i 's/VERSION/SATIP_VERSION/' $(pluginsrcdir)/satip/satip.h
	$(SED) -i 's/DESCRIPTION/SATIP_DESCRIPTION/' $(pluginsrcdir)/satip/satip.h
	$(SED) -i '1i\\nclass cSatipDiscoverServers;\n' $(pluginsrcdir)/satip/satip.h
	$(SED) -i '1i\extern const char SATIP_DESCRIPTION[];' $(pluginsrcdir)/satip/satip.h
	$(SED) -i '1i\extern const char SATIP_VERSION[];' $(pluginsrcdir)/satip/satip.h
	$(SED) -i '1i\#include <vdr\/plugin.h>\n' $(pluginsrcdir)/satip/satip.h
	$(SED) -i '1i\#pragma once' $(pluginsrcdir)/satip/satip.h
	$(SED) -i 's/VDRPLUGINCREATOR/\/\/VDRPLUGINCREATOR/g' $(pluginsrcdir)/satip/satip.c
	$(SED) -i 's/class cPluginSatip/\#include \"satip.h\"\n \/* class cPluginSatip/' $(pluginsrcdir)/satip/satip.c
	$(SED) -i 's/cPluginSatip\:\:cPluginSatip(void)/*\/\ncPluginSatip\:\:cPluginSatip(void)/' $(pluginsrcdir)/satip/satip.c
	$(SED) -i 's/const char VERSION\[\]/const char SATIP_VERSION\[\]/' $(pluginsrcdir)/satip/satip.c
	$(SED) -i 's/static const char DESCRIPTION\[\]/const char SATIP_DESCRIPTION\[\]/' $(pluginsrcdir)/satip/satip.c
	$(SED) -i 's/const char VERSION\[\]/const char SATIP_VERSION\[\]/' $(pluginsrcdir)/satip/common.h
	$(SED) -i 's/VERSION/SATIP_VERSION/g' $(pluginsrcdir)/satip/discover.c
	$(SED) -i 's/VERSION/SATIP_VERSION/g' $(pluginsrcdir)/satip/rtsp.c


$(pluginsrcdir)/satip.git:
	$(CD) $(pluginsrcdir) && $(GIT) clone $(SATIP_GIT_ADDR)
	$(CD) $(pluginsrcdir) && $(LN) -sf vdr-plugin-satip satip


$(pluginsrcdir)/satip: $(pluginsrcdir)/satip.git $(pluginsrcdir)/satip/satip.h


$(pluginsrcdir)/wirbelscan:
	$(CD) $(pluginsrcdir) && $(WGET) $(WIRBELSCAN_DL_ADDR)
	$(CD) $(pluginsrcdir) && $(TAR) xf $(WIRBELSCAN_TARBALL) && $(RM) -f $(WIRBELSCAN_TARBALL)
	$(CD) $(pluginsrcdir) && $(LN) -s $(WIRBELSCAN_VERSION) wirbelscan


.PHONY: clean mrproper Version.h
clean:
	@$(RM) -f $(LIBSI_OBJS) $(VDR_OBJS) $(OBJS) $(WIRBELSCAN_OBJS) $(SATIP_OBJS) $(BINARY)
	@$(RM) -rf $(vdrdir)/.git
	@$(RM) -rf $(pluginsrcdir)/vdr-plugin-satip/.git

mrproper: clean
	@$(RM) -r $(vdrdir)

Version.h:
	@echo "#pragma once" > Version.h
	@echo "#include <string>" >> Version.h
	@echo "/* AUTOMATICALLY GENERATED - DO NOT EDIT MANUALLY */" >> Version.h
	@echo "" >> Version.h
	@echo "const std::string version = \"$(VERSION)\";" >> Version.h
	@$(CHMOD) a-x Version.h

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
# * debug
# *****************************************************************************/
printvars:
	@echo "BINARY             = $(BINARY)"
	@echo "package_name       = $(package_name)"
	@echo "WIRBELSCAN_VERSION = $(WIRBELSCAN_VERSION)"
	@echo "SATIP_GIT_ADDR     = $(SATIP_GIT_ADDR)"
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
