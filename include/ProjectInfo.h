#ifndef PROJECTINFO_H
#define PROJECTINFO_H
#include "Logger.h"
#include <regex>
#include <string>
class ProjectInfo {
    private:
        std::string projectName = "NewProject"; // Requires strict regexing to prevent spaces on .h file
        std::string lastKnownValidProjectName = "";

    public:
        std::string GetProjectName() { return projectName; }
        void SetProjectName(std::string name) {
            bool isOkayToUse = std::regex_match(name, std::regex("^[a-zA-Z0-9_]+$"));
            if (isOkayToUse) {
                projectName = name;
                lastKnownValidProjectName = name;
            } else {
                Logger::log(
                    (std::string("Ignoring invalid project name: ") + name).c_str());
                projectName = lastKnownValidProjectName;
            }
        }
};
#endif