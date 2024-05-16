#!/bin/sh

# 1 : lib path
# 2 : lib name
# 3 : app path
# 4 : app name

mkdir $3/Contents/Frameworks
cp -L $1/$2 $3/Contents/Frameworks/

# reference dylib to itself
install_name_tool -id @executable_path/../Frameworks/$2 $3/Contents/Frameworks/$2

# reference binary to his new own dylib
install_name_tool -change `otool -D $1/$2 | cut -d':' -f2` @executable_path/../Frameworks/$2 $3/Contents/MacOS/$4
