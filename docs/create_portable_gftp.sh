#!/bin/sh
# run `git clean -dfx` before running this

if [ ! -d src/gtk ] ; then
    echo "src/gtk was not found"
    exit 1
fi

gitrev=$(git rev-parse --short HEAD)
date=$(date "+%Y.%m.%d")
arch=$(uname -m)

outdir=gftp-${date}-${gitrev}__${arch}

mkdir -p ${outdir}

if [ ! -f src/gtk/gftp-gtk ] ; then
    if [ ! -f ./configure ] ; then
        ./autogen.sh
    fi
    ./configure --prefix=/usr --disable-ssl --disable-nls
    make
    sync
fi

mkdir -p ${outdir}/gtk
cp -a src/gtk/gftp-gtk ${outdir}/gtk/gftp-${arch}
cp -a docs/sample.gftp ${outdir}
mkdir -p ${outdir}/doc
cp -a README.md LICENSE AUTHORS docs/USERS-GUIDE ChangeLog ${outdir}/doc

echo '#!/bin/sh

appdir=$(dirname "$0")
arch=$(uname -m)

export XDG_CONFIG_HOME=${appdir}
export GFTP_SHARE_DIR=${appdir}/sample.gftp

if [ -x ${appdir}/gtk/gftp-${arch} ] ; then
    gftp=${appdir}/gtk/gftp-${arch}
else
    gftp=${appdir}/gtk/gftp
fi

if ! [ -x ${gftp} ] ; then
    echo "cannot find gftp in $appdir"
    exit 1
fi

echo "exec ${gftp} $@"
exec ${gftp} "$@"
' > ${outdir}/gftp-portable
chmod +x ${outdir}/gftp-portable


echo 'This is gftp-portable for testing purposes

Date: '${date}'
Git revision: '${gitrev}'

Does not include SSL support
' > ${outdir}/readme.txt


chown -R nobody:nobody ${outdir}

tar -Jcf ${outdir}.tar.xz ${outdir}
