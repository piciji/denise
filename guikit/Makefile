
# compile api.cpp and add uiflags to generic compiler flags
# add uilink to application linker flags
# windows NOTE: don't forget to compile a rc file which points to a manifest file to apply visual styles

ifeq ($(platform),windows)
    uiflags = $(cppflags) -DGUIKIT_WINAPI
    uilink = -lkernel32 -luser32 -lgdi32 -lole32 -lcomctl32 -lcomdlg32 -luxtheme -lshlwapi -lmsimg32

else ifeq ($(platform),macosx)
    uiflags = $(objcppflags) -DGUIKIT_COCOA
    uilink = -framework Cocoa

else
    #uiflags = $(cppflags) `pkg-config --cflags gtk+-3.0` -DGUIKIT_GTK -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE
    uiflags = $(cppflags) `pkg-config --cflags gtk+-3.0` -DGUIKIT_GTK
    uilink = `pkg-config --libs gtk+-3.0` -lfontconfig

endif
