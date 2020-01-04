
#platform detection
ifeq ($(platform),)
    ifneq (,$(filter Windows%,$(OS)))
	platform := windows
    else
	uname := $(shell uname -s)

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

#compiler assignment
ifeq ($(compiler),)
    compiler := g++
    ifeq ($(platform),macosx)
	compiler := clang++
    endif
endif

cppflags := -x c++ -std=c++14
objcppflags := -x objective-c++ -std=c++14
	
compile = \
    $(strip \
	$(compiler) $(cppflags) $(flags) $1 -c $< -o $@ \
    )


#file handling
ifeq ($(platform),windows)
    SHELL := cmd
    delete = del $(subst /,\,$1)
    copy = xcopy /I /Y $3 $(subst /,\,$1) $(subst /,\,$2)
else
    delete = rm -rf $1
    copy = rm -rf $2 && cp -r $1 $2
endif


# string functions
[A-Z] = A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
[a-z] = a b c d e f g h i j k l m n o p q r s t u v w x y z

# function strtr(source, from, to)
strtr = \
  $(eval __temp := $1) \
  $(strip \
    $(foreach c, \
      $(join $(addsuffix :,$2),$3), \
      $(eval __temp := \
        $(subst $(word 1,$(subst :, ,$c)),$(word 2,$(subst :, ,$c)),$(__temp)) \
      ) \
    ) \
    $(__temp) \
  )

# function strupper(source)
strupper = $(strip $(call strtr,$1,$([a-z]),$([A-Z])))

# function strlower(source)
strlower = $(strip $(call strtr,$1,$([A-Z]),$([a-z])))
