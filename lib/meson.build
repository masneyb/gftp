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

libgftp = static_library(
    'libgftp',
    sources,
    c_args: [
        f'-DSHARE_DIR="@datadir@/gftp"',
	    f'-DLOCALE_DIR="@datadir@/locale"',
        f'-DDOC_DIR="@docdir@"'
    ],
    dependencies: [glib, pthread, ssl],
    install: false
)