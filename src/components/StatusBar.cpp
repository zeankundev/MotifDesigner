#include "Components.h"
#include <X11/Intrinsic.h>
#include <Xm/Label.h>
#include <Xm/Xm.h>
#include <Xm/Frame.h>
#include <string>

Widget statusBarText = nullptr;

Widget StatusBar::RenderStatusBar(Widget parent) {
    Arg args[10];
    int n = 0;

    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    Widget statusBarFrame = XmCreateFrame(parent, (char*)"StatusBarFrame", args, n);
    XtManageChild(statusBarFrame);

    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
    XtSetArg(args[n], XmNlabelString, XmStringCreateLocalized((char*)"Ready")); n++;
    statusBarText = XmCreateLabel(statusBarFrame, (char*)"StatusBarText", args, n);
    XtManageChild(statusBarText);

    return statusBarFrame;
}

void StatusBar::UpdateStatusBar(const char* message) {
    if (statusBarText == nullptr) return;

    Arg args[1];
    XtSetArg(args[0], XmNlabelString, XmStringCreateLocalized((char*)message));
    XtSetValues(statusBarText, args, 1);
}