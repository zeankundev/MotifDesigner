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
    # Check if we should use MaXX configuration
    rm -rf "$BUILD_DIR" "$INSTALL_DIR" linuxdeploy-x86_64.AppImage .cache appdir
    if [ "$VARIANT" = "IRIX" ]; then
        echo "Building Variant: IRIX"
        CMAKE_FLAGS="-DUSE_MAXX_DESKTOP=ON"
        APPIMAGE_NAME="MotifDesigner-IRIX-x86_64.AppImage"
        export LD_LIBRARY_PATH="/opt/MaXX/lib64:$LD_LIBRARY_PATH"
    else
        echo "Building Variant: GenericMotif"
        CMAKE_FLAGS="-DUSE_MAXX_DESKTOP=OFF"
        APPIMAGE_NAME="MotifDesigner-GenericMotif-x86_64.AppImage"
    fi

    rm -rf "$BUILD_DIR" "$INSTALL_DIR"

    # Configure and build
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release $CMAKE_FLAGS
    cmake --build "$BUILD_DIR"
    DESTDIR="$INSTALL_DIR" cmake --install "$BUILD_DIR" --prefix "/usr"

    # Fetch tool downloads...
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

    # Asset staging
    mkdir -p "$INSTALL_DIR/usr/share/applications"
    echo -e "[Desktop Entry]\nType=Application\nName=MotifDesigner\nExec=MotifDesigner\nIcon=motifdesigner\nCategories=Utility;" > "$INSTALL_DIR/usr/share/applications/motifdesigner.desktop"
    mkdir -p appimage && touch appimage/motifdesigner.png

    # Step 1: Run linuxdeploy (Using extract-and-run to ensure FUSE bypass)
    ./linuxdeploy-x86_64.AppImage --appimage-extract-and-run \
        --appdir "$INSTALL_DIR" \
        --executable "$INSTALL_DIR/usr/bin/MotifDesigner" \
        --icon-file=appimage/motifdesigner.png \
        --desktop-file="$INSTALL_DIR/usr/share/applications/motifdesigner.desktop"

    # Step 2: Assemble AppImage with appimagetool
    export EXTRA_MKSQUASHFS_ARGS="-progress -processor $(nproc)"
    ./appimagetool-x86_64.AppImage --appimage-extract-and-run \
        --runtime-file "$(pwd)/runtime-x86_64" \
        "$INSTALL_DIR" \
        "$APPIMAGE_NAME"

    echo "--> Success! Created $APPIMAGE_NAME"
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
