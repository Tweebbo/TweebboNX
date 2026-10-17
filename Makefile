#---------------------------------------------------------------------------------
# Tweebbo for Nintendo Switch (showcase proof of concept).
# Build with ./build.sh (uses the devkitpro/devkita64 container).
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. Use ./build.sh")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

TARGET      :=  tweebbo
BUILD       :=  build
SOURCES     :=  source source/gen source/nx
INCLUDES    :=  source
ROMFS       :=  romfs
APP_TITLE   :=  Tweebbo Showcase
APP_AUTHOR  :=  Tweebbo
APP_VERSION :=  $(shell cat $(TOPDIR)/VERSION)

ARCH    :=  -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS  :=  -g -Wall -O2 -ffunction-sections $(ARCH) $(DEFINES)
CFLAGS  +=  $(INCLUDE) -D__SWITCH__ -D_GNU_SOURCE -DCONFIG_VERSION='"2026-06-04"'
CFLAGS  +=  -DAPP_VERSION='"$(APP_VERSION)"' -DTWEEBBO_DEVICE=2
# The hotel on the local network this proof of concept plays on (set by build.sh).
CFLAGS  +=  -DTWEEBBO_LAN_ONLY=1 -DDEFAULT_SERVER='"$(TWEEBBO_SERVER)"' 
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS :=  -g $(ARCH)
LDFLAGS  =  -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS    :=  -lSDL2_image -lSDL2 -lEGL -ldrm_nouveau -lGLESv2 -lglapi -lstdc++ -lmbedtls -lmbedx509 -lmbedcrypto -lwebp -lpng16 -ljpeg -lz -lnx -lm

LIBDIRS :=  $(PORTLIBS) $(LIBNX)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT     :=  $(CURDIR)/$(TARGET)
export TOPDIR     :=  $(CURDIR)
export VPATH      :=  $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR    :=  $(CURDIR)/$(BUILD)
CFILES            :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES          :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES            :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES          :=  $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export LD         :=  $(CC)
export OFILES     :=  $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE    :=  $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                      $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                      $(foreach dir,$(PORTLIBS),-I$(dir)/include/SDL2) \
                      -I$(CURDIR)/$(BUILD)
export LIBPATHS   :=  $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export APP_ICON   :=  $(TOPDIR)/assets/icon.jpg
export NROFLAGS   +=  --icon=$(APP_ICON) --nacp=$(CURDIR)/$(TARGET).nacp
export NROFLAGS   +=  --romfsdir=$(CURDIR)/$(ROMFS)

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf

else

DEPENDS := $(OFILES:.o=.d)

all: $(OUTPUT).nro

$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp

$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif
#---------------------------------------------------------------------------------
