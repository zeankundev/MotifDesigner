#include "Components.h"
#include "Logger.h"
#include "Misc.h"
#include "ProjectManager.h"
#include "TextDialog.h"
#include <X11/Composite.h>
#include <X11/Intrinsic.h>
#include <Xm/FileSB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>
#include <cstddef>
#include <cstdlib>
#include <string>

typedef void (*ActionCallbackVoid)();
typedef void (*ActionCallbackWidget)(Widget);

struct MenuContent {
    const char* title;
    union {
        ActionCallbackVoid v;
        ActionCallbackWidget w;
        void* raw;
    } callback;
    bool takesWidget; // true if callback.w is valid, false if callback.v is valid
};

enum FilePickerState {
    // OpenProject,
    OpenFile,
    SaveFile,
};

#define MENU_ITEM_VOID(title, func) { title, { .v = (ActionCallbackVoid)func }, false }
#define MENU_ITEM_WIDGET(title, func) { title, { .w = (ActionCallbackWidget)func }, true }

struct MenubarItem {
    const char* title;
    const MenuContent* contents;
    int contentsCount;
};

TextDialog dialog;
FilePickerState pickerState = FilePickerState::OpenFile;

void AfterDialogCBTest(Widget w, XtPointer clientData, XtPointer callData) {
    Logger::log("Received callback");
    std::string value = dialog.GetCurrentDialogValue();
    // Do not store to my personal drive: store it at the user's /home directory by default!
    ProjectManager::SaveIndividualHeaderFile(w, (char*)getenv("HOME"), value);
}
void SpawnDialogTest(Widget w, XtPointer clientData, XtPointer callData) {
    Widget parent = (Widget)clientData;
    dialog.SpawnDialogInstance(parent, "Enter a class name.\nThis is required so we can generate proper header files", AfterDialogCBTest);
}

void _TEST_FileCallback_OnCancel(Widget w, XtPointer clientData, XtPointer callData) {
    Widget fileDialog = (Widget)clientData;
    Logger::log("Nothing is picked to be saved");
    XtDestroyWidget(fileDialog);
}

void _TEST_FileCallback_OnSuccess(Widget w, XtPointer clientData, XtPointer callData) {
    Widget fileDialog = (Widget)clientData;
    // Find the filepath that the user picked
    XmFileSelectionBoxCallbackStruct *callback = static_cast<XmFileSelectionBoxCallbackStruct*>(callData);
    char *filePath = nullptr;
    XmStringGetLtoR(callback->value, XmFONTLIST_DEFAULT_TAG, &filePath);

    if (filePath != nullptr) {
        Logger::log(("User picked: " + std::string(filePath)).c_str());
    } else {
        Logger::log("User cancelled the dialog");
    }
    
    switch (pickerState) {
        case FilePickerState::OpenFile: {
            Logger::log("Requested to open a visual file. (Not Implemented yet)");
            break;
        }
        case FilePickerState::SaveFile: {
            Logger::log("Requested to save a visual file. (Not Implemented yet)");
            break;
        }
    }

    XtDestroyWidget(fileDialog);
}

void _TEST_SaveFileDialog(Widget w, XtPointer clientData, XtPointer callData) {
    Widget parent = (Widget)clientData;
    pickerState = FilePickerState::SaveFile;
    Arg args[16];
    int n = 0;

    XtSetArg(args[n], XmNdialogTitle, XmStringCreateLocalized((char*)"Save Visual File")); n++;
    Widget saveDialog = XmCreateFileSelectionDialog(parent, (char*)"SaveFileDialog", args, n);
    XtAddCallback(saveDialog, XmNcancelCallback, _TEST_FileCallback_OnCancel, (XtPointer)saveDialog);
    XtAddCallback(saveDialog, XmNokCallback, _TEST_FileCallback_OnSuccess, (XtPointer)saveDialog);
    XtManageChild(saveDialog);
}

void _TEST_OpenFileDialog(Widget w, XtPointer clientData, XtPointer callData) {
    Widget parent = (Widget)clientData;
    pickerState = FilePickerState::OpenFile;
    Arg args[16];
    int n = 0;

    XtSetArg(args[n], XmNdialogTitle, XmStringCreateLocalized((char*)"Open Visual File")); n++;
    Widget openDialog = XmCreateFileSelectionDialog(parent, (char*)"OpenFileDialog", args, n);
    XtAddCallback(openDialog, XmNcancelCallback, _TEST_FileCallback_OnCancel, (XtPointer)openDialog);
    XtAddCallback(openDialog, XmNokCallback, _TEST_FileCallback_OnSuccess, (XtPointer)openDialog);
    XtManageChild(openDialog);
}

Widget Components::RenderMenubar(Widget parent) {
    Arg args[20];
    int n = 0;

    Logger::log("Initializing menubar");

    MenuContent fileContents[] = {
        // todo 1.2: implement project structuring for projects
        // MENU_ITEM_VOID("New Project", nullptr),
        // todo 1.0: add basic visual file editing, saving, etc.
        // MENU_ITEM_VOID("New Visual File", nullptr),
        // MENU_ITEM_VOID("Open Project", nullptr),
        MENU_ITEM_VOID("Open Visual File", _TEST_OpenFileDialog),
        MENU_ITEM_VOID("Save Visual File", _TEST_SaveFileDialog),
        MENU_ITEM_VOID("Exit", std::quick_exit),
    };

    MenuContent workflowContents[] = {
        // todo 1.2: implement IDE-like options and user code testing
        // MENU_ITEM_VOID("Open C++ Editor", nullptr),
        // MENU_ITEM_VOID("Regenerate Visual .h file", nullptr),
        MENU_ITEM_VOID("Save Individual .h file", SpawnDialogTest),
        // MENU_ITEM_VOID("Build and Run", nullptr),
        // MENU_ITEM_VOID("Build Only", nullptr),
        // MENU_ITEM_VOID("Package to AppImage", nullptr),
    };

    MenuContent helpContents[] = {
        MENU_ITEM_WIDGET("About Motif Designer", MiscFunctions::ShowAboutDialog),
    };

    MenubarItem menuItems[] = {
        { "File", fileContents, sizeof(fileContents) / sizeof(fileContents[0]) },
        { "Workflow", workflowContents, sizeof(workflowContents) / sizeof(workflowContents[0]) },
        { "Help", helpContents, sizeof(helpContents) / sizeof(helpContents[0]) },
    };

    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
    Widget menuBar = XmCreateMenuBar(parent, (char*)"Menubar", args, n);
    XtManageChild(menuBar);

    int menuCount = sizeof(menuItems) / sizeof(menuItems[0]);
    for (int i = 0; i < menuCount; i++) {

        // 1. Reset n and create the pulldown menu container
        n = 0;
        Widget pulldown = XmCreatePulldownMenu(menuBar, (char*)menuItems[i].title, args, n);
        // CRITICAL FIX: Do NOT call XtManageChild(pulldown) here!

        // 2. Populate the pulldown menu with items
        for (int j = 0; j < menuItems[i].contentsCount; j++) {
            n = 0;
            Widget menuItem = XmCreatePushButton(pulldown, (char*)menuItems[i].contents[j].title, args, n);
            XtManageChild(menuItem);

            if (menuItems[i].contents[j].takesWidget && menuItems[i].contents[j].callback.w != nullptr) {
                XtAddCallback(menuItem, XmNactivateCallback, (XtCallbackProc)menuItems[i].contents[j].callback.w, (XtPointer)parent);
            } else if (!menuItems[i].contents[j].takesWidget && menuItems[i].contents[j].callback.v != nullptr) {
                XtAddCallback(menuItem, XmNactivateCallback, (XtCallbackProc)menuItems[i].contents[j].callback.v, (XtPointer)parent);
            }
        }

        // 3. Attach the pulldown menu to a Cascade Button on the Menubar
        n = 0;
        XtSetArg(args[n], XmNsubMenuId, pulldown); n++;
        Widget menuButton = XmCreateCascadeButton(menuBar, (char*)menuItems[i].title, args, n);
        XtManageChild(menuButton);
    }

    return menuBar;
}
