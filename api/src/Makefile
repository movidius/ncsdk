ARCH := $(shell uname -m)


OUT := libmvnc.so.0
OBJDIR := obj-$(ARCH)
INSTALLDIR := ${DESTDIR}/usr/local
PYTHON3DIST := $(shell python3 -c "import site; print(site.getsitepackages()[0])")
PYTHON2DIST := $(shell python -c "import site; print(site.getsitepackages()[0])")
LINK_HIGHCLASS ?= no
VERSION_NUMBER=`cat ./version.txt`
SRCS := \
	mvnc_api.c \
	fp16.c


MV_COMMON_BASE=$(realpath ./common)
XLINK_BASE=$(MV_COMMON_BASE)/components/XLink
XLINKCONSOLE_BASE=$(MV_COMMON_BASE)/components/XLinkConsole

XLINK_SRCS=$(XLINK_BASE)/pc/usb_boot.c
XLINK_SRCS+=$(XLINK_BASE)/shared/XLink.c
XLINK_SRCS+=$(XLINK_BASE)/shared/XLinkDispatcher.c
XLINK_SRCS+=$(XLINKCONSOLE_BASE)/pc/XLinkConsole.c
XLINK_SRCS+=$(XLINK_BASE)/pc/UsbLinkPlatform.c

XLINK_CFLAGS= -I$(XLINK_BASE)/shared \
			-I$(XLINK_BASE)/pc \
			-I$(XLINKCONSOLE_BASE)/pc \
			-I $(MV_COMMON_BASE)/swCommon/include/ \
			-I $(MV_COMMON_BASE)/shared/include/

CFLAGS+=$(XLINK_CFLAGS) -D__PC__ -DUSE_USB_VSC -DVERSION_NAME="\"$(VERSION_NUMBER)\""
#CFLAGS+=-DXLINK_NO_BOOT
CFLAGS+=-DDEVICE_SHELL_ENABLED

LIBS += -lpthread -lusb-1.0 -ldl

#Allow compiling without highclass
ifeq ($(LINK_HIGHCLASS),no)
CFLAGS+= -DEXCLUDE_HIGHCLASS
else
LIBS+= -lmvnc_highclass -L$(INSTALLDIR)/lib/
endif
INCLUDES := \
	-I. \
	-I../include \
	-I$(SYSROOT)/usr/include/libusb-1.0 \

CFLAGS += -O2 -Wall -pthread -fPIC -MMD -MP
LDFLAGS += -shared

OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)
OBJS += $(XLINK_SRCS:%.c=$(OBJDIR)/%.o)
DEPS := $(OBJS:.o=.d)
all: obj-$(ARCH)/libmvnc.so.0

$(OBJDIR)/$(OUT): $(OBJS)
	if [ ! -e ./version.txt ] ; then echo "missing version.txt file"; exit 1; fi;
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)
	ln -fs $(OBJDIR)/$(OUT) libmvnc.so
	ln -fs $(OBJDIR)/$(OUT) $(OUT)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

-include $(DEPS)

basicinstall: $(OBJDIR)/$(OUT)
	mkdir -p $(INSTALLDIR)/include/
	mkdir -p $(INSTALLDIR)/include/mvnc2
	mkdir -p $(INSTALLDIR)/lib/
	cp $(OBJDIR)/$(OUT) $(INSTALLDIR)/lib/
	ln -fs libmvnc.so.0 $(INSTALLDIR)/lib/libmvnc.so
	cp ../include/mvnc.h $(INSTALLDIR)/include/mvnc2
	ln -fs $(INSTALLDIR)/include/mvnc2/mvnc.h $(INSTALLDIR)/include/mvnc.h 
	mkdir -p $(INSTALLDIR)/lib/mvnc
	cp mvnc/MvNCAPI-*.mvcmd $(INSTALLDIR)/lib/mvnc/
	mkdir -p ${DESTDIR}/etc/udev/rules.d/
	cp 97-usbboot.rules ${DESTDIR}/etc/udev/rules.d/

pythoninstall:
	mkdir -p ${DESTDIR}$(PYTHON3DIST)
	mkdir -p ${DESTDIR}$(PYTHON2DIST)
	cp -r ../python/mvnc ${DESTDIR}$(PYTHON3DIST)/
	cp -r ../python/mvnc ${DESTDIR}$(PYTHON2DIST)/

postinstall:
	udevadm control --reload-rules
	udevadm trigger
	ldconfig

install: basicinstall pythoninstall postinstall

uninstall:
	rm -f $(INSTALLDIR)/lib/libmvnc.so.0
	rm -f $(INSTALLDIR)/lib/libmvnc.so
	rm -f $(INSTALLDIR)/include/mvnc.h
	rm -f $(INSTALLDIR)/include/mvnc2/mvnc.h
	rm -f $(INSTALLDIR)/lib/mvnc/MvNCAPI-*.mvcmd
#	rm -rf $(INSTALLDIR)/lib/mvnc
	rm -rf ${DESTDIR}$(PYTHON3DIST)/mvnc
	rm -rf ${DESTDIR}$(PYTHON2DIST)/mvnc
	rm -f ${DESTDIR}/etc/udev/rules.d/97-usbboot.rules

clean:
	rm -f $(OUT)
	rm -f $(OBJS)
	rm -rf $(OBJDIR)
	rm -f libmvnc.so
