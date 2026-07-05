#include "CanvasInterface.h"
#include "Components.h"
#include "Logger.h"
#include "Misc.h"
#include "ProjectManager.h"
#include "TextDialog.h"
#include <X11/Composite.h>
#include <X11/Intrinsic.h>
#include <Xm/FileSB.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/Text.h>
#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>
#include <cstddef>
#include <cstdlib>
#include <fstream>
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
Widget g_parent;
char* ProjectManager::currentVisualFilePath = nullptr;
bool previousOneHasErrors = false;
bool hasParserErrors = false;

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

void FileCallback_OnCancel(Widget w, XtPointer clientData, XtPointer callData) {
    Widget fileDialog = (Widget)clientData;
    Logger::log("Nothing is picked to be saved");
    XtDestroyWidget(fileDialog);
}

std::string getFilename(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) return path;
    return path.substr(lastSlash + 1);
}

// Keep track of what the user is typing so directory changes don't wipe it
static std::string g_LastTypedFilename = "NewVisualFile.vfl";

// Callback that tracks the text field changes safely without modifying them
void FileCallback_OnTextChange(Widget w, XtPointer clientData, XtPointer callData) {
    char *currentText = XmTextGetString(w);
    if (!currentText) return;

    std::string currentPath(currentText);
    XtFree(currentText);

    // Extract just the filename component
    size_t lastSlash = currentPath.find_last_of('/');
    std::string filename = (lastSlash == std::string::npos) ? currentPath : currentPath.substr(lastSlash + 1);

    // Only update our cache if they actually typed something containing our extension
    if (!filename.empty() && filename.find(".vfl") != std::string::npos) {
        g_LastTypedFilename = filename;
    }
}

struct DirFixContext {
    Widget fileDialog;
    Widget directoryList;
};

void _DeferredDirFixTimer(XtPointer clientData, XtIntervalId *id) {
    DirFixContext *ctx = static_cast<DirFixContext*>(clientData);
    if (!ctx) return;

    // 1. Fetch the new directory path from Motif
    Arg args[1];
    XmString xmDir = nullptr;
    XtSetArg(args[0], XmNdirectory, &xmDir);
    XtGetValues(ctx->fileDialog, args, 1);

    char *dirPath = nullptr;
    XmStringGetLtoR(xmDir, XmFONTLIST_DEFAULT_TAG, &dirPath);
    std::string newDir = dirPath ? std::string(dirPath) : "";
    XtFree(dirPath);

    // 2. Combine the directory with our cached "sticky" filename
    Widget selectionTextField = XmFileSelectionBoxGetChild(ctx->fileDialog, (unsigned char)XmDIALOG_TEXT);
    if (selectionTextField && !newDir.empty()) {
        std::string finalStickyPath = newDir + g_LastTypedFilename;

        // Force-update the input field with the user's custom name retained
        XmTextSetString(selectionTextField, const_cast<char*>(finalStickyPath.c_str()));
        XmTextSetInsertionPosition(selectionTextField, finalStickyPath.length());
    }

    delete ctx;
}

// Fired when a directory is chosen or double-clicked from the list box
void FileCallback_OnDirSelect(Widget w, XtPointer clientData, XtPointer callData) {
    Widget fileDialog = (Widget)clientData;
    
    DirFixContext *ctx = new DirFixContext{ fileDialog, w };
    XtAppAddTimeOut(XtWidgetToApplicationContext(fileDialog), 1, _DeferredDirFixTimer, (XtPointer)ctx);
}

// Helper to extract just the directory from a full path (including trailing slash)
std::string getDirectory(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) return "./";
    return path.substr(0, lastSlash + 1);
}

void OpenVisualFile(char* filePath) {
    hasParserErrors = false;
    Logger::log("Opening and parsing");
    std::ifstream file(filePath);
    if (!file.is_open()) {
        Logger::log("Failed to open file");
        return;
    }
    std::vector<CanvasInterface::EditorWidgetInstance> widgetInstanceVectors = Parser::ParseVisualFile(file);
    file.close();
    // Log every single widget that was parsed.
    for (auto& widget : widgetInstanceVectors) {
        std::stringstream stream;
        stream  << "[PARSED WIDGET] "
                << "Type=" << (int)widget.type << " "
                << "Name=" << widget.name << " "
                << "Value=" << widget.value << " "
                << "X=" << widget.x << " "
                << "Y=" << widget.y << " "
                << "Width=" << widget.width << " "
                << "Height=" << widget.height;
        Logger::log(stream.str().c_str());
    }

    // Import the vector of widgets into the canvas
    if (!hasParserErrors) {
        g_canvas->ImportVectorOfWidgets(widgetInstanceVectors);
        Logger::log("Imported widgets into the canvas");
        StatusBar::UpdateStatusBar((std::string("Loaded ") + std::string(filePath) + std::string(" successfully.")).c_str());
        ProjectManager::SetCurrentVisualFilePath(filePath);
        previousOneHasErrors = false;
    } else {
        Logger::log("Skipping import because there were parser errors");
    }
}

void OpenVisualFileCallback(Widget, XtPointer client_data, XtPointer) {
    Logger::log("Callback to open visual fired");
    char *filePath = static_cast<char*>(client_data);
    if (!filePath) {
        Logger::log("OpenVisualFileCallback: null filePath");
        return;
    }
    OpenVisualFile(filePath);
}

void FileCallback_OnSuccess(Widget w, XtPointer clientData, XtPointer callData) {
    Widget fileDialog = (Widget)clientData;
    XmFileSelectionBoxCallbackStruct *callback = static_cast<XmFileSelectionBoxCallbackStruct*>(callData);
    char *filePath = nullptr;
    XmStringGetLtoR(callback->value, XmFONTLIST_DEFAULT_TAG, &filePath);

    if (filePath != nullptr) {
        Logger::log(("User picked: " + std::string(filePath)).c_str());
    } else {
        Logger::log("User cancelled the dialog");
        XtDestroyWidget(fileDialog);
        return;
    }
    
    switch (pickerState) {
        case FilePickerState::OpenFile: {
            Logger::log("Requested to open a visual file. (Not Implemented yet)");
            if (g_canvas->GetWidgetList().size() > 0 && !previousOneHasErrors) {
                char *filePathCopy = XtNewString(filePath);
                Arg args[1];
                XtSetArg(args[0], XmNmessageString, XmStringCreateLocalized((char*)"This will wipe the canvas. Are you sure you want to proceed?"));
                Widget question = XmCreateQuestionDialog(g_parent, (char*)"QuestionDialog", args, 1);
                Widget helpButton = XmMessageBoxGetChild(question, (unsigned char)XmDIALOG_HELP_BUTTON);
                XtUnmanageChild(helpButton);
                XtAddCallback(question, XmNokCallback, OpenVisualFileCallback, (XtPointer)filePathCopy);
                XtManageChild(question);
            } else {
                Logger::log("First time opening a visual file in a session");
                OpenVisualFile(filePath);
            }
            break;
        }
        case FilePickerState::SaveFile: {
            Logger::log("Requested to save a visual file. (Not Implemented yet)");
            ProjectManager::SetCurrentVisualFilePath(filePath);
            ProjectManager::ExportVisualFile(filePath);
            break;
        }
    }

    // Free the XmString-allocated memory
    XtDestroyWidget(fileDialog);
}

static std::vector<std::string> g_pendingParserErrors;
static Widget g_errorDialog = nullptr;   // persistent single instance
static bool g_errorDialogVisible = false;

static const char* ErrorToPrefix(Parser::Errors e) {
    switch (e) {
        case Parser::Errors::INVALID_VERSION: return "Invalid version";
        case Parser::Errors::UNKNOWN_VERSION: return "Unknown version";
        case Parser::Errors::SYNTAX_ERROR:    return "Syntax error";
        default:                              return "Parser error";
    }
}

static void AppendParserError(Parser::Errors error, const char* message) {
    std::ostringstream line;
    line << ErrorToPrefix(error);
    if (message && *message) line << ": " << message;
    g_pendingParserErrors.push_back(line.str());
}

static std::string BuildMergedParserErrors() {
    previousOneHasErrors = true;
    hasParserErrors = true;
    std::ostringstream merged;
    bool usePlural = g_pendingParserErrors.size() != 1;
    merged << "Parsing failed with " << g_pendingParserErrors.size() << " error" << (usePlural ? "s" : "") << ":\n\n";
    for (size_t i = 0; i < g_pendingParserErrors.size(); ++i) {
        merged << (i + 1) << ". " << g_pendingParserErrors[i];
        if (i + 1 < g_pendingParserErrors.size()) merged << '\n';
    }
    return merged.str();
}

static void ClearParserErrorState(Widget, XtPointer, XtPointer) {
    g_pendingParserErrors.clear();
    g_errorDialogVisible = false;
    // keep g_errorDialog allocated for reuse
}

static void EnsureErrorDialogCreated() {
    if (g_errorDialog) return;

    Arg args[1];
    XtSetArg(args[0], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);
    g_errorDialog = XmCreateErrorDialog(g_parent, (char*)"ErrorDialog_popup", args, 1);

    Widget helpButton = XmMessageBoxGetChild(g_errorDialog, XmDIALOG_HELP_BUTTON);
    XtUnmanageChild(helpButton);

    // When user closes via OK or Cancel, reset accumulation state
    Widget okBtn = XmMessageBoxGetChild(g_errorDialog, XmDIALOG_OK_BUTTON);
    Widget cancelBtn = XmMessageBoxGetChild(g_errorDialog, XmDIALOG_CANCEL_BUTTON);
    XtAddCallback(okBtn, XmNactivateCallback, ClearParserErrorState, nullptr);
    XtAddCallback(cancelBtn, XmNactivateCallback, ClearParserErrorState, nullptr);
}

void Parser::OnParserFinished(Errors error, char *message) {
    if (error == Parser::Errors::OK) {
        Logger::log("Visual file parsed successfully");
        return;
    }

    AppendParserError(error, message);
    EnsureErrorDialogCreated();

    // Update message on the SAME dialog each time
    std::string merged = BuildMergedParserErrors();
    XmString xmMsg = XmStringCreateLocalized(const_cast<char*>(merged.c_str()));
    XtVaSetValues(g_errorDialog, XmNmessageString, xmMsg, NULL);
    XmStringFree(xmMsg);

    if (!g_errorDialogVisible) {
        XtManageChild(g_errorDialog);  // show once
        g_errorDialogVisible = true;
    }
}

void SaveFileDialog(Widget w, XtPointer clientData, XtPointer callData) {
    if (ProjectManager::GetCurrentVisualFilePath() == nullptr || ProjectManager::GetCurrentVisualFilePath() == "") {
        Widget parent = (Widget)clientData;
        pickerState = FilePickerState::SaveFile;
        Arg args[16];
        int n = 0;

        // 1. Reset the sticky cache to default when the dialog opens
        g_LastTypedFilename = "NewVisualFile.vfl";

        XtSetArg(args[n], XmNdialogTitle, XmStringCreateLocalized((char*)"Save Visual File")); n++;
        
        // 2. Create the dialog frame first so we can extract its default directory context
        Widget saveDialog = XmCreateFileSelectionDialog(parent, (char*)"SaveFileDialog", args, n);

        // 3. Extract the actual current directory Motif is looking at
        n = 0;
        XmString xmDir = nullptr;
        XtSetArg(args[0], XmNdirectory, &xmDir);
        XtGetValues(saveDialog, args, 1);

        char *dirPath = nullptr;
        XmStringGetLtoR(xmDir, XmFONTLIST_DEFAULT_TAG, &dirPath);
        std::string absoluteDir = dirPath ? std::string(dirPath) : "";
        XtFree(dirPath);

        // Fallback security if directory resolution fails
        if (absoluteDir.empty()) {
            const char* home = getenv("HOME");
            absoluteDir = home ? std::string(home) + "/" : "./";
        }

        // 4. Construct the full template path (e.g., /home/zean/Projects/NewVisualFile.vfl)
        std::string fullInitialPath = absoluteDir + g_LastTypedFilename;
        XmString xmDefaultSpec = XmStringCreateLocalized((char*)fullInitialPath.c_str());
        
        // 5. Force Motif to use the full path specification as the baseline text selection
        n = 0;
        XtSetArg(args[n], XmNdirSpec, xmDefaultSpec); n++;
        XtSetValues(saveDialog, args, n);
        XmStringFree(xmDefaultSpec);

        // Clean up UI layout hooks
        Widget helpButton = XmFileSelectionBoxGetChild(saveDialog, (unsigned char)XmDIALOG_HELP_BUTTON);
        XtUnmanageChild(helpButton);

        // A. Track user keystrokes on the input field dynamically
        Widget selectionTextField = XmFileSelectionBoxGetChild(saveDialog, (unsigned char)XmDIALOG_TEXT);
        if (selectionTextField) {
            XtAddCallback(selectionTextField, XmNvalueChangedCallback, FileCallback_OnTextChange, nullptr);
        }

        // B. Catch directory jumps
        Widget dirList = XmFileSelectionBoxGetChild(saveDialog, (unsigned char)XmDIALOG_DIR_LIST);
        if (dirList) {
            XtAddCallback(dirList, XmNbrowseSelectionCallback, FileCallback_OnDirSelect, (XtPointer)saveDialog);
            XtAddCallback(dirList, XmNdefaultActionCallback, FileCallback_OnDirSelect, (XtPointer)saveDialog);
        }

        XtAddCallback(saveDialog, XmNcancelCallback, FileCallback_OnCancel, (XtPointer)saveDialog);
        XtAddCallback(saveDialog, XmNokCallback, FileCallback_OnSuccess, (XtPointer)saveDialog);
        XtManageChild(saveDialog);
    } else {
        ProjectManager::ExportVisualFile(ProjectManager::GetCurrentVisualFilePath());
    }
}

void OpenFileDialog(Widget w, XtPointer clientData, XtPointer callData) {
    Widget parent = (Widget)clientData;
    pickerState = FilePickerState::OpenFile;
    Arg args[16];
    int n = 0;

    XtSetArg(args[n], XmNdialogTitle, XmStringCreateLocalized((char*)"Open Visual File")); n++;
    Widget openDialog = XmCreateFileSelectionDialog(parent, (char*)"OpenFileDialog", args, n);
    XtAddCallback(openDialog, XmNcancelCallback, FileCallback_OnCancel, (XtPointer)openDialog);
    XtAddCallback(openDialog, XmNokCallback, FileCallback_OnSuccess, (XtPointer)openDialog);
    XtManageChild(openDialog);
}

void CreateNewVisualFile(Widget w, XtPointer clientData, XtPointer callData) {
    ProjectManager::SetCurrentVisualFilePath("");
    g_canvas->ImportVectorOfWidgets({});
}
    

void CreateNewVisualFileCB(Widget w, XtPointer clientData, XtPointer callData) {
    Widget parent = (Widget)clientData;
    Arg args[1];
    XtSetArg(args[0], XmNmessageString, XmStringCreateLocalized((char*)"This will create a new visual file. Any unsaved work will be lost. Continue?"));
    Widget question = XmCreateQuestionDialog(parent, (char*)"QuestionDialog", args, 1);
    Widget helpButton = XmMessageBoxGetChild(question, (unsigned char)XmDIALOG_HELP_BUTTON);
    XtUnmanageChild(helpButton);
    XtAddCallback(question, XmNokCallback, CreateNewVisualFile, (XtPointer)question);
    XtManageChild(question);
}

Widget Components::RenderMenubar(Widget parent) {
    g_parent = parent;
    Arg args[20];
    int n = 0;

    Logger::log("Initializing menubar");

    MenuContent fileContents[] = {
        // todo 1.2: implement project structuring for projects
        // MENU_ITEM_VOID("New Project", nullptr),
        // todo 1.0: add basic visual file editing, saving, etc.
        MENU_ITEM_VOID("New Visual File", CreateNewVisualFileCB),
        // MENU_ITEM_VOID("Open Project", nullptr),
        MENU_ITEM_VOID("Open Visual File", OpenFileDialog),
        MENU_ITEM_VOID("Save Visual File", SaveFileDialog),
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
