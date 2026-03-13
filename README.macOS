# gFTP macOS App Bundle

This document describes the implementation of the `gFTP.app` bundle for macOS, including its structure and resource management.

## Overview

gFTP is packaged as a standard macOS application bundle (`.app`). This allows it to be a self-contained, relocatable application that integrates well with the macOS environment.

## Implementation Details

### Resource Management

- **App Bundle Resources:** All necessary resources for the application, such as icons, translations, documentation, and sample files, are included within the `gFTP.app` bundle in the `Contents/Resources` directory.
- **First-Run Initialization:** When the application is launched for the first time, it copies its default resources from the app bundle to the user's local application support directory at `$HOME/Library/gFTP`. This includes the default `gftprc` configuration file and sample bookmarks. This ensures that each user has their own set of configuration files to modify.
- **Runtime Detection:** The application uses CoreFoundation APIs to determine if it's running from within an app bundle and to locate its `Resources` directory.

### App Bundle Structure

The `gFTP.app` bundle has the following structure:

```
gFTP.app/
├── Contents/
│   ├── Info.plist
│   ├── MacOS/
│   │   └── gftp-gtk                 (executable)
│   └── Resources/
│       ├── gftp.icns                (application icon)
│       ├── locale/                  (translation files)
│       │   ├── es/LC_MESSAGES/gftp.mo
│       │   └── ...
│       ├── gftp/                    (sample gftprc and icons)
│       │   ├── gftprc
│       │   └── *.xpm
│       └── doc/                     (documentation)
│           ├── USERS-GUIDE
│           └── ...
```

## Building and Packaging

### Step 1: Build gFTP with Meson

First, build the application using Meson and Ninja. This will compile the code and prepare the files for bundling.

```bash
# Configure the build
meson setup build

# Compile the application
ninja -C build

# Install the artifacts to a local directory
ninja -C build install
```

### Step 2: Create the App Bundle

The repository includes a script to automate the creation of the app bundle.

```bash
./create_app_bundle.sh
```

This script performs the following actions:
- Creates the `gFTP.app` directory structure.
- Copies the `gftp-gtk` executable from the `install` directory.
- Copies all resources (icons, translations, documentation, etc.) from the `install` directory into `gFTP.app/Contents/Resources/`.
- Generates an `Info.plist` file.

## Testing

### Launching the App

You can launch the application by double-clicking `gFTP.app` in the Finder or by using the command line:

```bash
open gFTP.app
```

### Verifying Resource Installation

On the first launch, you can verify that the resources have been copied to your local application support directory:

```bash
ls -l ~/Library/gFTP/
```
This directory should contain a `gftprc` file and other resources.

## Benefits

1.  **Self-Contained:** The app bundle contains everything the application needs to run.
2.  **Relocatable:** The `gFTP.app` bundle can be moved to any location on the filesystem.
3.  **Clean User Environment:** User-specific configuration is stored cleanly in the standard `~/Library/gFTP` location, separate from the application itself.
4.  **Standard macOS Experience:** The application behaves like a typical macOS app, making it easy for users to install and manage.
