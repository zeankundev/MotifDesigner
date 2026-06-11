#include "Misc.h"
#include "PixmapManager.h"
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
#include <string>
#include "../xpm/MotifDesignerLogo.xpm"
#include "Xm/Xm.h"

// Structure to hold pixmap data for transparency rendering

void CloseDialog(Widget w, XtPointer clientData, XtPointer callData) {
    Widget dialog = (Widget)clientData;
    XtUnmanageChild(dialog);
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
    XtSetArg(args[n], XmNtitle, "About MotifDesigner"); n++;
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

    PixmapManager::PixmapWithMask scaledResult = {None, None, 0, 0, display};

    if (status == XpmSuccess && originalPixmap != None) {
        // Scale the pixmap and mask to target dimensions
        scaledResult = PixmapManager::ScalePixmapWithMask(display, originalPixmap, originalMask,
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
    PixmapManager::PixmapWithMask *pixmapData = new PixmapManager::PixmapWithMask(scaledResult);

    if (scaledResult.pixmap != None) {
        // Add expose callback to draw with transparency
        XtAddCallback(logoArea, XmNexposeCallback, PixmapManager::ExposeCallback, (XtPointer)pixmapData);
    }

    XtManageChild(logoArea);

    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, logoArea); n++;
    XtSetArg(args[n], XmNtopOffset, 10); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 20); n++;
    std::string label = std::string("MotifDesigner v") + PROJECT_VERSION_STR;
    XtSetArg(args[n], XmNlabelString, XmStringCreateLocalized(const_cast<char*>(label.c_str()))); n++;
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
