#include "CanvasInterface.h"
#include "Logger.h"
#include "Misc.h"
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
using EditorWidget = CanvasInterface::EditorWidgetInstance; // <- VALID

string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

static const unordered_map<string, CanvasInterface::ToolTypes> typeDatas = {
    {"Button", CanvasInterface::ToolTypes::Button},
    {"Label", CanvasInterface::ToolTypes::Label},
    {"TextField", CanvasInterface::ToolTypes::TextField},
    {"Toggle", CanvasInterface::ToolTypes::Toggle},
    {"Frame", CanvasInterface::ToolTypes::Frame}
};

vector<EditorWidget> Parser::ParseVisualFile(ifstream& stream) {
    vector<EditorWidget> widgets;
    optional<EditorWidget> current;
    string line;
    string fileVersion;
    bool inWidgetBlock = false;
    bool hasVersion = false;
    int num = 0;
    vector<string> tagStack;
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
        if (line == "[MotifDesignerVisualFile]" || line == "[EndMotifDesignerVisualFile]" ||
            line == "[Widgets]" || line == "[EndWidgets]") {
                if (inWidgetBlock && (line == "[EndWidgets]" || line == "[MotifDesignerVisualFile]")) {
                    Logger::log(string(string("[PARSER] [ERR] Syntax error detected at line") + to_string(num) + string(": Unmatched closing tag")).c_str());
                    OnParserFinished(SYNTAX_ERROR, (char*)(string("Syntax error detected at line ") + to_string(num) + string(": Unmatched closing tag")).c_str());
                    break;
                }
                continue;
            }
        if (line.front() == '[' && line.back() == ']') {
            string tag = line.substr(1, line.size() - 2);
            if (tag.rfind("End", 0) == 0) {
                string closingType = tag.substr(3);
                if (tagStack.empty()) {
                    Logger::log((std::string("[PARSER] [ERR] Syntax error detected at line ")
                        + to_string(num) + ": Unexpected closing tag [End" + closingType + "]").c_str());
                    OnParserFinished(SYNTAX_ERROR, (char*)(std::string("Syntax error detected at line ")
                        + to_string(num) + ": Unexpected closing tag [End" + closingType + "] without an opening tag").c_str());
                    break;
                }

                string expectedType = tagStack.back();
                if (closingType != expectedType) {
                    Logger::log((std::string("[PARSER] [ERR] Syntax error detected at line ")
                        + to_string(num) + ": Mismatched closing tag [End" + closingType + "]").c_str());
                    OnParserFinished(SYNTAX_ERROR, (char*)(std::string("Syntax error detected at line ")
                        + to_string(num) + ": Mismatched closing tag [End" + closingType + "] (expected [End" + expectedType + "])").c_str());
                    break;
                }
                tagStack.pop_back();
            } else {
                if (typeDatas.count(tag) == 0 && tag != "MotifDesignerVisualFile" && tag != "Widgets") {
                    Logger::log((std::string("[PARSER] [ERR] Syntax error detected at line ")
                        + to_string(num) + ": Unexpected tag [" + tag + "]").c_str());
                    OnParserFinished(SYNTAX_ERROR, (char*)(std::string("Syntax error detected at line ")
                        + to_string(num) + ": Unexpected tag [" + tag + "]").c_str());
                    break;
                }
                tagStack.push_back(tag);
            }
        }
    }
    if (!tagStack.empty()) {
        string unclosed = tagStack.back(); // last opened, first missing close
        Logger::log((string("[PARSER] [ERR] Syntax error: Unclosed tag [")
            + unclosed + "]").c_str());
        OnParserFinished(
            SYNTAX_ERROR,
            (char*)(string("Syntax error detected: Missing closing tag [End")
                + unclosed + "] for [" + unclosed + "]").c_str()
        );
    }
    return widgets;
}