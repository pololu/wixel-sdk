DEFAULT_LIBRARIES = radio_com.lib radio_link.lib radio_mac.lib radio_registers.lib \
  random.lib uart.lib usb.lib usb_cdc_acm.lib wixel.lib adc.lib gpio.lib dma.lib

# This template defines the things we want to add to the makefile for each library.
define LIB_template

LIB_RELS := $(patsubst %.c,%.rel, $(wildcard libraries/src/$(1)/*.c)) $(patsubst %.s,%.rel, $(wildcard libraries/src/$(1)/*.s))
-include libraries/src/$(1)/lib_options.mk

RELs += $$(LIB_RELS)
LIBs += libraries/lib/$(1).lib

libraries/lib/$(1).lib : $$(LIB_RELS)

.PHONY : lib_$(1)
lib_$(1) : libraries/lib/$(1).lib

endef

# Auto detect the libs, and store the list of app names in the APPs variable.
AUTOLIBs := $(foreach lib, $(wildcard libraries/src/*),$(notdir $(lib)))

# Add information about each lib to the Makefile.
$(foreach lib, $(AUTOLIBs), $(eval $(call LIB_template,$(lib))))

# Make a phony target called "libs" which builds all the libraries.
# You can type "make libs" to build all the libraries.
.PHONY : libs
libs: $(LIBs)

# You can type "make docs" to build the docs.
.PHONY : docs
docs:
	cd libraries && doxygen
	cp libraries/docs/docs.html docs/docs.html