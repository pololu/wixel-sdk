# This template defines the things we want to add to the makefile for each app.
define APP_template

APP_RELS := $$(patsubst %.c,%.rel, $$(wildcard apps/$(1)/*.c)) $$(patsubst %.s,%.rel, $$(wildcard apps/$(1)/*.s))
APP_LIBS := $$(DEFAULT_LIBRARIES)
-include apps/$(1)/options.mk
APP_LIBS := $$(foreach lib, $$(APP_LIBS), libraries/lib/$$(lib))

RELs += $$(APP_RELS)
HEXs += apps/$(1)/$(1).hex

apps/$(1)/$(1).hex : $$(APP_RELS) $$(APP_LIBS)
	$$(LINK_COMMAND)
	$$(V)mv -f $$(@:%.hex=%.ihx) $$@

.PHONY : $(1)
$(1) : apps/$(1)/$(1).wxl

.PHONY : load_$(1)
load_$(1) : apps/$(1)/$(1).wxl
	$$(WIXELCMD) write $$< $$(S) -a 

.PHONY : open_$(1)
open_$(1) : apps/$(1)/$(1).wxl
	$$(WIXELCONFIG) $$< 
endef

# Auto detect the apps, and store the list of app names in the APPs variable.
APPs := $(foreach app, $(wildcard apps/*),$(notdir $(app)))

# Add information about each app to the Makefile.
$(foreach app, $(APPs), $(eval $(call APP_template,$(app))))

# Make a phony target called "apps" which builds all the apps.
# You can type "make apps" to build all the apps.
.PHONY : apps
apps: $(APPs)

