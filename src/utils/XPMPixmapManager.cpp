#include "PixmapManager.h"
#include <Xm/XmStrDefs.h>
#include <cstdlib>

void PixmapManager::ExposeCallback(Widget w, XtPointer clientData, XtPointer callData) {
    PixmapManager::PixmapWithMask *pixmapData = (PixmapManager::PixmapWithMask *)clientData;
    if (pixmapData == NULL || pixmapData->pixmap == None) return;

    Display *display = pixmapData->display;
    Window window = XtWindow(w);
    if (window == None) return;

    // Get widget dimensions to center the pixmap
    Dimension widgetWidth, widgetHeight;
    XtVaGetValues(w, XmNwidth, &widgetWidth, XmNheight, &widgetHeight, NULL);

    // Calculate center position
    int x = (widgetWidth - pixmapData->width) / 2;
    int y = (widgetHeight - pixmapData->height) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    // Create GC with clip mask for transparency
    GC gc = XCreateGC(display, window, 0, NULL);
    if (gc == NULL) return;

    if (pixmapData->mask != None) {
        // Set clip mask to make transparent areas not draw
        XSetClipMask(display, gc, pixmapData->mask);
        XSetClipOrigin(display, gc, x, y);
    }

    // Draw the pixmap with mask applied
    XCopyArea(display, pixmapData->pixmap, window, gc, 0, 0,
              pixmapData->width, pixmapData->height, x, y);

    XFreeGC(display, gc);
}

PixmapManager::PixmapWithMask PixmapManager::ScalePixmapWithMask(Display *display, Pixmap source,
                                          Pixmap sourceMask,
                                          unsigned int srcWidth, unsigned int srcHeight,
                                          unsigned int targetWidth, unsigned int targetHeight) {
    PixmapManager::PixmapWithMask result = {None, None, 0, 0, display};
    int screen = DefaultScreen(display);
    int depth = DefaultDepth(display, screen);
    Window window = RootWindow(display, screen);

    // Calculate scale factor maintaining aspect ratio
    double scaleX = (double)targetWidth / srcWidth;
    double scaleY = (double)targetHeight / srcHeight;
    double scale = (scaleX < scaleY) ? scaleX : scaleY;

    unsigned int newWidth = (unsigned int)(srcWidth * scale);
    unsigned int newHeight = (unsigned int)(srcHeight * scale);

    result.width = newWidth;
    result.height = newHeight;

    // Get source image
    XImage *srcImage = XGetImage(display, source, 0, 0, srcWidth, srcHeight,
                                 AllPlanes, ZPixmap);
    if (srcImage == NULL) return result;

    // Get source mask if available
    XImage *srcMaskImage = NULL;
    if (sourceMask != None) {
        srcMaskImage = XGetImage(display, sourceMask, 0, 0, srcWidth, srcHeight,
                                 AllPlanes, ZPixmap);
    }

    // Create destination pixmap
    result.pixmap = XCreatePixmap(display, window, newWidth, newHeight, depth);
    if (result.pixmap == None) {
        XDestroyImage(srcImage);
        if (srcMaskImage) XDestroyImage(srcMaskImage);
        return result;
    }

    // Create destination mask if source has one
    if (srcMaskImage != NULL) {
        result.mask = XCreatePixmap(display, window, newWidth, newHeight, 1);
        if (result.mask == None) {
            XFreePixmap(display, result.pixmap);
            XDestroyImage(srcImage);
            XDestroyImage(srcMaskImage);
            return result;
        }
    }

    // Allocate image data
    char *imageData = (char *)malloc(newWidth * newHeight * 4);
    if (imageData == NULL) {
        XFreePixmap(display, result.pixmap);
        if (result.mask != None) XFreePixmap(display, result.mask);
        XDestroyImage(srcImage);
        if (srcMaskImage) XDestroyImage(srcMaskImage);
        return result;
    }

    // Create destination image
    XImage *destImage = XCreateImage(display, DefaultVisual(display, screen),
                                     depth, ZPixmap, 0, imageData,
                                     newWidth, newHeight, 32, 0);
    if (destImage == NULL) {
        free(imageData);
        XFreePixmap(display, result.pixmap);
        if (result.mask != None) XFreePixmap(display, result.mask);
        XDestroyImage(srcImage);
        if (srcMaskImage) XDestroyImage(srcMaskImage);
        return result;
    }

    // Allocate mask data if needed
    XImage *destMaskImage = NULL;
    if (srcMaskImage != NULL) {
        char *maskData = (char *)malloc((newWidth + 7) / 8 * newHeight);
        if (maskData == NULL) {
            free(imageData);
            XFreePixmap(display, result.pixmap);
            XFreePixmap(display, result.mask);
            XDestroyImage(srcImage);
            XDestroyImage(srcMaskImage);
            return result;
        }

        destMaskImage = XCreateImage(display, DefaultVisual(display, screen),
                                     1, ZPixmap, 0, maskData,
                                     newWidth, newHeight, 8, 0);
        if (destMaskImage == NULL) {
            free(maskData);
            free(imageData);
            XFreePixmap(display, result.pixmap);
            XFreePixmap(display, result.mask);
            XDestroyImage(srcImage);
            XDestroyImage(srcMaskImage);
            return result;
        }
    }

    // Perform nearest-neighbor scaling
    for (unsigned int y = 0; y < newHeight; y++) {
        for (unsigned int x = 0; x < newWidth; x++) {
            unsigned int srcX = (unsigned int)(x / scale);
            unsigned int srcY = (unsigned int)(y / scale);

            // Clamp to source bounds
            if (srcX >= srcWidth) srcX = srcWidth - 1;
            if (srcY >= srcHeight) srcY = srcHeight - 1;

            unsigned long pixel = XGetPixel(srcImage, srcX, srcY);
            XPutPixel(destImage, x, y, pixel);

            // Scale mask if it exists
            if (destMaskImage != NULL) {
                unsigned long maskPixel = XGetPixel(srcMaskImage, srcX, srcY);
                XPutPixel(destMaskImage, x, y, maskPixel);
            }
        }
    }

    // Copy scaled image to pixmap
    GC gc = XCreateGC(display, result.pixmap, 0, NULL);
    if (gc != NULL) {
        XPutImage(display, result.pixmap, gc, destImage, 0, 0, 0, 0,
                 newWidth, newHeight);
        XFreeGC(display, gc);
    }

    // Copy scaled mask to mask pixmap
    if (destMaskImage != NULL) {
        GC maskGc = XCreateGC(display, result.mask, 0, NULL);
        if (maskGc != NULL) {
            XPutImage(display, result.mask, maskGc, destMaskImage, 0, 0, 0, 0,
                     newWidth, newHeight);
            XFreeGC(display, maskGc);
        }

        destMaskImage->data = NULL;
        XDestroyImage(destMaskImage);
    }

    // Clean up
    destImage->data = NULL;
    XDestroyImage(destImage);
    XDestroyImage(srcImage);
    if (srcMaskImage) XDestroyImage(srcMaskImage);

    return result;
}
