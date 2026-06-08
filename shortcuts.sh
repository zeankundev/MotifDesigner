#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Configuration
BUILD_DIR="build"
INSTALL_DIR="appdir"

build_project() {
    echo "--> Making sure cmake exists..."
    if ! command -v cmake &> /dev/null; then
        echo "Error: cmake is not installed."
        exit 1
    fi
    echo "--> Building project..."
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR" --target all
}

build_appimage() {
    build_project

    rm -rf "$INSTALL_DIR"
    echo "--> Installing to temporary AppDir..."
    DESTDIR="$INSTALL_DIR" cmake --install "$BUILD_DIR" --prefix "/usr"

    echo "--> Fetching deployment tools..."
    if [ ! -f "linuxdeploy-x86_64.AppImage" ]; then
        wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
        chmod +x linuxdeploy-x86_64.AppImage
    fi

    if [ ! -f "appimagetool-x86_64.AppImage" ]; then
        wget -q https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
        chmod +x appimagetool-x86_64.AppImage
    fi

    if [ ! -f "runtime-x86_64" ]; then
        wget -q https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64
    fi

    # === NEW: BUNDLE THE MOTIF/MaXX ASSETS ===
    echo "--> Bundling MaXX Theme & Scheme assets..."
    # We copy them into appdir/usr/share/X11/schemes inside your bundle
    mkdir -p "$INSTALL_DIR/usr/share/X11"
    if [ -d "/opt/MaXX/share/X11/schemes" ]; then
        cp -r /opt/MaXX/share/X11/schemes "$INSTALL_DIR/usr/share/X11/"
    else
        echo "Warning: /opt/MaXX/share/X11/schemes not found. Themes won't bundle!"
    fi

    echo "--> Preparing AppDir assets..."
    mkdir -p appimage
    touch appimage/motifdesigner.png

    echo "--> Step 1: Running linuxdeploy..."
    LD_LIBRARY_PATH="/opt/MaXX/lib64:$LD_LIBRARY_PATH" ./linuxdeploy-x86_64.AppImage \
        --appdir "$INSTALL_DIR" \
        --executable "$INSTALL_DIR/usr/bin/MotifDesigner" \
        --icon-file=appimage/motifdesigner.png \
        --desktop-file="$INSTALL_DIR/usr/share/applications/motifdesigner.desktop"

    echo "--> Step 2: Running appimagetool..."
    export EXTRA_MKSQUASHFS_ARGS="-progress -processor $(nproc)"

    ./appimagetool-x86_64.AppImage \
        --runtime-file "$(pwd)/runtime-x86_64" \
        "$INSTALL_DIR" \
        MotifDesigner-x86_64.AppImage

    echo "--> Success!"
}

# Command routing
case "$1" in
    "build")
        build_project
        ;;
    "clean")
        echo "Cleaning up..."
        rm -rf "$BUILD_DIR" "$INSTALL_DIR" linuxdeploy-x86_64.AppImage *.AppImage .cache appdir        ;;
    "run")
        if [ -f "$BUILD_DIR/MotifDesigner" ]; then
            ./"$BUILD_DIR/MotifDesigner"
        else
            echo "Binary not found. Run './script.sh build' first."
        fi
        ;;
    "quick")
        rm -rf "$BUILD_DIR"
        build_project
        ./"$BUILD_DIR/MotifDesigner"
        ;;
    "appimage")
        build_appimage
        ;;
    *)
        echo "Usage: $0 {build|clean|run|quick|appimage}"
        exit 1
        ;;
esac
