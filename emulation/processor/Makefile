# gprof := 1

DEBUG ?= 1
name := CpuTester

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
    copy = xcopy /I /Y $(subst /,\,$1) $(subst /,\,$2)
else
    delete = rm -rf $1
    copy = rm -rf $2 && cp -r $1 $2
endif

ifeq ($(compiler),)
    compiler := g++
    ifeq ($(platform),macosx)
	compiler := clang++
    endif
endif

cppflags := -x c++ -std=c++11

objects := main m6502 m6510 m68base m68000 m68010 m68020 results tester

flags := -I m65xx/m6502/
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
	link += -s
    endif
else
    flags += -O0 -g
endif

ifeq ($(gprof), 1)
    link += -pg
endif

compile = \
    $(strip \
	$(compiler) $(cppflags) $(flags) $1 -c $< -o $@ \
    )

#rule:
#	@echo $(OS)

all: build;


%.o: $<; $(call compile)

obj/main.o:	main.cpp
obj/m6502.o:	m65xx/m6502/m6502.cpp
obj/m6510.o:	m65xx/m6510/m6510.cpp
obj/m68base.o:	m680x0/base/base.cpp
obj/m68000.o:	m680x0/m68000/m68000.cpp
obj/m68010.o:	m680x0/m68010/m68010.cpp
obj/m68020.o:	m680x0/m68020/m68020.cpp
obj/results.o:	m680x0/unittest/results.cpp
obj/tester.o:	m680x0/unittest/tester.cpp



objects := $(patsubst %,obj/%.o,$(objects))

build: $(objects)
    ifeq ($(platform),macosx)
	if [ -d out/$(name).app ]; then rm -r out/$(name).app; fi
	mkdir out/$(name).app
	mkdir out/$(name).app/Contents
	mkdir out/$(name).app/Contents/MacOS

	$(strip $(compiler) -o out/$(name).app/Contents/MacOS/$(name) $(objects) $(link))
    else
	$(strip $(compiler) -o out/$(name) $(objects) $(link))
    endif

clean:
	-@$(call delete,obj/*.o)
	-@$(call delete,out/$(name)*)
