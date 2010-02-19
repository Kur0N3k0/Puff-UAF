#!/usr/bin/env bash
#installs to ~/Puff-UAF, I think?
./configure --prefix "/home/josh/puff" --enable-warnings --with-freetype NETCDF_INC='/usr/include/netcdf/'
make
make install
