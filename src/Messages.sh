#! /bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.ui` `find ../plugins -name \*.rc -o -name \*.ui` >> rc.cpp
$XGETTEXT `find . -name \*.cpp -o -name \*.h` `find ../plugins -name \*.cpp -o -name \*.h` -o $podir/k3b.pot
rm -f rc.cpp

