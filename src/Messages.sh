#! /bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.ui` `find ../plugins -name \*.rc -o -name \*.ui` >> rc.cpp
$PREPARETIPS > tips.cpp
$XGETTEXT `find . -name \*.cpp -o -name \*.h` `find ../plugins -name \*.cpp -o -name \*.h` -o $podir/k3b.pot
rm -f tips.cpp
rm -f rc.cpp

