#include "CanvasInterface.h"
#include "Logger.h"
#include "Misc.h"
#include <cstddef>
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

std::optional<CanvasInterface::ToolTypes> ParseToolType(const std::string& tag) {
    if (tag == "Button") return CanvasInterface::ToolTypes::Button;
    if (tag == "Label") return CanvasInterface::ToolTypes::Label;
    if (tag == "TextField") return CanvasInterface::ToolTypes::TextField;
    if (tag == "Toggle") return CanvasInterface::ToolTypes::Toggle;
    if (tag == "Frame") return CanvasInterface::ToolTypes::Frame;
    return std::nullopt;
}

vector<EditorWidget> Parser::ParseVisualFile(ifstream& stream) {
    vector<EditorWidget> widgets;
    optional<EditorWidget> current;
    string line;
    string fileVersion;
    bool inWidgetBlock = false;
    bool hasVersion = false;
    bool hasRootTag = false;
    int num = 0;
    vector<string> tagStack;
    while (getline(stream, line)) {
        num++;
        line = trim(line);

        if (line == "[MotifDesignerVisualFile]") {
            hasRootTag = true;
            continue;
        }

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
            continue;
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

                if (inWidgetBlock && current.has_value()) {
                    auto type = ParseToolType(closingType);
                    if (!type.has_value() || current->type != type.value()) {
                        Logger::log((std::string("[PARSER] [ERR] Syntax error detected at line ")
                            + to_string(num) + ": Mismatched closing tag [End" + closingType + "]").c_str());
                        OnParserFinished(SYNTAX_ERROR, (char*)(std::string("Syntax error detected at line ")
                            + to_string(num) + ": Mismatched closing tag [End" + closingType + "] (expected [End" + expectedType + "])").c_str());
                        break;
                    }
                    if (current->name.empty()) {
                        Logger::log((std::string("[PARSER] [ERR] Syntax error detected at line ")
                            + to_string(num) + ": Widget without a name").c_str());
                        OnParserFinished(SYNTAX_ERROR, (char*)(std::string("Syntax error detected at line ")
                            + to_string(num) + ": Widget without a name").c_str());
                        break;
                    }
                    widgets.push_back(current.value());
                    current.reset();
                    inWidgetBlock = false;
                }
                tagStack.pop_back();
                continue;
            } else {
                // opening tag
                if (tag == "MotifDesignerVisualFile" || tag == "Widgets") {
                    tagStack.push_back(tag);
                    continue;
                }

                auto type = ParseToolType(tag);
                if (type.has_value()) {
                    if (inWidgetBlock) {
                        Logger::log((std::string("[PARSER] [ERR] Syntax error detected at line ")
                            + to_string(num) + ": Nested widget blocks are not allowed").c_str());
                        OnParserFinished(SYNTAX_ERROR, (char*)(std::string("Syntax error detected at line ")
                            + to_string(num) + ": Nested widget blocks are not allowed").c_str());
                        break;
                    }

                    current = EditorWidget(type.value(), "", "", 0, 0, 0, 0);
                    inWidgetBlock = true;
                    tagStack.push_back(tag);
                    continue; // NOT break
                }

                Logger::log((std::string("[PARSER] [ERR] Syntax error detected at line ")
                    + to_string(num) + ": Unexpected tag [" + tag + "]").c_str());
                OnParserFinished(SYNTAX_ERROR, (char*)(std::string("Syntax error detected at line ")
                    + to_string(num) + ": Unexpected tag [" + tag + "]").c_str());
                break;
            }
        }
        size_t delimPos = line.find('=');
        if (delimPos != string::npos) {
            if (!inWidgetBlock) {
                Logger::log((std::string("[PARSER] [ERR] Syntax error detected at line ")
                        + to_string(num) + ": Unexpected assignment operator").c_str());
                    OnParserFinished(SYNTAX_ERROR, (char*)(std::string("Syntax error detected at line ")
                        + to_string(num) + ": Unexpected assignment operator").c_str());
                    break;
            }
            string key = trim(line.substr(0, delimPos));
            string value = trim(line.substr(delimPos + 1));
            if (key == "name") current->name = value;
            else if (key == "value") current->value = value;
            else if (key == "x") current->x = stoi(value);
            else if (key == "y") current->y = stoi(value);
            else if (key == "width") current->width = stoi(value);
            else if (key == "height") current->height = stoi(value);
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
    if (!hasRootTag) {
        Logger::log("[PARSER] [ERR] Missing root tag [MotifDesignerVisualFile]");
        OnParserFinished(SYNTAX_ERROR,
            (char*)"Missing root tag [MotifDesignerVisualFile]");
    }
    return widgets;
}