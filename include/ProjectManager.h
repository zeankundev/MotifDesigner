#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H
#include "Misc.h"
#include <X11/Intrinsic.h>
#include <string>
class ProjectManager {
    private:
        static char* currentVisualFilePath;
    public:
        static void SaveIndividualHeaderFile(Widget parent, std::string pathToSave, std::string className);
        static void ExportVisualFile(char* pathToSave);
        static void SetCurrentVisualFilePath(char* pathToSave) {
            currentVisualFilePath = pathToSave;
            if (pathToSave == nullptr || pathToSave == "") {
                MiscFunctions::UpdateTitle("<empty project>");
            } else {
                MiscFunctions::UpdateTitle(currentVisualFilePath);
            }
        };
        static char* GetCurrentVisualFilePath() {
            return currentVisualFilePath;
        };
};
#endif