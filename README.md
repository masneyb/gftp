The gFTP file transfer client. http://www.gftp.org

# FAQ

## What are the requirements to compile/run gFTP?

  - Glib 2.32+
  - GTK 2.14+ (optional)
  - OpenSSL (optional)
  - libreadline (optional)
  - OpenSSH client (enables SFTP (ssh2) support)


## Supported protocols?

  - FTP (ftp://)
  - FTPS (ftps://) (Explicit TLS - current standard for encrypted FTP)
  - FTPSi (ftpsi://) (Implicit TLS, default port: 990)
  - SSH2 SFTP (ssh2://) - requires OpenSSH client: `ssh`
  - FSP (fsp://) (UDP File Service Protocol)


## How do I install gFTP?

  Git repository:

  - ./autogen.sh
  - ./configure --prefix=/usr
  - make install

  Release tarball:

  - cd gftp-<version>
  - ./configure --prefix=/usr
  - make install


## What systems is gFTP known to run on?

  - Linux distributions

  If gFTP compiles and runs on a platform not listed here, please go to
  https://github.com/masneyb/gftp/ and notify us.


## How do I report bugs in gFTP?

  Go to https://github.com/masneyb/gftp/issues

  When sending in bug reports, PLEASE try to be as descriptive as
  possible: what OS/version you are running, what compiler you are
  compiling with, the output of gftp --info and any other important information.


## How do I force running the text or gtk+ version of gFTP?

  To run the text port, you can type gftp-text or to run the gtk+ port, you can
  run the gftp-gtk. The gftp command is just a shell script that checks if your
  DISPLAY variable is set, and if so it'll run the appropriate version.


## Can gFTP download a bunch of files/directories and then exit when it's completed?

  Yes, the text port of gFTP supports this well. You can type:
  gftp-text -d ftp://ftp.somesite.com/someplace.
  If someplace is a directory, it'll automatically download all of its
  subdirectories as well. If you want to transfer a file through ssh instead of
  ftp, just change the ftp:// to ssh://.


## GTK UI: do I have to enter a port, username and password to connect to a ftp server?

  No you don't. If you leave the port blank, it'll default to the default port
  for the protocol you selected (port 21 for FTP). If you leave the username
  blank, it will default to logging in as anonymous.


## Where does gFTP store its options?

  gFTP follows the XDG Base Directory Specification, it will automatically create a
  ${XDG_CONFIG_HOME}/gftp directory when it is first run. Your
  config file is ${XDG_CONFIG_HOME}/gftp/gftprc, and this is where all of gFTP's settings are
  stored. The config file is well commented, so take a glance at it and see if
  there is anything you want to change. Your bookmarks are stored in the file
  ${XDG_CONFIG_HOME}/gftp/bookmarks.

  ${XDG_CONFIG_HOME} is usually ${HOME}/.config, unless you change it.

  Every time gFTP is run, it will log the contents of the log window to ${XDG_CONFIG_HOME}/gftp/
  gftp.log. The contents of this file will be automatically purged this file when
  gFTP is started up.


## I can't transfer certain file types in binary mode using the FTP protocol.

  Edit your ${XDG_CONFIG_HOME}/gftp/gftprc file and look at the ext= lines towards the bottom of
  the file. These lines control what icon is used for each file type. It also
  controls what mode is used for the file transfer. For example, to transfer all
  HTML files as binary, change the following two lines:

  - ext=.htm:world.png:A:
  - ext=.html:world.png:A:

  to the following:
  - ext=.htm:world.png:B:
  - ext=.html:world.png:B:

## When gFTP tries to get the remote directory listing, I receive the error: Cannot create a data connection: Connection refused

  Go under gFTP->Options->FTP and turn off passive file transfers. Instead of
  sending the PASV command to open up the data connection on the server side, the
  data connection will be opened up on the client side, and the PORT command will
  be sent to the server instead.


## When using the FTPS protocol, gFTP cannot connect if the remote server uses a self signed certificate.

  Go to `gFTP` menu -> `Preferences` -> `SSL Engine` and untick `Verify SSL Peer`

  Or you should add the public key of your self signed CA to your OpenSSL certs
  directory.

