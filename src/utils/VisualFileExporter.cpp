#include "CanvasInterface.h"
#include "Components.h"
#include "Logger.h"
#include "ProjectManager.h"
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

void ProjectManager::ExportVisualFile(char* pathToSave) {
    vector<CanvasInterface::EditorWidgetInstance> widgets = g_canvas->GetWidgetList();
    stringstream stream;
    stream  << "&& MotifDesigner Visual File \n"
            << "[MotifDesignerVisualFile]\n"
            << "Version=" PROJECT_VERSION_STR << "\n"
            << "[Widgets]\n";
    for (auto& widget : widgets) {
        switch (widget.type) {
            case CanvasInterface::ToolTypes::Button:
                stream << "[Button]\n";
                break;
            case CanvasInterface::ToolTypes::Toggle:
                stream << "[Toggle]\n";
                break;
            case CanvasInterface::ToolTypes::Label:
                stream << "[Label]\n";
                break;
            case CanvasInterface::ToolTypes::Frame:
                stream << "[Frame]\n";
                break;
            case CanvasInterface::ToolTypes::TextField:
                stream << "[TextField]\n";
                break;
            default:
                stream << "[NIL]\n";
                break;
        }
        stream  << "x=" << widget.x << "\n"
                << "y=" << widget.y << "\n"
                << "width=" << widget.width << "\n"
                << "height=" << widget.height << "\n"
                << "name=" << widget.name << "\n"
                << "value=" << widget.value << "\n";
        switch (widget.type) {
            case CanvasInterface::ToolTypes::Button:
                stream << "[EndButton]\n";
                break;
            case CanvasInterface::ToolTypes::Toggle:
                stream << "[EndToggle]\n";
                break;
            case CanvasInterface::ToolTypes::Label:
                stream << "[EndLabel]\n";
                break;
            case CanvasInterface::ToolTypes::Frame:
                stream << "[EndFrame]\n";
                break;
            case CanvasInterface::ToolTypes::TextField:
                stream << "[EndTextField]\n";
                break;
            default:
                stream << "[EndNIL]\n";
                break;
        }
    }
    stream  << "[EndWidgets]" << endl;
    stream  << "[EndMotifDesignerVisualFile]" << endl;
    ofstream visualFile(pathToSave, ios::out | ios::binary);
    if (!visualFile.is_open()) {
        Logger::log("[FAIL] opening file failed");
        StatusBar::UpdateStatusBar("Failed to save visual file!");
        return;
    }
    visualFile << stream.str();
    visualFile.close();
    Logger::log("[SUCCESS] saving visual file");
    StatusBar::UpdateStatusBar((std::string("Visual file saved in: ") + pathToSave).c_str());
}