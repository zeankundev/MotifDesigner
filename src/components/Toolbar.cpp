#include "Components.h"
#include "../xpm/SelectIcon.xpm"
#include "PixmapManager.h"
#include <X11/Composite.h>
#include <X11/Intrinsic.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>

Widget Components::RenderToolbar(Widget parent) {
    Arg args[10];
    int n = 0;

    // Pad the frame properly.
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    Widget mainToolbar = XmCreateFrame(parent, (char*)"LeftToolbar", args, n);
    XtManageChild(mainToolbar);

    n = 0;
    Widget toolbarForm = XmCreateForm(mainToolbar, (char*)"ToolbarForm", args, n);
    XtManageChild(toolbarForm);

    n = 0;
    Pixmap selectIcon = PixmapManager::XpmToPixmap(parent, SelectIcon);
    XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
    XtSetArg(args[n], XmNlabelPixmap, selectIcon); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++;
    XtSetArg(args[n], XmNwidth, 28); n++;
    XtSetArg(args[n], XmNheight, 28); n++;
    Widget selectButton = XmCreatePushButton(toolbarForm, (char*)"SelectButton", args, n);
    XtManageChild(selectButton);

    n = 0;
    XtSetArg(args[n], XmNwidth, 28); n++;
    XtSetArg(args[n], XmNheight, 28); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, selectButton); n++;
    XtSetArg(args[n], XmNtopOffset, 10); n++;
    XtSetArg(args[n], XmNlabelString, XmStringCreateLocalized((char*)"S")); n++;
    Widget somethingButton = XmCreatePushButton(toolbarForm, (char*)"GenericButtonButton", args, n);
    XtManageChild(somethingButton);

    return mainToolbar;
}
