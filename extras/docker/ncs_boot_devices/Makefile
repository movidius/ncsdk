MVNC_API_PATH=$(realpath ../../../api/src/)
ifdef $$MVNC_API_PATH
MVNC_API_PATH := $$MVNC_API_PATH
endif

VERSION_NUMBER=`cat $(MVNC_API_PATH)/version.txt`

SRCS := ./ncs_boot_devices.c
SRCS += \
	$(MVNC_API_PATH)/mvnc_api.c \
	$(MVNC_API_PATH)/fp16.c

MV_COMMON_BASE=$(realpath $(MVNC_API_PATH)/common)
XLINK_BASE=$(MV_COMMON_BASE)/components/XLink

XLINK_SRCS=$(XLINK_BASE)/pc/usb_boot.c
XLINK_SRCS+=$(XLINK_BASE)/shared/XLink.c
XLINK_SRCS+=$(XLINK_BASE)/shared/XLinkDispatcher.c
XLINK_SRCS+=$(XLINK_BASE)/pc/XLinkPlatform.c

XLINK_INCLUDES= -I$(XLINK_BASE)/shared \
			-I$(XLINK_BASE)/pc \
			-I$(MV_COMMON_BASE)/shared/include/

CFLAGS+=-D__PC__ -DUSE_USB_VSC -DXLINK_NO_RESET -DVERSION_NAME="\"$(VERSION_NUMBER)\""
CFLAGS+=-DDEVICE_SHELL_ENABLED
CFLAGS+=-DEXCLUDE_HIGHCLASS

LIBS += -lpthread -lusb-1.0 -ldl

INCLUDES := \
	-I$(MVNC_API_PATH)/ \
	-I$(MVNC_API_PATH)/../include \
	-I$(SYSROOT)/usr/include/libusb-1.0 \

INCLUDES+= $(XLINK_INCLUDES)
CFLAGS+= -O2 -Wall -pthread -fPIC -MMD -MP $(INCLUDES)

SRCS += $(XLINK_SRCS)
OBJS := $(SRCS:%.c=%.o)

.PHONY: all
all: ncs_boot_devices

.PHONY:	ncs_boot_devices
ncs_boot_devices: $(OBJS)
ifeq ($(MVNC_API_PATH),)
	echo "Error: MVNC_API_PATH not defined! Please define and rerun!"
	exit 1
endif
	@echo "\nmaking ncs_boot_devices"
	@$(CC) -o $@ $^  $(CFLAGS) $(LIBS)
	if [ ! -e  $(MVNC_API_PATH)/mvnc/*.mvcmd ];then \
		cd $(MVNC_API_PATH); make get_mvcmd; \
	fi
	@ln -fs $(MVNC_API_PATH)/mvnc/

.PHONY: run
run: ncs_boot_devices
	@echo "\nmaking run"
	./ncs_boot_devices;

.PHONY: help
help:
	@echo "possible make targets: ";
	@echo "  make help - shows this message";
	@echo "  make all - same as ncs_boot_devices";
	@echo "  make ncs_boot_devices - builds the ncs_boot_devices executable example";
	@echo "  make run - runs the ncs_boot_devices executable example program";
	@echo "  make clean - removes all created content and temporary files";

clean:
	@echo "\nmaking clean";
	rm $(OBJS)
	rm -f ncs_boot_devices;
