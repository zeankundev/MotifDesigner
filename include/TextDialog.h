#ifndef TEXTDIALOG_H
#define TEXTDIALOG_H
#include <X11/Intrinsic.h>
#include <string>
class TextDialog {
public:
    using DialogCallback = void (*)();
    Widget SpawnDialogInstance(Widget parent, std::string message, DialogCallback callback);
    std::string GetCurrentDialogValue();
};
#endif