# Simplified Build Guide for gFTP on macOS

This guide provides the simplest path to building a distributable `gFTP.app` on macOS using Homebrew for dependencies.

It relies on the `build_gftp_homebrew.sh` script, which automates the entire process.

## 1. Prerequisites

Before you begin, you need to install a few tools using [Homebrew](https://brew.sh/).

1.  **Install Homebrew**:
    If you don't have it, open Terminal and run:
    ```bash
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    ```

2.  **Install Build Dependencies**:
    Now, use Homebrew to install the necessary tools and libraries:
    ```bash
    brew install gtk+3 meson ninja imagemagick pkg-config
    ```

3.  **Clone the `AppBundleGenerator` Tool**:
    The build script uses a helper tool to create the `.app` bundle. Clone it into the same directory that contains the `gftp` source folder (e.g., `~/source/`).
    ```bash
    # Assuming you are in a directory like ~/source
    git clone https://github.com/sedwards-ibowl/AppBundleGenerator.git
    ```
    The script will automatically find it if it's located at `../AppBundleGenerator`.

## 2. The Build Process

The `build_gftp_homebrew.sh` script is the recommended way to build the application. It will:

1.  Check for all the prerequisites.
2.  Configure the build using `meson`.
3.  Compile the `gftp` source code using `ninja`.
4.  Convert icons to the required macOS format.
5.  Create a self-contained `gFTP.app` bundle in your current directory.
6.  Copy `gftp`, its libraries (from Homebrew), and all necessary resources (icons, translations, etc.) into the app bundle.
7.  Clean up any unwanted dependencies that might have been pulled in.

## 3. Step-by-Step Instructions

1.  **Open your Terminal** and navigate to the `gftp` source code directory.

2.  **Run the build script**:
    ```bash
    ./build_gftp_homebrew.sh
    ```

3.  **Wait for the process to complete.** It will print its progress, including checking for tools, building `gftp`, and creating the app bundle.

## 4. Results

Once the script finishes, you will find a complete `gFTP.app` in the project's root directory.

You can run it like any other macOS application:

```bash
open ./gFTP.app
```

## Other Available Scripts

The project contains several build scripts. Here is a summary of what they do:

-   **`build_gftp_simple.sh`**: Builds `gftp` and installs it to a local directory (`~/gftp-install`). It does **not** create a self-contained `.app` bundle, but is useful for local development and testing.
-   **`build_gftp_app.sh`**: A script similar to the recommended `build_gftp_homebrew.sh` that also produces a full `.app` bundle.
-   **`build_and_bundle_gftp.sh`**: An incomplete script. It builds the project but fails to bundle the required libraries, resulting in an app that won't launch.
-   **`docs/BUILDING-MACOS.md`**: Contains instructions for a much more complex build process that builds all dependencies from source using `jhbuild`. This is not recommended unless you specifically need to avoid Homebrew.

For simplicity and reliability, stick with `build_gftp_homebrew.sh`.
