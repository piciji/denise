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

prefix=~/.local
mkdir -p $prefix/bin/
mkdir -p $prefix/share/icons/
mkdir -p $prefix/share/applications/
mkdir -p $prefix/denise/translation/
mkdir -p $prefix/denise/data/
mkdir -p $prefix/denise/fonts/
mkdir -p $prefix/denise/img/
mkdir -p $prefix/denise/shader/

install -D -m 755 Denise $prefix/bin/Denise
install -D -m 644 denise.png $prefix/share/icons/denise.png
install -D -m 644 denise.desktop $prefix/share/applications/denise.desktop
install -D -m 644 translation/* $prefix/denise/translation
install -D -m 644 data/* $prefix/denise/data
install -D -m 644 fonts/*.ttf $prefix/denise/fonts
install -D -m 644 img/* $prefix/denise/img
cp -r shader/* $prefix/denise/shader/
echo "Installation complete"
