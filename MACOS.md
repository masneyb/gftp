# Building and Packaging gFTP for macOS

This guide provides complete instructions for building gFTP natively on macOS and creating a distributable application bundle.

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Building gFTP with Meson](#building-gftp-with-meson)
4. [Creating a macOS App Bundle](#creating-a-macos-app-bundle)
5. [Creating a DMG Installer](#creating-a-dmg-installer)

## Overview

This guide covers the modern, simplified process of building gFTP on macOS from source using Meson, then packaging it as a native macOS application bundle (`.app`).

### Build Strategy

- **Meson Build:** Uses the Meson build system directly for a fast and straightforward build process.
- **Homebrew for Dependencies:** Relies on Homebrew to provide necessary libraries like GTK, OpenSSL, and build tools.
- **Native macOS:** Uses the system's `clang` compiler and the macOS SDK.
- **Self-Contained App Bundle:** The final `.app` bundle includes all necessary resources and is fully relocatable.

## Prerequisites

### Required Software

1. **Xcode Command Line Tools**
   ```bash
   xcode-select --install
   ```

2. **Homebrew**
   If you don't have Homebrew installed, you can install it with:
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

3. **Homebrew Dependencies**
   Install all required build tools and libraries with a single command:
   ```bash
   brew install gtk+3 openssl meson ninja pkg-config gettext
   ```

### Verify Installation

Ensure all tools are correctly installed and available in your `PATH`.
```bash
which meson ninja pkg-config
# Should output paths for each command.
```

## Building gFTP with Meson

With the prerequisites installed, building gFTP is a simple, three-step process.

### Step 1: Configure the Build

Run Meson to configure the build. This only needs to be done once.

```bash
meson setup build
```
This will create a `build` directory and prepare it for compilation. Meson will automatically detect the dependencies you installed with Homebrew.

### Step 2: Compile the Application

Compile the source code using Ninja.

```bash
ninja -C build
```
This will build the `gftp-gtk` and `gftp-text` executables and place them in the `build/src/` directory.

### Step 3: Install the Artifacts

Install the compiled application and all its resources (icons, documentation, etc.) into a local `install` directory. This is a staging area for creating the app bundle.

```bash
ninja -C build install
```
After this step, the `install` directory will contain a complete, runnable version of the application.

## Creating a macOS App Bundle

Once gFTP is built and installed, you can create a proper macOS application bundle.

### Use the `create_app_bundle.sh` Script

The repository includes a script that automates the creation of the `gFTP.app` bundle.

```bash
./create_app_bundle.sh
```

This script will:
1. Create a `gFTP.app` directory with the standard macOS bundle structure.
2. Copy the `gftp-gtk` executable into `gFTP.app/Contents/MacOS/`.
3. Copy all necessary resources (icons, translations, documentation) from the `install` directory into `gFTP.app/Contents/Resources/`.
4. Generate a standard `Info.plist` file for the application.
5. On the first run, the app will automatically copy its resources to `$HOME/Library/gFTP` for user-specific configuration.

### Launch gFTP.app

You can now launch the application directly:

```bash
open gFTP.app
```

## Creating a DMG Installer

To create a distributable DMG file containing `gFTP.app`, use the included script.

### Use the `create_dmg_for_app.sh` Script

```bash
./create_dmg_for_app.sh
```

This will create a `gFTP.dmg` file in the project root, ready for distribution. The DMG will contain the `gFTP.app` bundle and a link to the Applications folder.

### Distribution Notes

For public distribution, you should code sign and notarize the application to ensure it runs on other users' machines without security warnings.

1. **Code Sign with Developer ID:**
   ```bash
   codesign -s "Developer ID Application: Your Name" --deep --force --options runtime gFTP.app
   ```

2. **Notarize the App:**
   ```bash
   xcrun notarytool submit gFTP.dmg --keychain-profile "AC_PASSWORD" --wait
   ```

3. **Staple the Notarization Ticket:**
   ```bash
   xcrun stapler staple gFTP.app
   ```
