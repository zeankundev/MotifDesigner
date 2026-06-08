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

    echo "--> Fetching linuxdeploy and runtime cache..."
    if [ ! -f "linuxdeploy-x86_64.AppImage" ]; then
        wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
        chmod +x linuxdeploy-x86_64.AppImage
    fi

    # CACHE THE RUNTIME: Download it locally so linuxdeploy doesn't fetch it every time
    if [ ! -f "runtime-x86_64" ]; then
        echo "--> Downloading AppImage runtime cache..."
        wget -q https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64
    fi

    echo "--> Preparing Desktop Entries..."
    mkdir -p "$INSTALL_DIR/usr/share/applications"
    echo -e "[Desktop Entry]\nType=Application\nName=MotifDesigner\nExec=MotifDesigner\nIcon=motifdesigner\nCategories=Utility;" > "$INSTALL_DIR/usr/share/applications/motifdesigner.desktop"

    echo "--> Running linuxdeploy (Offline/Fast mode)..."

    # We export this environment variable so linuxdeploy uses our local downloaded file
    export APPIMAGE_EXTRACTED_RUNTIME=$(pwd)/runtime-x86_64

    OUTPUT=MotifDesigner-x86_64.AppImage ./linuxdeploy-x86_64.AppImage \
        --appdir "$INSTALL_DIR" \
        --executable "$INSTALL_DIR/usr/bin/MotifDesigner" \
        --output appimage \
        --icon-file=appimage/motifdesigner.png \
        --desktop-file="$INSTALL_DIR/usr/share/applications/motifdesigner.desktop"

    echo "--> Success! Created MotifDesigner-x86_64.AppImage"
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
