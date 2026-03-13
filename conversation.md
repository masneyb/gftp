Based on the moduleset files in your `jhbuild_modulesets` directory, here are the available meta-packages that you can build with `jhbuild`:

From `gtk-osx-bootstrap.modules`:
*   **`meta-gtk-osx-bootstrap`**: This builds the core set of libraries needed to bootstrap the GTK environment.

From `gtk-osx-network.modules`:
*   **`meta-gtk-osx-webkit-gtk3`**: This builds WebKitGTK and its dependencies for GTK3.

From `gtk-osx.modules`:
*   **`meta-gtk-osx-gtk3`**: This builds the main GTK3 stack.
*   **`meta-gtk-osx-freetype`**: This appears to be a null target, likely for compatibility.

To try and fix your environment, you could attempt to rebuild the core GTK stack. The most relevant targets would be:

1.  `jhbuild build meta-gtk-osx-bootstrap`
2.  `jhbuild build meta-gtk-osx-gtk3`

Running these commands may resolve the inconsistencies in your `jhbuild` environment. Please be aware that this will recompile a large number of packages and may take a significant amount of time.
