#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H
#include <X11/Intrinsic.h>
#include <string>
class ProjectManager {
    public:
        static void SaveIndividualHeaderFile(Widget parent, std::string pathToSave, std::string className);
};
#endif