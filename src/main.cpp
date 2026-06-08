#include <iostream>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include "IMCPasser.h"
#include <X11/Shell.h>
#include <Xm/PushB.h>
#include <X11/Composite.h>
#include <Xm/XmStrDefs.h>
#include <X11/Intrinsic.h>

using namespace std;

void ButtonPushCallback(Widget w, XtPointer clientData, XtPointer callData) {
    IMCPasser::TestStdOut("Hello while clicking!");
}

Widget InitializeUI(Widget parent) {
    Arg args[10];
    int n = 0;
    Widget mainLayout = XmCreateForm(parent, (char*)"mainLayout", args, 0);
    XtManageChild(mainLayout);
    /* Attach the button to the form and set offsets. Form ignores XmNx/XmNy so use
       attachment + offsets to control absolute positioning inside the Form. */
    XtSetArg(args[n], XmNx, 30); n++;
    XtSetArg(args[n], XmNy, 30); n++;
    XtSetArg(args[n], XmNwidth, 200); n++;
    XtSetArg(args[n], XmNheight, 30); n++;
    XtSetArg(args[n], XmNlabelString, XmStringCreateLocalized((char*)"Send to IMC passer as test")); n++;
    Widget testButton = XmCreatePushButton(mainLayout, (char*)"testButton", args, n);
    XtManageChild(testButton);
    XtAddCallback(testButton, XmNactivateCallback, ButtonPushCallback, NULL);
    return mainLayout;
}

int main(int argc, char **argv) {
    XtAppContext context;
    Widget shell = XtVaOpenApplication(
        &context, "MotifDesigner",
        NULL, 0, &argc, argv, NULL,
        sessionShellWidgetClass,
        XmNwidth, 300,
        XmNheight, 300,
        NULL);
    InitializeUI(shell);
    XtRealizeWidget(shell);
    XtAppMainLoop(context);
    return 0;
}
