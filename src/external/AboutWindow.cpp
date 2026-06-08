#include "Misc.h"
#include <X11/ICE/ICElib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <Xm/XmStrDefs.h>
#include <X11/Composite.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <X11/Intrinsic.h>
#include <X11/xpm.h>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include "../xpm/MotifDesignerLogo.xpm"
#include "Xm/Xm.h"

// Structure to hold pixmap data for transparency rendering
typedef struct {
    Pixmap pixmap;
    Pixmap mask;
    unsigned int width;
    unsigned int height;
    Display *display;
} PixmapWithMask;

void CloseDialog(Widget w, XtPointer clientData, XtPointer callData) {
    Widget dialog = (Widget)clientData;
    XtUnmanageChild(dialog);
}

// Expose/redraw callback to draw pixmap with transparency
static void ExposeCallback(Widget w, XtPointer clientData, XtPointer callData) {
    PixmapWithMask *pixmapData = (PixmapWithMask *)clientData;
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

// Helper function to scale both pixmap and mask maintaining transparency
static PixmapWithMask ScalePixmapWithMask(Display *display, Pixmap source,
                                          Pixmap sourceMask,
                                          unsigned int srcWidth, unsigned int srcHeight,
                                          unsigned int targetWidth, unsigned int targetHeight) {
    PixmapWithMask result = {None, None, 0, 0, display};
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

void MiscFunctions::ShowAboutDialog(Widget parent) {
    Arg args[12];
    int n = 0;

    // Create the dialog
    Widget aboutDialog = XmCreateFormDialog(parent, (char*)"AboutDialog", NULL, 0);
    Widget dialogShell = XtParent(aboutDialog);

    // Configure dialog shell
    n = 0;
    XtSetArg(args[n], XmNnoResize, True); n++;
    XtSetArg(args[n], XmNminWidth, 350); n++;
    XtSetArg(args[n], XmNminHeight, 230); n++;
    XtSetArg(args[n], XmNmaxWidth, 350); n++;
    XtSetArg(args[n], XmNmaxHeight, 230); n++;
    XtSetValues(dialogShell, args, n);

    Display *display = XtDisplay(parent);
    Window window = RootWindow(display, DefaultScreen(display));

    // Target dimensions
    const unsigned int TARGET_WIDTH = 350;
    const unsigned int TARGET_HEIGHT = 150;

    // Load XPM from embedded data with transparency support
    Pixmap originalPixmap = None;
    Pixmap originalMask = None;
    XpmAttributes attributes;
    memset(&attributes, 0, sizeof(XpmAttributes));
    attributes.valuemask = XpmSize | XpmReturnPixels;

    int status = XpmCreatePixmapFromData(display, window, MotifDesignerLogo,
                                         &originalPixmap, &originalMask, &attributes);

    PixmapWithMask scaledResult = {None, None, 0, 0, display};

    if (status == XpmSuccess && originalPixmap != None) {
        // Scale the pixmap and mask to target dimensions
        scaledResult = ScalePixmapWithMask(display, originalPixmap, originalMask,
                                           attributes.width, attributes.height,
                                           TARGET_WIDTH, TARGET_HEIGHT);
        XFreePixmap(display, originalPixmap);

        if (originalMask != None) {
            XFreePixmap(display, originalMask);
        }
    }

    // Create drawing area for transparent pixmap display
    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopOffset, 0); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNwidth, TARGET_WIDTH + 20); n++;
    XtSetArg(args[n], XmNheight, TARGET_HEIGHT - 60); n++;

    Widget logoArea = XmCreateDrawingArea(aboutDialog, (char*)"LogoArea", args, n);

    // Store pixmap data for access in callback
    PixmapWithMask *pixmapData = new PixmapWithMask(scaledResult);

    if (scaledResult.pixmap != None) {
        // Add expose callback to draw with transparency
        XtAddCallback(logoArea, XmNexposeCallback, ExposeCallback, (XtPointer)pixmapData);
    }

    XtManageChild(logoArea);

    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, logoArea); n++;
    XtSetArg(args[n], XmNtopOffset, 10); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 20); n++;
    XtSetArg(args[n], XmNlabelString, XmStringCreateLocalized((char*)"MotifDesigner v0.5")); n++;
    Widget aboutText = XmCreateLabel(aboutDialog, (char*)"AboutText", args, n);
    XtManageChild(aboutText);

    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, aboutText); n++;
    XtSetArg(args[n], XmNtopOffset, 4); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 20); n++;
    XtSetArg(args[n], XmNlabelString, XmStringCreateLocalized((char*)"Copyright (c) 2026 zeankun.")); n++;
    Widget aboutText2 = XmCreateLabel(aboutDialog, (char*)"AboutText2", args, n);
    XtManageChild(aboutText2);

    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, aboutText2); n++;
    XtSetArg(args[n], XmNtopOffset, 4); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 20); n++;
    XtSetArg(args[n], XmNlabelString, XmStringCreateLocalized((char*)"Made specifically for Hack Club Stardance.")); n++;
    Widget aboutText3 = XmCreateLabel(aboutDialog, (char*)"AboutText2", args, n);
    XtManageChild(aboutText3);

    // Create close button
    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, aboutText3); n++;
    XtSetArg(args[n], XmNtopOffset, 10); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 120); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightOffset, 120); n++;

    Widget closeButton = XmCreatePushButton(aboutDialog, (char*)"OK", args, n);
    XtAddCallback(closeButton, XmNactivateCallback, CloseDialog, (XtPointer)aboutDialog);
    XtManageChild(closeButton);

    // Display the dialog
    XtManageChild(aboutDialog);
}
