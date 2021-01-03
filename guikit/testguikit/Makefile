DEBUG ?= 0
name := pigui
prefix := $(HOME)/.local

ifeq ($(platform),)

    ifneq (,$(filter Windows%,$(OS)))
	platform := windows
    else
	uname := $(shell uname -a)

	ifeq ($(uname),)
	    platform := windows
	else ifneq ($(findstring Msys,$(uname)),)
	    platform := windows
	else ifneq ($(findstring MINGW,$(uname)),)
	    platform := windows
	else ifneq ($(findstring Windows,$(uname)),)
	    platform := windows
	else ifneq ($(findstring CYGWIN,$(uname)),)
	    platform := windows
	else ifneq ($(findstring Darwin,$(uname)),)
	    platform := macosx
	else
	    platform := x
	endif
    endif
endif

ifeq ($(platform),windows)
    SHELL := cmd
    delete = del $(subst /,\,$1)
else
    delete = rm -f $1
endif

#compiler assignment
ifeq ($(compiler),)
    compiler := g++
    ifeq ($(platform),macosx)
	compiler := clang++
    endif
endif

ifeq ($(platform),macosx)
    cppflags := -x c++ -std=c++14
    objcppflags := -x objective-c++ -std=c++14
else	
    cppflags := -x c++ -std=c++17
endif

objects := main guikit

flags :=
link :=

ifeq ($(platform),windows)
    link += -static
else ifeq ($(platform),macosx)
    flags += -w -stdlib=libc++ -march=native
    link += -lc++ -lobjc
else
    flags += -march=native
endif

ifeq ($(DEBUG), 0)
    flags += -O3

    ifeq ($(platform),windows)
	link += -mwindows -s
    endif
else
    flags += -O0 -g3 -Wall

    ifeq ($(platform),windows)
	link += -mconsole
    endif
endif

compile = \
    $(strip \
	$(compiler) $(cppflags) $(flags) $1 -c $< -o $@ \
    )

include ../Makefile
link += $(uilink)


#rule:
#	@echo $(OS)

all: build;

ifeq ($(platform),windows)
objects += resource
obj/resource.o: data/resource.rc
	windres data/resource.rc obj/resource.o
endif

%.o: $<; $(call compile)

obj/guikit.o: ../api.cpp
	$(compiler) $(uiflags) $(flags) -c $< -o $@

obj/main.o:	main.cpp

objects := $(patsubst %,obj/%.o,$(objects))

build: $(objects)
    ifeq ($(platform),macosx)
	if [ -d out/$(name).app ]; then rm -r out/$(name).app; fi
	mkdir out/$(name).app
	mkdir out/$(name).app/Contents
	mkdir out/$(name).app/Contents/MacOS
	mkdir out/$(name).app/Contents/Resources

	cp data/Info.plist out/$(name).app/Contents/Info.plist

	sips -s format icns data/$(name).png --out out/$(name).app/Contents/Resources/$(name).icns
	$(strip $(compiler) -o out/$(name).app/Contents/MacOS/$(name) $(objects) $(link))
    else
	$(strip $(compiler) -o out/$(name) $(objects) $(link))
    endif

clean:
	-@$(call delete,obj/*.o)

install:
    ifeq ($(platform),windows)
    else ifeq ($(platform),macosx)
    else
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/applications/

	install -D -m 755 out/$(name) $(prefix)/bin/$(name)
	install -D -m 644 data/$(name).png $(prefix)/share/icons/$(name).png
	install -D -m 644 data/$(name).desktop $(prefix)/share/applications/$(name).desktop
    endif

uninstall:
    ifeq ($(platform),windows)
    else ifeq ($(platform),macosx)
    else
	if [ -f $(prefix)/bin/$(name) ]; then rm $(prefix)/bin/$(name); fi
	if [ -f $(prefix)/share/icons/$(name).png ]; then rm $(prefix)/share/icons/$(name).png; fi
	if [ -f $(prefix)/share/applications/$(name).desktop ]; then rm $(prefix)/share/applications/$(name).desktop; fi
    endif
