# UT zipper

Archive reader/writer for Ubuntu Touch. 

Read support includes `zip`, `tar`, `tar.gz`, `tar.bz2`, `tar.xz`, `7z`, `rar`, `ar`, Debian packages (`.deb`) and Click packages (`.click`).
Password-protected ZIP archives can be opened with a passphrase.

Write/export support includes `zip`, `tar`, `tar.gz`, `tar.bz2`, `tar.xz` and `7z`.
ZIP export can optionally be protected with a passphrase.
`rar` is currently read-only.

For `.deb` and `.click`, the browser opens the package payload from `data.tar.*` so installed files can be explored and extracted directly.

[![OpenStore](https://open-store.io/badges/en_US.png)](https://open-store.io/app/utzip.lduboeuf)

# Developper

Relies on https://www.libarchive.org/

## Build

`clickable`


# License

Copyright (C) 2021 Lionel Duboeuf
libarchive: https://www.libarchive.org/

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 3, as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranties of MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
