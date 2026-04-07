#!/bin/bash

prefix=usr/local

rm -rf denisepackage
mkdir denisepackage
mkdir denisepackage/DEBIAN
mkdir -p denisepackage/$prefix/
mkdir denisepackage/$prefix/bin
mkdir denisepackage/$prefix/share
mkdir denisepackage/$prefix/share/denise
mkdir denisepackage/$prefix/share/denise/translation
mkdir denisepackage/$prefix/share/denise/images
mkdir denisepackage/$prefix/share/denise/fonts
mkdir denisepackage/$prefix/share/denise/data
mkdir denisepackage/$prefix/share/denise/sounds
mkdir denisepackage/$prefix/share/denise/shader
mkdir denisepackage/$prefix/share/applications
mkdir denisepackage/$prefix/share/icons
mkdir denisepackage/$prefix/share/mime
mkdir denisepackage/$prefix/share/mime/packages

install -D -m 644 control denisepackage/DEBIAN
install -D -m 644 translation/* denisepackage/$prefix/share/denise/translation
install -D -m 644 fonts/* denisepackage/$prefix/share/denise/fonts
cp -r sounds/* denisepackage/$prefix/share/denise/sounds/
cp -r shader/* denisepackage/$prefix/share/denise/shader/
install -D -m 644 data/* denisepackage/$prefix/share/denise/data
install -D -m 644 img/denise.png denisepackage/$prefix/share/icons
install -D -m 644 images/* denisepackage/$prefix/share/denise/images
install -D -m 644 denise.desktop denisepackage/$prefix/share/applications
install -D -m 644 application-x-denise.xml denisepackage/$prefix/share/mime/packages
install -D -m 755 ../builds/release/program/denise denisepackage/$prefix/bin

INSTALLED_SIZE=$(du -ks denisepackage | cut -f 1)
VERSION=$(cat ../program/program.h | grep '^#define VERSION' ../program/program.h | sed 's/.*"\(.*\)".*/\1/')

sed -i 's/_INSTALLED_SIZE/'$INSTALLED_SIZE'/;s/_VERSION/'$VERSION'/' denisepackage/DEBIAN/control
