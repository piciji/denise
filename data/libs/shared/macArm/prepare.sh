#!/bin/sh

install_name_tool -change `otool -D libbrotlicommon.1.dylib | cut -d':' -f2` @rpath/libbrotlicommon.1.dylib libbrotlidec.1.dylib
install_name_tool -change `otool -D libz.1.dylib | cut -d':' -f2` @rpath/libz.1.dylib libpng16.16.dylib
install_name_tool -change /usr/lib/libz.1.dylib @rpath/libz.1.dylib libpng16.16.dylib
install_name_tool -change `otool -D libbz2.1.0.dylib | cut -d':' -f2` @rpath/libbz2.1.0.dylib libfreetype.6.dylib
install_name_tool -change `otool -D libpng16.16.dylib | cut -d':' -f2` @rpath/libpng16.16.dylib libfreetype.6.dylib
install_name_tool -change `otool -D libz.1.dylib | cut -d':' -f2` @rpath/libz.1.dylib libfreetype.6.dylib
install_name_tool -change `otool -D libbrotlidec.1.dylib | cut -d':' -f2` @rpath/libbrotlidec.1.dylib libfreetype.6.dylib

install_name_tool -id @rpath/libbrotlicommon.1.dylib libbrotlicommon.1.dylib
install_name_tool -id @rpath/libbrotlidec.1.dylib libbrotlidec.1.dylib
install_name_tool -id @rpath/libbz2.1.0.dylib libbz2.1.0.dylib
install_name_tool -id @rpath/libz.1.dylib libz.1.dylib
install_name_tool -id @rpath/libusb-1.0.0.dylib libusb-1.0.0.dylib
install_name_tool -id @rpath/libpng16.16.dylib libpng16.16.dylib
install_name_tool -id @rpath/libfreetype.6.dylib libfreetype.6.dylib
