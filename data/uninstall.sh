#!/bin/bash

prefix=/usr

if [ -f $prefix/local/bin/Denise ];	then rm $prefix/local/bin/Denise;
elif [ -f $prefix/bin/Denise ];	then rm $prefix/bin/Denise; fi

if [ -f $prefix/share/icons/denise.png ]; then rm $prefix/share/icons/denise.png; fi
if [ -f $prefix/share/applications/denise.desktop ]; then rm $prefix/share/applications/denise.desktop; fi
if [ -f $prefix/share/mime/packages/application-x-denise.xml ]; then
  rm $prefix/share/mime/packages/application-x-denise.xml;
  if [ -x "$(command -v update-mime-database)" ]; then update-mime-database $prefix/share/mime; fi
  if [ -x "$(command -v update-desktop-database)" ]; then update-desktop-database $prefix/share/mime; fi
fi
if [ -d $prefix/share/denise ]; then rm -rf $prefix/share/denise; fi

echo "Uninstallation complete"
