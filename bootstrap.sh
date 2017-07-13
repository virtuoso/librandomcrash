#!/bin/bash

aclocal
autoconf
[[ ! -f ltmain.sh && -f /usr/share/libtool/config/ltmain.sh ]] && ln -s /usr/share/libtool/config/ltmain.sh
automake --add-missing
