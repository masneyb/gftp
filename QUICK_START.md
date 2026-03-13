# gFTP Quick Start Guide - macOS

This guide provides a quick start for building and running gFTP on macOS using Homebrew.

## 🚀 Quick Start (TL;DR)

```bash
# 1. Install Dependencies
brew install gtk+3 openssl meson ninja pkg-config gettext

# 2. Build and Install gFTP
./build_gftp_homebrew.sh

# 3. Run the App
open gFTP.app
```

---

## 📦 Building and Running

### 1. Install Dependencies

Install all required build tools and libraries using Homebrew:
```bash
brew install gtk+3 openssl meson ninja pkg-config gettext
```

### 2. Build and Create the App Bundle

The `build_gftp_homebrew.sh` script automates the entire process of building gFTP, creating the `gFTP.app` bundle, and packaging all necessary dependencies.

```bash
./build_gftp_homebrew.sh
```

This will create the `gFTP.app` bundle in the current directory.

### 3. Run gFTP

You can now launch the application directly:

```bash
open gFTP.app
```

---

## 📝 Notes

- The `build_gftp_homebrew.sh` script handles everything from building to bundling.
- For more detailed instructions on building and packaging, please see the `MACOS.md` file.
- The generated `gFTP.app` is self-contained and can be moved to your `/Applications` folder.
