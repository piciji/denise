#!/bin/bash

sudo apt-get install libgtk2.0-dev
sudo apt-get install mesa-common-dev
sudo apt-get install libopenal-dev
sudo apt-get install libpulse-dev
sudo apt-get install libsdl2-dev
sudo apt-get install libudev-dev

prefix=~/.local
mkdir -p $prefix/bin/
mkdir -p $prefix/share/icons/
mkdir -p $prefix/share/applications/
mkdir -p $prefix/denise/translation/
mkdir -p $prefix/denise/data/
mkdir -p $prefix/denise/fonts/
mkdir -p $prefix/denise/shader/

install -D -m 755 Denise $prefix/bin/Denise
install -D -m 644 denise.png $prefix/share/icons/denise.png
install -D -m 644 denise.desktop $prefix/share/applications/denise.desktop
install -D -m 644 translation/* $prefix/denise/translation
install -D -m 644 data/* $prefix/denise/data
install -D -m 644 fonts/*.ttf $prefix/denise/fonts
install -D -m 644 shader/* $prefix/denise/shader
