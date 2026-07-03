#include "CanvasInterface.h"
#include "Logger.h"
#include "Misc.h"
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

using namespace std;
using EditorWidget = CanvasInterface::EditorWidgetInstance; // <- VALID

string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

vector<EditorWidget> Parser::ParseVisualFile(ifstream& stream) {
    vector<EditorWidget> widgets;
    optional<EditorWidget> current;
    string line;
    string fileVersion;
    bool inWidgetBlock = false;
    bool hasVersion = false;
    int num = 0;
    while (getline(stream, line)) {
        num++;
        line = trim(line);

        if (line.empty() || line.rfind("&&", 0) == 0) continue;
        if (line.rfind("Version=", 0) == 0) {
            fileVersion = line.substr(8);
            // Tolerate newer versions to open files from older version
            // If we are opening a file which contains from a newer version, decline it.
            float thisClientVersion = (float)atof(PROJECT_VERSION_STR);
            float fileVersionInFloat = (float)atof(fileVersion.c_str());
            if (fileVersionInFloat > thisClientVersion) {
                Logger::log("[PARSER] [ERR] File version is newer than client version.");
                OnParserFinished(INVALID_VERSION, (char*)(string("This file is made for a newer version of MotifDesigner at version ") + fileVersion + string(". Please use a newer version of MotifDesigner to open it") ).c_str());
                break;
            } else if (fileVersionInFloat < 0.01 || fileVersion.size() == 0) {
                Logger::log("[PARSER] [ERR] Invalid version");
                OnParserFinished(INVALID_VERSION, (char*)(string("This is an invalid version of MotifDesigner") ).c_str());
                break;
            } else {
                Logger::log("[PARSER] Valid version. Continuing...");
                hasVersion = true;
            }
        }
    }
    return widgets;
}