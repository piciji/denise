
ifeq ($(platform),windows)
    uiflags = $(cppflags) -DGUIKIT_WINAPI
    uilink = -lgdi32 -lcomdlg32 -lole32 -lcomctl32 -lshlwapi -lmsimg32 -lwinmm

else ifeq ($(platform),macosx)
    uiflags = $(objcppflags) -DGUIKIT_COCOA
    uilink = -framework Cocoa -framework CoreVideo

else
    #uiflags = $(cppflags) `pkg-config --cflags gtk+-3.0` -DGUIKIT_GTK -DINSTALL_FOLDER="\"$(prefix)\"" -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE
    uiflags = $(cppflags) $(shell pkg-config --cflags gtk+-3.0) -DGUIKIT_GTK -DINSTALL_FOLDER="\"$(prefix)\""
    uilink = $(shell pkg-config --libs gtk+-3.0) -lfontconfig -lX11 -lXrandr -lrt

endif
