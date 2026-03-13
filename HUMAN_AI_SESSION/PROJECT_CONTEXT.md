# Project: gFTP

**Description:** gFTP is a free/open source multithreaded FTP client for *NIX based machines. It has a text interface and a GTK+ interface.

**Repository:** https://github.com/masneyb/gftp

## Build and Dependencies

gFTP uses the Meson build system. The main dependencies are:
- GLib
- GTK+ (optional, for the graphical interface)
- OpenSSL (optional, for encrypted protocols)

## Supported Protocols

- FTP
- FTPS (Explicit TLS)
- FTPSi (Implicit TLS)
- SSH2 SFTP
- FSP (File Service Protocol)

## User Interfaces

gFTP provides two user interfaces:
- A GTK-based graphical interface (`gftp-gtk`)
- A text-based interface (`gftp-text`)

## Source Code Structure

The source code is organized as follows:
- `src/`: Main source code
- `src/gtk/`: GTK+ user interface
- `src/text/`: Text-based user interface
- `lib/`: Core library with protocol implementations
- `po/`: Translation files
- 'icoms/' : Images used for files, folders, binaries
