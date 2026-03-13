install_data(
    '48x48' / 'gftp.png',
    rename: 'gftp.png',
    install_dir: install_dir_base / get_option('datadir') / 'gftp'
)

foreach dir : ['16x16','22x22','24x24','32x32','48x48']
    install_data(
        dir / 'gftp.png',
        install_dir: install_dir_base / get_option('datadir') / 'icons' / 'hicolor' / dir / 'apps'
    )
endforeach
install_data(
    'scalable' / 'gftp.svg',
    install_dir: install_dir_base / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps'
)