sources = [
    'cache.c',
    'charset-conv.c',
    'config_file.c',
    'ftp-dir-listing.c',
    'misc.c',
    'protocols.c',
    'protocol_bookmark.c',
    'protocol_ftp.c',
    'protocol_ftps.c',
    'protocol_localfs.c',
    'protocol_http.c',
    'protocol_https.c',
    'pty.c',
    'sshv2.c',
    'sslcommon.c',
    'socket-connect.c',
    'sockutils.c',
]

datadir = get_option('prefix') / get_option('datadir')
docdir = datadir / 'doc'

libgftp_sources = sources
libgftp_c_args = [
    f'-DSHARE_DIR="@datadir@/gftp"',
    f'-DLOCALE_DIR="@datadir@/locale"',
    f'-DDOC_DIR="@docdir@"',
    '-DHAVE_GRANTPT'  # Enable Unix98 PTY support for macOS
]
libgftp_deps = [glib, pthread, ssl]