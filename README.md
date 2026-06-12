# MotifDesigner

An X11 Motif visual IDE for your retro desktop. Inspired by Visual Basic. UI builder, code editor, all right in the app. Works best if you have Motif and you're on MaXX. **This is still under heavy WIP, so expect missing buttons (intentionally commented out until ready for production) in order to comply with shipping standards!**

## Building
To build, you would require:
- `cmake`
- `g++`
- Devel files of:
    - `libX11`
    - `libXt`
    - `libXmu`
    - `libXm`
    - `libXpm`

`shortcuts.sh` provide a wide range of options to build and run.
- `build`: Builds the project
- `clean`: Cleans the build directory
- `run`: Runs the project
- `quick`: Builds and runs the project
- `appimage`: Builds an AppImage of the project

Before launching `shortcuts.sh`, you can prepend the `VARIANT` variable. That being either `GENERIC` or `IRIX` (e.g. `VARIANT=GENERIC ./shortcuts.sh quick`). 

`GENERIC` will take your system's local Motif libraries and compile with that (typically under `/usr/lib` and `/usr/lib/include`, though this may vary).

`IRIX` will take all libraries that are in `/opt/MaXX/include` and `/opt/MaXX/lib64`. **IMPORTANT: Do not run the IRIX version if you are not running MaXX or does not have the original theme files. Technically, it will still work, however, expect the rest to black out.**

### Examples
To build and run for an MaXX desktop:
```
VARIANT=IRIX ./shortcuts.sh quick
```

To build generically without environment variables (okay!):
```
./shortcuts.sh build
```