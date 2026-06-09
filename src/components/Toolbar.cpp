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
#include <iostream>

class ToolbarButtons {
    public:
        static void CreateToolbarButton(Widget parent, char **xpmData, char *name, void (*callback)(Widget, XtPointer, XtPointer)) {
            Arg args[10];
            int n = 0;
            Pixmap pixmap = PixmapManager::XpmToPixmap(parent, xpmData);
            if (pixmap == None) return;
            XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
            XtSetArg(args[n], XmNlabelPixmap, pixmap); n++;
            XtSetArg(args[n], XmNwidth, 30); n++;
            XtSetArg(args[n], XmNheight, 30); n++;
            XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++;
            Widget button = XmCreatePushButton(parent, name, args, n);
            XtAddCallback(button, XmNactivateCallback, callback, NULL);
            XtManageChild(button);
        }
};

void SelectButtonCallback(Widget widget, XtPointer clientData, XtPointer callData) {
    std::cout << "Test select?" << std::endl;
}

Widget Components::RenderToolbar(Widget parent) {
    Arg args[10];
    int n = 0;

    // Pad the frame properly.
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNwidth, 40); n++;
    Widget mainToolbar = XmCreateFrame(parent, (char*)"LeftToolbar", args, n);
    XtManageChild(mainToolbar);

    n = 0;
    /* Add inner padding so children inside the Form have breathing room. */
    XtSetArg(args[n], XmNmarginWidth, 6); n++;
    XtSetArg(args[n], XmNmarginHeight, 6); n++;
    Widget toolbarForm = XmCreateForm(mainToolbar, (char*)"ToolbarForm", args, n);
    XtManageChild(toolbarForm);

    ToolbarButtons::CreateToolbarButton(toolbarForm, SelectIcon, (char*)"SelectButton", SelectButtonCallback);

    return mainToolbar;
}
