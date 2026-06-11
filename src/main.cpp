#include <Xm/Display.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include "Components.h"
#include "IMCPasser.h"
#include "Logger.h"
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

    Logger::log("Initializing UI");

    // Create main layout that contains menubar and editor layouts
    Widget mainLayout = XmCreateForm(parent, (char*)"mainLayout", args, 0);
    XtManageChild(mainLayout);

    // Create menubar layout (fixed height, full width)
    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    Widget menubarLayout = XmCreateForm(mainLayout, (char*)"menubarLayout", args, n);
    XtManageChild(menubarLayout);
    Components::RenderMenubar(menubarLayout);

    // Create editor layout (stretches to bottom and sides)
    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, menubarLayout); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    Widget editorLayout = XmCreateForm(mainLayout, (char*)"editorLayout", args, n);
    XtManageChild(editorLayout);
    Widget toolbar = Components::RenderToolbar(editorLayout);
    Widget propertiesPanel = Components::RenderPropertiesPanel(editorLayout);
    Components::RenderCanvas(editorLayout, toolbar, propertiesPanel);

    return mainLayout;
}

int main(int argc, char **argv) {
    XtAppContext context;
    Logger::log("Starting shell");
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
