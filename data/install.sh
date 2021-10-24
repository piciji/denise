#!/bin/bash

echo "Installing dependencies."
if [ -x "$(command -v apk)" ];       then sudo apk add --no-cache sdl2 gtk+3.0
elif [ -x "$(command -v apt-get)" ]; then sudo apt-get install libsdl2-2.0-0 libgtk-3-0
elif [ -x "$(command -v pacman)" ];  then sudo pacman -Sy gtk3 || sudo pacman -Sy pkgconf
elif [ -x "$(command -v dnf)" ];     then sudo dnf install SDL2 gtk3
elif [ -x "$(command -v zypper)" ];  then sudo zypper install libSDL2 gtk3-devel
elif [ -x "$(command -v yum)" ];     then sudo yum install SDL2 gtk3
elif [ -x "$(command -v emerge)" ];  then sudo emerge libsdl2 gtk3
else
  echo "Your package manager is not supported. Please manually install the sdl2 and gtk3 packages for your system, if not yet."
fi
echo "Finished installing dependencies. Copying application files."

prefix=/usr

if [ -d $prefix/local ]; then mkdir -p $prefix/local/bin/; else mkdir -p $prefix/bin/; fi
mkdir -p $prefix/share/icons/
mkdir -p $prefix/share/applications/
mkdir -p $prefix/share/mime/packages/
mkdir -p $prefix/share/denise/translation/
mkdir -p $prefix/share/denise/data/
mkdir -p $prefix/share/denise/fonts/
mkdir -p $prefix/share/denise/img/
mkdir -p $prefix/share/denise/sounds/
mkdir -p $prefix/share/denise/shader/

if [ -d $prefix/local ]; then
	install -D -m 755 Denise $prefix/local/bin/Denise;
else
	install -D -m 755 Denise $prefix/bin/Denise;
fi

install -D -m 644 denise.png $prefix/share/icons/denise.png
install -D -m 644 denise.desktop $prefix/share/applications/denise.desktop
install -D -m 644 application-x-denise.xml $prefix/share/mime/packages/application-x-denise.xml
install -D -m 644 translation/* $prefix/share/denise/translation
install -D -m 644 data/* $prefix/share/denise/data
install -D -m 644 fonts/*.ttf $prefix/share/denise/fonts
install -D -m 644 img/* $prefix/share/denise/img
cp -r sounds/* $prefix/share/denise/sounds/
cp -r shader/* $prefix/share/denise/shader/
echo "Installation complete"
