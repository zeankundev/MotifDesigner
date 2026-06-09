#ifndef PIXMAPMANAGER_H
#define PIXMAPMANAGER_H

#include <X11/Intrinsic.h>
#include <X11/X.h>
#include <X11/Xlib.h>
class PixmapManager {
    public:
        typedef struct {
            Pixmap pixmap;
            Pixmap mask;
            unsigned int width;
            unsigned int height;
            Display *display;
        } PixmapWithMask;
        static void ExposeCallback(Widget w, XtPointer clientData, XtPointer callData);
        static Pixmap XpmToPixmap(Widget reference, char **xpmData);
        static PixmapWithMask ScalePixmapWithMask(Display *display, Pixmap source,
                                                  Pixmap sourceMask,
                                                  unsigned int srcWidth, unsigned int srcHeight,
                                                  unsigned int targetWidth, unsigned int targetHeight);
};

#endif
