#include "CanvasInterface.h"
#include "Components.h"
#include "../xpm/SelectIcon.xpm"
#include "../xpm/PushButtonIcon.xpm"
#include "../xpm/LabelIcon.xpm"
#include "../xpm/TextFieldIcon.xpm"
#include "../xpm/CheckmarkIcon.xpm"
#include "PixmapManager.h"
#include <X11/Composite.h>
#include <X11/ICE/ICElib.h>
#include <X11/Intrinsic.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>
#include <iostream>

class ToolbarButtons {
    public:
        // Keep the static method signature and allow stacking without caller-side variables.
        static void CreateToolbarButton(Widget parent, char **xpmData, char *name, void (*callback)(Widget, XtPointer, XtPointer), char *tooltip = NULL) {
            Arg args[32];
            int n = 0;
            Pixmap pixmap = PixmapManager::XpmToPixmap(parent, xpmData);
            if (pixmap == None) return;

            XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
            XtSetArg(args[n], XmNlabelPixmap, pixmap); n++;
            XtSetArg(args[n], XmNwidth, 30); n++;
            XtSetArg(args[n], XmNheight, 30); n++;
            XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++;

            // If parent is a Form, attach this button below the last managed child so calls
            // can remain as single static invocations and buttons will stack vertically.
            Widget *children = NULL;
            Cardinal numChildren = 0;
            // Use XtGetValues to obtain the current children of the parent form.
            XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); // default; may be overwritten
            n++; // reserve slot for topAttachment

            // Temporarily get children using separate Arg array to avoid disturbing main args
            Arg qargs[2];
            XtSetArg(qargs[0], XmNchildren, &children);
            XtSetArg(qargs[1], XmNnumChildren, &numChildren);
            XtGetValues(parent, qargs, 2);

            if (numChildren > 0) {
                // Attach below the last child so widgets stack in the order created
                // Replace the previously reserved topAttachment slot with widget attachment args
                n--; // rewind to overwrite reserved topAttachment
                XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
                XtSetArg(args[n], XmNtopWidget, children[numChildren - 1]); n++;
            }

            // Always attach left to the form for a vertical toolbar
            XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
            XtSetArg(args[n], XmNtoolTipEnable, True); n++;
            XtSetArg(args[n], XmNtraversalOn, True); n++;

            // Add tooltip for accessibility
            if (tooltip != NULL) {
                XtSetArg(args[n], XmNtoolTipString, XmStringCreateLocalized(tooltip)); n++;
            }

            Widget button = XmCreatePushButton(parent, name, args, n);
            if (callback != nullptr) {
                XtAddCallback(button, XmNactivateCallback, callback, NULL);
            }
            XtManageChild(button);

            // If children was allocated by Motif, free it. According to Motif, XmNchildren returns
            // a pointer owned by the widget; do not free. So do nothing here.
            (void)children;
        }
};

void SelectButtonCallback(Widget widget, XtPointer clientData, XtPointer callData) {
    g_canvas->SetTool(CanvasInterface::ToolTypes::Select);
}

void PushButtonCallback(Widget widget, XtPointer clientData, XtPointer callData) {
    g_canvas->SetTool(CanvasInterface::ToolTypes::Button);
}

void LabelButtonCallback(Widget widget, XtPointer clientData, XtPointer callData) {
    g_canvas->SetTool(CanvasInterface::ToolTypes::Label);
}

void TextFieldButtonCallback(Widget widget, XtPointer clientData, XtPointer callData) {
    g_canvas->SetTool(CanvasInterface::ToolTypes::TextField);
}

void CheckmarkButtonCallback(Widget widget, XtPointer clientData, XtPointer callData) {
    g_canvas->SetTool(CanvasInterface::ToolTypes::Toggle);
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
    XtSetArg(args[n], XmNmarginWidth, 4); n++;
    XtSetArg(args[n], XmNmarginHeight, 4); n++;
    XtSetArg(args[n], XmNwidth, 36); n++;
    Widget toolbarForm = XmCreateForm(mainToolbar, (char*)"ToolbarForm", args, n);
    XtManageChild(toolbarForm);

    ToolbarButtons::CreateToolbarButton(toolbarForm, SelectIcon, (char*)"SelectButton", SelectButtonCallback, "Select Component");
    ToolbarButtons::CreateToolbarButton(toolbarForm, PushButtonIcon, (char*)"PushButton", PushButtonCallback, "Add Push Button");
    ToolbarButtons::CreateToolbarButton(toolbarForm, LabelIcon, (char*)"LabelButton", LabelButtonCallback, "Add Label");
    ToolbarButtons::CreateToolbarButton(toolbarForm, TextFieldIcon, (char*)"TextFieldButton", TextFieldButtonCallback, "Add Text Field");
    ToolbarButtons::CreateToolbarButton(toolbarForm, CheckmarkIcon, (char*)"ToggleButton", CheckmarkButtonCallback, "Add Toggle");
    // todo 0.8: implement more options (toggle, frame) as tool choices

    return mainToolbar;
}
