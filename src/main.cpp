#include <Xm/Xm.h>
#include <Xm/Form.h>
#include "Components.h"
#include "IMCPasser.h"
#include "Misc.h"
#include <X11/Shell.h>
#include <Xm/PushB.h>
#include <X11/Composite.h>
#include <Xm/XmStrDefs.h>
#include <X11/Intrinsic.h>

using namespace std;

void ButtonPushCallback(Widget w, XtPointer clientData, XtPointer callData) {
    IMCPasser::TestStdOut("Hello while clicking!");
}

void ShowAboutCallback(Widget w, XtPointer clientData, XtPointer callData) {
    MiscFunctions::ShowAboutDialog(w);
}

Widget InitializeUI(Widget parent) {
    Arg args[10];
    int n = 0;
    Widget mainLayout = XmCreateForm(parent, (char*)"mainLayout", args, 0);
    XtManageChild(mainLayout);
    Components::RenderToolbar(mainLayout);
    return mainLayout;
}

int main(int argc, char **argv) {
    XtAppContext context;
    Widget shell = XtVaOpenApplication(
        &context, "MotifDesigner",
        NULL, 0, &argc, argv, NULL,
        sessionShellWidgetClass,
        XmNwidth, 1024,
        XmNheight, 600,
        NULL);
    InitializeUI(shell);
    XtRealizeWidget(shell);
    XtAppMainLoop(context);
    return 0;
}
