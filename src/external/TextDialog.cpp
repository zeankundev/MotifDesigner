#include "TextDialog.h"
#include "Logger.h"
#include <X11/Composite.h>
#include <X11/ICE/ICElib.h>
#include <X11/Intrinsic.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/TextF.h>
#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>
#include <regex>
#include <sstream>
#include <string>
#include <regex>
#include <vector>

Widget previousWidget = NULL;
Widget textFieldWidget = NULL;

// Bundles both pieces of state that OnSubmitCallback needs.
// Heap-allocated in SpawnDialogInstance and freed inside OnSubmitCallback.
struct DialogContext {
    Widget dialog;
    void (*userCallback)();
};

void OnSubmitCallback(Widget w, XtPointer clientData, XtPointer callData) {
    Logger::log("Executing callback");
    DialogContext* ctx = reinterpret_cast<DialogContext*>(clientData);
    if (ctx->userCallback) {
        ctx->userCallback();
    }
    XtUnmanageChild(ctx->dialog);
    delete ctx;
}

void ValidateTextValue(Widget w, XtPointer clientData, XtPointer callData) {
    Logger::log("Validating text value");
    // w may be the OK button, not the text field — always read from textFieldWidget
    char* rawText = XmTextFieldGetString(textFieldWidget);
    std::string text(rawText ? rawText : "");
    XtFree(rawText);
    bool isValidText = std::regex_match(text, std::regex("^[A-Za-z0-9_]+$"));
    if (isValidText) {
        Logger::log("Valid text value");
        OnSubmitCallback(w, clientData, callData);
    } else {
        Logger::log("Invalid, throwing error");
        Arg args[1];
        XtSetArg(args[0], XmNmessageString, XmStringCreateLocalized((char*)"Invalid format! Use only letters, numbers, and underscores."));
        Widget errorDialog = XmCreateErrorDialog(XtParent(textFieldWidget), (char*)"ErrorDialog", args, 1);
        Widget helpButton = XmMessageBoxGetChild(errorDialog, XmDIALOG_HELP_BUTTON);
        XtUnmanageChild(helpButton);
        XtManageChild(errorDialog);
    }
}

Widget TextDialog::SpawnDialogInstance(Widget parent, std::string message, TextDialog::DialogCallback callback) {
    Arg args[10];
    int n = 0;
    std::vector<std::string> linedText;
    std::string segment;
    std::stringstream ss(message);

    while (std::getline(ss, segment, '\n')) {
        linedText.push_back(segment);
    }

    Widget textDialog = XmCreateFormDialog(parent, (char*)"TextDialog", args, n);
    Widget textDialogShell = XtParent(textDialog);

    n = 0;
    XtSetArg(args[n], XmNnoResize, True); n++;
    XtSetArg(args[n], XmNminWidth, 400); n++;
    XtSetArg(args[n], XmNminHeight, 170); n++;
    XtSetArg(args[n], XmNmaxWidth, 400); n++;
    XtSetArg(args[n], XmNmaxHeight, 170); n++;
    XtSetArg(args[n], XmNtitle, "Text Dialog"); n++;
    XtSetValues(textDialogShell, args, n);

    Display *display = XtDisplay(parent);
    Window win = RootWindow(display, DefaultScreen(display));

    if (!linedText.empty()) {
        for (size_t i = 0; i < linedText.size(); i++) {
            n = 0;
            if (i == 0) {
                XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
            } else {
                // Subsequent labels attach to the bottom of the previous label
                XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
                XtSetArg(args[n], XmNtopWidget, previousWidget); n++; // This is the missing link!
            }
            
            XtSetArg(args[n], XmNtopOffset, 10); n++;
            XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
            XtSetArg(args[n], XmNleftOffset, 20); n++;
            XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
            XtSetArg(args[n], XmNrightOffset, 20); n++;
            
            // Note: Remember to free XmString components in production to avoid memory leaks!
            XmString labelStr = XmStringCreateLocalized(const_cast<char*>(linedText[i].c_str()));
            XtSetArg(args[n], XmNlabelString, labelStr); n++;
            
            Widget label = XmCreateLabel(textDialog, (char*)"Label", args, n);
            XtManageChild(label);
            
            // Clean up the XmString
            XmStringFree(labelStr);
            
            // Update the pointer so the next loop cycle points to this label
            previousWidget = label; 
        }
    }

    n = 0;
    if (previousWidget != NULL) {
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
        XtSetArg(args[n], XmNtopWidget, previousWidget); n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNtopOffset, 10); n++;
        XtSetArg(args[n], XmNleftOffset, 20); n++;
        XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNrightOffset, 20); n++;

        textFieldWidget = XmCreateTextField(textDialog, (char*)"TextField", args, n);
        XtManageChild(textFieldWidget);
    } else {
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNtopOffset, 5); n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNleftOffset, 20); n++;
        XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNrightOffset, 20); n++;

        textFieldWidget = XmCreateTextField(textDialog, (char*)"TextField", args, n);
        XtManageChild(textFieldWidget);
    }

    if (callback != nullptr) {
        // Pass both the dialog widget and the user callback via a context struct.
        // OnSubmitCallback will free this allocation.
        DialogContext* ctx = new DialogContext{textDialog, callback};

        // Create and attach an "OK" PushButton
        n = 0;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
        XtSetArg(args[n], XmNtopWidget, textFieldWidget); n++;
        XtSetArg(args[n], XmNtopOffset, 10); n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNleftOffset, 20); n++;
        XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNrightOffset, 20); n++;

        XmString okStr = XmStringCreateLocalized((char*)"OK");
        XtSetArg(args[n], XmNlabelString, okStr); n++;

        Widget okButton = XmCreatePushButton(textDialog, (char*)"OKButton", args, n);
        XtManageChild(okButton);
        XmStringFree(okStr);

        XtAddCallback(okButton, XmNactivateCallback, ValidateTextValue, (XtPointer)ctx);

        // Make the OK button the default — pressing Enter anywhere in the dialog routes
        // through this button, so we do NOT also register XmNactivateCallback on the
        // text field directly. Both firing on the same ctx would delete it twice (use-after-free).
        XtSetArg(args[0], XmNdefaultButton, okButton);
        XtSetValues(textDialog, args, 1);
    }

    XtRealizeWidget(textDialog);
    XtManageChild(textDialog);  

    return textDialog;
}

std::string TextDialog::GetCurrentDialogValue() {
    return (std::string)XmTextFieldGetString(textFieldWidget);
}