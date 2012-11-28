# Adapted from libopencm3 examples and wombat makefiles

BINARY = dormouse
PREFIX = arm-none-eabi
TOOLCHAIN_DIR = /opt/arm-toolchain
CFLAGS += -O2
OBJS = $(patsubst %.c,%.o,$(wildcard src/*.c)) \
	   $(patsubst %.c,%.o,$(wildcard sd_lib/*.c))
LDSCRIPT = src/dormouse.ld

##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
## Copyright (C) 2010 Piotr Esden-Tempski <piotr@esden.net>
## Modifications made by Daniel Richman
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

PREFIX			?= arm-none-eabi
#PREFIX			?= arm-elf
CC				= $(PREFIX)-gcc
LD				= $(PREFIX)-gcc
OBJCOPY			= $(PREFIX)-objcopy
OBJDUMP			= $(PREFIX)-objdump
SIZE			= $(PREFIX)-size
GDB				= $(PREFIX)-gdb

ARCH_FLAGS      = -mthumb -mcpu=cortex-m3 -msoft-float
CFLAGS			+= -g -Wall -Wextra -Werror \
				   -fno-common $(ARCH_FLAGS) -MD -DSTM32F1 \
				   -DSTM32F10X_XL
#				   -Os
#				   -I$(TOOLCHAIN_DIR)/include
LDSCRIPT		?= $(BINARY).ld
LDFLAGS			+= --static \
				   -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group \
				   -T$(LDSCRIPT) -nostartfiles -Wl,--gc-sections \
				   $(ARCH_FLAGS) -mfix-cortex-m3-ldrd
#				   -L$(TOOLCHAIN_DIR)/lib

OOCD			?= openocd
OOCD_INTERFACE	?= flossjtag
OOCD_BOARD		?= olimex_stm32_h103
# Black magic probe specific variables
# Set the BMP_PORT to a serial port and then BMP is used for flashing
BMP_PORT        ?=
# texane/stlink can be used by uncommenting this...
# or defining it in your own makefiles
#STLINK_PORT ?= :4242

all: images

images: $(BINARY).images
flash: $(BINARY).flash

$(BINARY).images : $(BINARY).bin $(BINARY).hex $(BINARY).srec $(BINARY).list \
					$(BINARY).sizes

$(BINARY).bin : $(BINARY).elf
	$(OBJCOPY) -Obinary $(BINARY).elf $(BINARY).bin

$(BINARY).hex : $(BINARY).elf
	$(OBJCOPY) -Oihex $(BINARY).elf $(BINARY).hex

$(BINARY).srec : $(BINARY).elf
	$(OBJCOPY) -Osrec $(BINARY).elf $(BINARY).srec

$(BINARY).list : $(BINARY).elf
	$(OBJDUMP) -S $(BINARY).elf > $(BINARY).list

$(BINARY).sizes : $(BINARY).elf $(OBJS)
	$(SIZE) $(BINARY).elf $(OBJS) > $(BINARY).sizes

$(BINARY).elf : $(OBJS) $(LDSCRIPT)
	$(LD) -o $@ $(OBJS) -lopencm3_stm32f1 $(LDFLAGS) \
		-Wl,-Map,$(BINARY).map

sd_lib/stm32_eval_sdio_sd.o : sd_lib/stm32_eval_sdio_sd.c
	$(CC) $(CFLAGS) -Wno-unused-parameter -Wno-unused-variable -o $@ -c $<

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJS)
	rm -f $(OBJS:.o=.d)
	rm -f $(BINARY).elf
	rm -f $(BINARY).bin
	rm -f $(BINARY).hex
	rm -f $(BINARY).srec
	rm -f $(BINARY).list
	rm -f $(BINARY).map

ifeq ($(STLINK_PORT),)
ifeq ($(BMP_PORT),)
ifeq ($(OOCD_SERIAL),)
%.flash: %.hex
	# IMPORTANT: Don't use "resume", only "reset" will work correctly!
	$(OOCD) -f interface/$(OOCD_INTERFACE).cfg \
		    -f board/$(OOCD_BOARD).cfg \
		    -c "init" -c "reset init" \
		    -c "stm32f1x mass_erase 0" \
		    -c "flash write_image $(BINARY).hex" \
		    -c "reset" \
		    -c "shutdown" $(NULL)
else
%.flash: %.hex
	# IMPORTANT: Don't use "resume", only "reset" will work correctly!
	$(OOCD) -f interface/$(OOCD_INTERFACE).cfg \
		    -f board/$(OOCD_BOARD).cfg \
		    -c "ft2232_serial $(OOCD_SERIAL)" \
		    -c "init" -c "reset init" \
		    -c "stm32f1x mass_erase 0" \
		    -c "flash write_image $(BINARY).hex" \
		    -c "reset" \
		    -c "shutdown" $(NULL)
endif
else
%.flash: %.elf
	$(GDB) --batch \
		   -ex 'target extended-remote $(BMP_PORT)' \
		   -x $(TOOLCHAIN_DIR)/scripts/black_magic_probe_flash.scr \
		   $(BINARY).elf
endif
else
%.flash: %.elf
	$(GDB) --batch \
		   -ex 'target extended-remote $(STLINK_PORT)' \
		   -x $(SCRIPT_DIR)/libopencm3/scripts/stlink_flash.scr \
		   $(BINARY).elf
endif

.PHONY: images clean

-include $(OBJS:.o=.d)

