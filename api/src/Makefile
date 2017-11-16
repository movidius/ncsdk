ARCH := $(shell uname -m)

LIBS += -lpthread -lusb-1.0 -ldl

OUT := libmvnc.so.0
OBJDIR := obj-$(ARCH)
INSTALLDIR := ${DESTDIR}/usr/local
PYTHON3DIST := $(shell python3 -c "import site; print(site.getsitepackages()[0])")
PYTHON2DIST := $(shell python -c "import site; print(site.getsitepackages()[0])")

SRCS := \
	usb_boot.c \
	usb_link_vsc.c \
	mvnc_api.c

INCLUDES := \
	-I. \
	-I../include \
	-I$(SYSROOT)/usr/include/libusb-1.0 \

CFLAGS += -O2 -Wall -pthread -fPIC -MMD -MP
LDFLAGS += -shared

OBJS := $(SRCS:%.c=$(OBJDIR)/%.o)
DEPS := $(OBJS:.o=.d)

all: obj-$(ARCH)/libmvnc.so.0

$(OBJDIR)/$(OUT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)
	ln -fs $(OBJDIR)/$(OUT) libmvnc.so
	ln -fs $(OBJDIR)/$(OUT) $(OUT)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR):
	@mkdir $@

-include $(DEPS)

basicinstall: $(OBJDIR)/$(OUT)
	mkdir -p $(INSTALLDIR)/include/
	mkdir -p $(INSTALLDIR)/lib/
	cp $(OBJDIR)/$(OUT) $(INSTALLDIR)/lib/
	ln -fs libmvnc.so.0 $(INSTALLDIR)/lib/libmvnc.so
	cp ../include/*.h $(INSTALLDIR)/include/
	mkdir -p $(INSTALLDIR)/lib/mvnc
	cp mvnc/MvNCAPI.mvcmd $(INSTALLDIR)/lib/mvnc/
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
	rm -f $(INSTALLDIR)/include/mvnc_deprecated.h
	rm -f $(INSTALLDIR)/lib/mvnc/MvNCAPI.mvcmd
	rm -rf $(INSTALLDIR)/lib/mvnc
	rm -rf ${DESTDIR}$(PYTHON3DIST)/mvnc
	rm -rf ${DESTDIR}$(PYTHON2DIST)/mvnc
	rm -f ${DESTDIR}/etc/udev/rules.d/97-usbboot.rules

clean:
	rm -f $(OUT)
	rm -f $(OBJS)
	rm -rf $(OBJDIR)
	rm -f libmvnc.so
