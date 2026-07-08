#ifndef CANVASINTERFACE_H
#define CANVASINTERFACE_H
#include <X11/Intrinsic.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <string>
#include <vector>
class CanvasInterface {
public:
    Widget canvas;
    enum ToolTypes {
        Select,
        Button,
        Label,
        TextField,
        Toggle,
        Frame
    };
    struct EditorWidgetInstance {
        ToolTypes type;
        std::string name;
        std::string value;
        int x;
        int y;
        int width;
        int height;
        bool selected;

        EditorWidgetInstance(
            ToolTypes t,
            const std::string& n,
            const std::string& l,
            int px,
            int py,
            int w,
            int h
        ) : type(t), name(n), value(l), x(px), y(py), width(w), height(h), selected(false) {}
    };
    static void InitializeGraphicContexts(Widget canvas);
    void ImportVectorOfWidgets(const std::vector<EditorWidgetInstance>& widgets);
    void DrawGrid(Display* display, Window win, int width, int height);
    void DrawBevel(Display* display, Window win, int x, int y, int width, int height, bool sunken);
    void DrawWidgetElement(Display* display, Window win, const EditorWidgetInstance& widget);
    void RefreshCanvas();
    void SelectWidget(int index);
    void SetPropertyPanel();
    void ClearPropertyPanel();
    // start region: double click to edit value feature
    struct SelectedTextRegion {
        int startCurIndex;
        int endCurIndex;
    };
    int ReturnWidgetValueLength();
    void EnterDoubleClickValueEdit(bool shouldEnter);
    bool IsEnteringDoubleClickValueEdit();
    int GetCurrentCursorPositionIndex();
    void ShiftCurrentCursorIndexPos(int amount);
    void TeleportCursorToIndex(int index);
    void SetSelectedTextRange(int start, int end);
    SelectedTextRegion GetSelectedTextRegion();
    // Claude was used because handling text is utterly complicated
    void SelectAllText();
    void ClearTextSelection();
    void ExtendSelectionByKeystroke(int amount);
    bool HasActiveTextSelection();
    void DeleteSelectedText();
    void InsertTextAtCursor(const std::string& text);
    void CopySelectionToClipboard();
    void CutSelectionToClipboard();
    void PasteFromClipboard();
    // end region
    static void SetTool(ToolTypes tool);
    static void HandleCanvasMouseDown(int mouseX, int mouseY, Time eventTime);
    static void HandleCanvasMouseMove(int mouseX, int mouseY);
    static void HandleCanvasMouseUp();
    void ShiftComponentPositionByKeystroke(int directionX, int directionY);
    void ScheduleRedraw();
    void ApplyPropertyPanelChanges();
    void DeleteSelectedWidget();
    std::vector<EditorWidgetInstance> GetWidgetList();
    static void ChangeActiveTool(ToolTypes tool);
    void SetSnapToGrid(bool enable);
    bool GetSnapToGridStatus();
};

// Global canvas instance accessible throughout the app
extern CanvasInterface* g_canvas;

#endif
