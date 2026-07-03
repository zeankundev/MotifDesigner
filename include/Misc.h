#ifndef MISC_H
#define MISC_H
#include "CanvasInterface.h"
#include <X11/Intrinsic.h>
#include <fstream>
#include <vector>
class MiscFunctions {
    public:
        static void ShowAboutDialog(Widget parent);
};
class Parser {
    public:
        enum Errors {
            OK,
            INVALID_VERSION,
            UNKNOWN_VERSION,
            SYNTAX_ERROR
        };
        static std::vector<CanvasInterface::EditorWidgetInstance> ParseVisualFile(std::ifstream& stream);
        // Returns true for a successful compilation, false for a failed one.
        // This function must be defined externally on another file
        static void OnParserFinished(Errors error, char* message);
};
#endif
