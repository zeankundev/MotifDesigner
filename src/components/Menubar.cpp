#include "Components.h"
#include "Logger.h"
#include "Misc.h"
#include "ProjectManager.h"
#include "TextDialog.h"
#include <X11/Intrinsic.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>
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

#define MENU_ITEM_VOID(title, func) { title, { .v = (ActionCallbackVoid)func }, false }
#define MENU_ITEM_WIDGET(title, func) { title, { .w = (ActionCallbackWidget)func }, true }

struct MenubarItem {
    const char* title;
    const MenuContent* contents;
    int contentsCount;
};
TextDialog dialog;
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

Widget Components::RenderMenubar(Widget parent) {
    Arg args[20];
    int n = 0;

    Logger::log("Initializing menubar");

    MenuContent fileContents[] = {
        // MENU_ITEM_VOID("New Project", nullptr),
        // MENU_ITEM_VOID("New Visual File", nullptr),
        // MENU_ITEM_VOID("Open Project", nullptr),
        // MENU_ITEM_VOID("Open Visual File", nullptr),
        // MENU_ITEM_VOID("Save Visual File", nullptr),
        MENU_ITEM_VOID("Exit", std::quick_exit),
    };

    MenuContent workflowContents[] = {
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
