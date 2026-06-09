#include <Xm/Display.h>
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
        XmNminWidth, 900,
        XmNminHeight, 600,
        XmNtitle, "MotifDesigner - Something modern for your X11",
        NULL);
    Widget display = XmGetXmDisplay(XtDisplay(shell));
    XtVaSetValues(display, XmNtoolTipEnable, True, NULL);
    InitializeUI(shell);
    XtRealizeWidget(shell);
    XtAppMainLoop(context);
    return 0;
}
