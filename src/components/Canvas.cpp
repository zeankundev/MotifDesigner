#include "CanvasInterface.h"
#include "Components.h"
#include "Logger.h"
#include <X11/Composite.h>
#include <X11/ICE/ICElib.h>
#include <X11/Intrinsic.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <Xm/DrawingA.h>
#include <Xm/Frame.h>
#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

enum class ResizeHandle {
    Nothing,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

ResizeHandle activeHandle = ResizeHandle::Nothing;

// Stores that are canvas only
std::vector<CanvasInterface::EditorWidgetInstance> widgets;
CanvasInterface::ToolTypes activeTool = CanvasInterface::ToolTypes::Select;
int selectedIndex = -1;
bool isDragging = false;
bool isResizing = false;
int dragStartX = 0;
int dragStartY = 0;
int widget_originalX = 0;
int widget_originalY = 0;
int widget_originalWidth = 0;
int widget_originalHeight = 0;
bool snapToGrid = true;
const int gridSize = 10;
const int resizeHandleSize = 10;

CanvasInterface* g_canvas = nullptr;

GC backgroundContext;
GC gridContext;
GC widgetBackgroundContext;
GC shadowDarkContext;
GC shadowLightContext;
GC textContext;
GC labelContext;
GC selectBorderGraphicsContext;
GC textHighlightContext;

GC handleOuterContext;
GC handleInnerContext;

bool graphicsContextsInitialized = false;
bool isDeletingWidget = false;
bool requiresRedraw = false;
bool isEnteringVisualStyleTextInput = false;

static XtIntervalId pendingRedrawTimer = 0;
const unsigned int REDRAW_DEBOUNCE_MS = 16;

Time lastClickTime = 0;
int lastClickX = 0;
int lastClickY = 0;
bool lastClickValid = false;

int currentCursorPositionIndex = 0;
CanvasInterface::SelectedTextRegion selectedTextRegion;
int selectionAnchorIndex = -1;
std::string textEditClipboard;
const int textCharWidth = 6;

bool IsDoubleClick(int x, int y, Time currentTime) {
    static constexpr Time DOUBLE_CLICK_MS = 1500;
    bool isDouble = false;
    if (lastClickValid) {
        Time delta = currentTime - lastClickTime;
        if (delta <= DOUBLE_CLICK_MS) {
            const int maxDistance = 6;
            if (std::abs(x - lastClickX) <= maxDistance && std::abs(y - lastClickY) <= maxDistance) {
                isDouble = true;
                Logger::log((std::string("Double click detected at (") + std::to_string(x) + ", " + std::to_string(y) + ")").c_str());
            }
        }
    }
    lastClickTime = currentTime;
    lastClickX = x;
    lastClickY = y;
    lastClickValid = true;
    return isDouble;
}

void CanvasInterface::InitializeGraphicContexts(Widget canvas) {
    if (graphicsContextsInitialized) {
        return;  // Already initialized
    }

    Display* display = XtDisplay(canvas);
    Window win = XtWindow(canvas);

    // Only proceed if window has been realized
    if (win == 0) {
        return;  // Widget not yet realized, will retry later
    }

    XGCValues values;

    Colormap cmap = DefaultColormap(display, DefaultScreen(display));
    XColor color, exact;

    XAllocNamedColor(display, cmap, "#d3d3d3", &color, &exact);
    values.foreground = color.pixel;
    backgroundContext = XCreateGC(display, win, GCForeground, &values);

    XAllocNamedColor(display, cmap, "#666666", &color, &exact);
    values.foreground = color.pixel;
    gridContext = XCreateGC(display, win, GCForeground, &values);

    XAllocNamedColor(display, cmap, "#a4a4a4", &color, &exact);
    values.foreground = color.pixel;
    widgetBackgroundContext = XCreateGC(display, win, GCForeground, &values);

    XAllocNamedColor(display, cmap, "#404040", &color, &exact);
    values.foreground = color.pixel;
    shadowDarkContext = XCreateGC(display, win, GCForeground, &values);

    XAllocNamedColor(display, cmap, "#e0e0e0", &color, &exact);
    values.foreground = color.pixel;
    shadowLightContext = XCreateGC(display, win, GCForeground, &values);

    XAllocNamedColor(display, cmap, "#000000", &color, &exact);
    values.foreground = color.pixel;
    textContext = XCreateGC(display, win, GCForeground, &values);

    XAllocNamedColor(display, cmap, "#000000", &color, &exact);
    values.foreground = color.pixel;
    // Background should be the same as backgroundContext which is #d3d3d3. Just use hex
    values.background = 0xd3d3d3;
    labelContext = XCreateGC(display, win, GCForeground | GCBackground, &values);

    XAllocNamedColor(display, cmap, "#00A0FF", &color, &exact);
    values.foreground = color.pixel;
    selectBorderGraphicsContext = XCreateGC(display, win, GCForeground, &values);

    XAllocNamedColor(display, cmap, "#00A0FF", &color, &exact);
    values.foreground = color.pixel;
    handleOuterContext = XCreateGC(display, win, GCForeground, &values);

    XAllocNamedColor(display, cmap, "#FFFFFF", &color, &exact);
    values.foreground = color.pixel;
    handleInnerContext = XCreateGC(display, win, GCForeground, &values);

    XAllocNamedColor(display, cmap, "#99CCFF", &color, &exact);
    values.foreground = color.pixel;
    textHighlightContext = XCreateGC(display, win, GCForeground, &values);

    graphicsContextsInitialized = true;
}

// Helpers for double click to edit text

void CanvasInterface::EnterDoubleClickValueEdit(bool shouldEnter) {
    isEnteringVisualStyleTextInput = shouldEnter;
}
bool CanvasInterface::IsEnteringDoubleClickValueEdit() {
    return isEnteringVisualStyleTextInput;
}

int CanvasInterface::GetCurrentCursorPositionIndex() {
    return currentCursorPositionIndex;
}

void CanvasInterface::ShiftCurrentCursorIndexPos(int amount) {
int maxLen = (selectedIndex >= 0 && selectedIndex < (int)widgets.size())
        ? (int)widgets[selectedIndex].value.length() : 0;
    int newPos = currentCursorPositionIndex + amount;
    if (newPos < 0) newPos = 0;
    if (newPos > maxLen) newPos = maxLen;
    currentCursorPositionIndex = newPos;
}

void CanvasInterface::ClearTextSelection() {
    selectionAnchorIndex = -1;
    selectedTextRegion.startCurIndex = currentCursorPositionIndex;
    selectedTextRegion.endCurIndex = currentCursorPositionIndex;
}

bool CanvasInterface::HasActiveTextSelection() {
    return selectedTextRegion.startCurIndex != selectedTextRegion.endCurIndex;
}

void CanvasInterface::SelectAllText() {
    if (selectedIndex < 0 || selectedIndex >= (int)widgets.size()) return;
    int len = (int)widgets[selectedIndex].value.length();
    selectionAnchorIndex = 0;
    selectedTextRegion.startCurIndex = 0;
    selectedTextRegion.endCurIndex = len;
    currentCursorPositionIndex = len;
}

void CanvasInterface::ExtendSelectionByKeystroke(int amount) {
    if (selectedIndex < 0 || selectedIndex >= (int)widgets.size()) return;
    if (selectionAnchorIndex == -1) {
        selectionAnchorIndex = currentCursorPositionIndex;
    }
    int len = (int)widgets[selectedIndex].value.length();
    int newPos = currentCursorPositionIndex + amount;
    if (newPos < 0) newPos = 0;
    if (newPos > len) newPos = len;
    currentCursorPositionIndex = newPos;

    selectedTextRegion.startCurIndex = std::min(selectionAnchorIndex, currentCursorPositionIndex);
    selectedTextRegion.endCurIndex = std::max(selectionAnchorIndex, currentCursorPositionIndex);
}

void CanvasInterface::DeleteSelectedText() {
    if (selectedIndex < 0 || selectedIndex >= (int)widgets.size()) return;
    auto& w = widgets[selectedIndex];
    if (HasActiveTextSelection()) {
        int start = selectedTextRegion.startCurIndex;
        int end = selectedTextRegion.endCurIndex;
        w.value.erase(start, end - start);
        currentCursorPositionIndex = start;
        ClearTextSelection();
    } else if (currentCursorPositionIndex > 0) {
        w.value.erase(currentCursorPositionIndex - 1, 1);
        currentCursorPositionIndex--;
    }
    ScheduleRedraw();
}

void CanvasInterface::InsertTextAtCursor(const std::string& text) {
    if (selectedIndex < 0 || selectedIndex >= (int)widgets.size() || text.empty()) return;
    auto& w = widgets[selectedIndex];
    if (HasActiveTextSelection()) {
        int start = selectedTextRegion.startCurIndex;
        int end = selectedTextRegion.endCurIndex;
        w.value.erase(start, end - start);
        currentCursorPositionIndex = start;
        ClearTextSelection();
    }
    w.value.insert(currentCursorPositionIndex, text);
    currentCursorPositionIndex += (int)text.length();
    ScheduleRedraw();
}

void CanvasInterface::CopySelectionToClipboard() {
    if (selectedIndex < 0 || selectedIndex >= (int)widgets.size() || !HasActiveTextSelection()) return;
    const auto& w = widgets[selectedIndex];
    int start = selectedTextRegion.startCurIndex;
    int end = selectedTextRegion.endCurIndex;
    textEditClipboard = w.value.substr(start, end - start);
}

void CanvasInterface::CutSelectionToClipboard() {
    CopySelectionToClipboard();
    DeleteSelectedText();
}

void CanvasInterface::PasteFromClipboard() {
    if (textEditClipboard.empty()) return;
    InsertTextAtCursor(textEditClipboard);
}

void CanvasInterface::TeleportCursorToIndex(int index) {
    currentCursorPositionIndex = index;
}

int CanvasInterface::ReturnWidgetValueLength() {
    if (selectedIndex == -1) return 0;
    return widgets[selectedIndex].value.length();
}

void CanvasInterface::SetSelectedTextRange(int start, int end) {
    selectedTextRegion.startCurIndex = start;
    selectedTextRegion.endCurIndex = end;
}

CanvasInterface::SelectedTextRegion CanvasInterface::GetSelectedTextRegion() {
    return selectedTextRegion;
}

void CanvasInterface::DrawGrid(Display* display, Window win, int width, int height) {
    if (!display || !win || !backgroundContext || !gridContext) return;
    Logger::log("Drawing the grid");
    XFillRectangle(display, win, backgroundContext, 0, 0, width, height);
    for (int x = gridSize; x < width; x+= gridSize) {
        for (int y = gridSize; y < height; y+= gridSize) {
            XDrawPoint(display, win, gridContext, x, y);
        }
    }
}

void CanvasInterface::DrawBevel(Display* display, Window win, int x, int y, int width, int height, bool sunken) {
    GC topContext = sunken ? shadowDarkContext : shadowLightContext;
    GC bottomContext = sunken ? shadowLightContext : shadowDarkContext;

    XDrawLine(display, win, topContext, x, y, x+width, y);
    XDrawLine(display, win, topContext, x, y, x, y+height);
    XDrawLine(display, win, bottomContext, x+width, y+height, x+width, y);
    XDrawLine(display, win, bottomContext, x+width, y+height, x, y+height);
}

static void FixedRedrawCb(XtPointer clientData, XtIntervalId* id) {
    if (g_canvas) {
        g_canvas->RefreshCanvas();
    }
    pendingRedrawTimer = 0;
}

void CanvasInterface::ScheduleRedraw() {
    if (pendingRedrawTimer != 0) {
        return;
    }

    if (this->canvas && XtIsRealized(this->canvas)) {
        pendingRedrawTimer = XtAppAddTimeOut(
            XtWidgetToApplicationContext(this->canvas),
            REDRAW_DEBOUNCE_MS,
            FixedRedrawCb,
            nullptr
        );
    }
}

void CanvasInterface::ImportVectorOfWidgets(const std::vector<EditorWidgetInstance>& widgetsFromMethod) {
    widgets.clear();
    widgets = widgetsFromMethod;
    ScheduleRedraw();
}

void CanvasInterface::DrawWidgetElement(Display* display, Window win, const EditorWidgetInstance& widget) {
    if (!display || !win || !widgetBackgroundContext || !textContext) return;
    bool isBeingEdited = widget.selected && isEnteringVisualStyleTextInput;

    switch (widget.type) {
        case ToolTypes::Button: {
            XFillRectangle(display, win, widgetBackgroundContext, widget.x, widget.y, widget.width, widget.height);
            CanvasInterface::DrawBevel(display, win, widget.x, widget.y, widget.width, widget.height, false);
            int textY = widget.y + (widget.height / 2) + 4;
            int textX = widget.x + (widget.width - (widget.value.length() * 6)) / 2;
            if (textX < widget.x + 4) textX = widget.x + 4;
            XDrawString(display, win, textContext, textX, textY, widget.value.c_str(), widget.value.length());
            break;
        }
        case ToolTypes::Label: {
            XFillRectangle(display, win, backgroundContext, widget.x, widget.y, widget.width, widget.height);
            int textY = widget.y + (widget.height / 2) + 4;
            int textX = widget.x + 4;
            if (isBeingEdited && selectedTextRegion.startCurIndex != selectedTextRegion.endCurIndex) {
                int selStart = std::min(selectedTextRegion.startCurIndex, selectedTextRegion.endCurIndex);
                int selEnd = std::max(selectedTextRegion.startCurIndex, selectedTextRegion.endCurIndex);
                int highlightX = textX + selStart * textCharWidth;
                int highlightW = (selEnd - selStart) * textCharWidth;
                XFillRectangle(display, win, textHighlightContext, highlightX, widget.y + 4, highlightW, widget.height - 8);
            }
            XDrawString(display, win, labelContext, textX, textY, widget.value.c_str(), widget.value.length());
            if (isBeingEdited) {
                int cursorX = textX + currentCursorPositionIndex * textCharWidth;
                // add blinking cursor
                if (time(NULL) % 2 == 0) {
                    XDrawLine(display, win, labelContext, cursorX, textY-8, cursorX, textY+4);
                }
            }
            break;
        }
        case ToolTypes::TextField: {
            XFillRectangle(display, win, widgetBackgroundContext, widget.x, widget.y, widget.width, widget.height);
            CanvasInterface::DrawBevel(display, win, widget.x, widget.y, widget.width, widget.height, true);
            int textY = widget.y + (widget.height / 2) + 4;
            int textBaseX = widget.x + 6;

            // Draw the selection highlight behind the text, if this field is being edited
            if (isBeingEdited && selectedTextRegion.startCurIndex != selectedTextRegion.endCurIndex) {
                int selStart = std::min(selectedTextRegion.startCurIndex, selectedTextRegion.endCurIndex);
                int selEnd = std::max(selectedTextRegion.startCurIndex, selectedTextRegion.endCurIndex);
                int highlightX = textBaseX + selStart * textCharWidth;
                int highlightW = (selEnd - selStart) * textCharWidth;
                XFillRectangle(display, win, textHighlightContext, highlightX, widget.y + 4, highlightW, widget.height - 8);
            }

            XDrawString(display, win, textContext, textBaseX, textY, widget.value.c_str(), widget.value.length());

            // Draw the text cursor: at the live edit position while editing, otherwise trailing the text
            int cursorX = isBeingEdited
                ? textBaseX + currentCursorPositionIndex * textCharWidth
                : widget.x + 8 + (int)(widget.value.length() * textCharWidth);
            if (cursorX < widget.x + widget.width - 4) {
                XDrawLine(display, win, textContext, cursorX, widget.y + 4, cursorX, widget.y + widget.height - 4);
            }
            break;
        }
        case ToolTypes::Toggle: {
            XFillRectangle(display, win, backgroundContext, widget.x, widget.y, widget.width, widget.height);
            int boxSize = 12;
            int boxY = widget.y + (widget.height - boxSize) / 2;
            CanvasInterface::DrawBevel(display, win, widget.x + 4, boxY, boxSize, boxSize, true);

            XDrawLine(display, win, textContext, widget.x + 6 , boxY + 4, widget.x + 9, boxY + 8);
            XDrawLine(display, win, textContext, widget.x + 9 , boxY + 8, widget.x + 13, boxY + 2);

            int textY = widget.y + (widget.height / 2) + 4;
            XDrawString(display, win, textContext, widget.x + 22, textY, widget.value.c_str(), widget.value.length());
            break;
        }
        case ToolTypes::Frame: {
            XFillRectangle(display, win, widgetBackgroundContext, widget.x, widget.y, widget.width, widget.height);
            CanvasInterface::DrawBevel(display, win, widget.x, widget.y, widget.width, widget.height, true);
            XDrawString(display, win, textContext, widget.x + 8, widget.y + 14, widget.value.c_str(), widget.value.length());
            break;
        }
        default:
            break;
    }
    if (widget.selected) {
        XDrawRectangle(display, win, selectBorderGraphicsContext, widget.x, widget.y, widget.width, widget.height);
        // Bottom right
        XFillRectangle(display, win, handleOuterContext, widget.x + widget.width - 4, widget.y + widget.height - 4, resizeHandleSize, resizeHandleSize);
        XFillRectangle(display, win, handleInnerContext, widget.x + widget.width - 3, widget.y + widget.height - 3, resizeHandleSize - 2, resizeHandleSize - 2);
        // Top right
        XFillRectangle(display, win, handleOuterContext, widget.x + widget.width - 4, widget.y - 4, resizeHandleSize, resizeHandleSize);
        XFillRectangle(display, win, handleInnerContext, widget.x + widget.width - 3, widget.y - 3, resizeHandleSize - 2, resizeHandleSize - 2);
        // Bottom left
        XFillRectangle(display, win, handleOuterContext, widget.x - 4, widget.y + widget.height - 4, resizeHandleSize, resizeHandleSize);
        XFillRectangle(display, win, handleInnerContext, widget.x - 3, widget.y + widget.height - 3, resizeHandleSize - 2, resizeHandleSize - 2);
        // Top left
        XFillRectangle(display, win, handleOuterContext, widget.x - 4, widget.y - 4, resizeHandleSize, resizeHandleSize);
        XFillRectangle(display, win, handleInnerContext, widget.x - 3, widget.y - 3, resizeHandleSize - 2, resizeHandleSize - 2);

        // Size label badge: centered horizontally below the widget
        {
            std::string sizeLabel = std::to_string(widget.width) + "x" + std::to_string(widget.height);
            int charWidth  = 6;  // Approximate fixed-font glyph width
            int charHeight = 10; // Approximate fixed-font glyph height
            int textW = (int)sizeLabel.length() * charWidth;
            int padX  = 8;
            int padY  = 4;
            int r     = 6;  // Corner radius
            int badgeW = textW + padX * 2;
            int badgeH = charHeight + padY * 2;
            int badgeX = widget.x + (widget.width - badgeW) / 2;
            int badgeY = widget.y + widget.height + 8;

            // Fill the rounded rect using three overlapping filled areas + 4 arcs
            // Center horizontal strip
            XFillRectangle(display, win, handleOuterContext, badgeX + r, badgeY, badgeW - r * 2, badgeH);
            // Left vertical strip
            XFillRectangle(display, win, handleOuterContext, badgeX, badgeY + r, r, badgeH - r * 2);
            // Right vertical strip
            XFillRectangle(display, win, handleOuterContext, badgeX + badgeW - r, badgeY + r, r, badgeH - r * 2);
            // Top-left arc
            XFillArc(display, win, handleOuterContext, badgeX, badgeY, r * 2, r * 2, 90 * 64, 90 * 64);
            // Top-right arc
            XFillArc(display, win, handleOuterContext, badgeX + badgeW - r * 2, badgeY, r * 2, r * 2, 0, 90 * 64);
            // Bottom-left arc
            XFillArc(display, win, handleOuterContext, badgeX, badgeY + badgeH - r * 2, r * 2, r * 2, 180 * 64, 90 * 64);
            // Bottom-right arc
            XFillArc(display, win, handleOuterContext, badgeX + badgeW - r * 2, badgeY + badgeH - r * 2, r * 2, r * 2, 270 * 64, 90 * 64);

            // Draw the label text centered in the badge
            int textX = badgeX + padX;
            int textY = badgeY + padY + charHeight;
            XDrawString(display, win, handleInnerContext, textX, textY, sizeLabel.c_str(), sizeLabel.length());
        }
    }
    if (isEnteringVisualStyleTextInput) {
        // Draw the highlight first
        // XFillRectangle(display, win, handleOuterContext, );
    }
}

void CanvasInterface::RefreshCanvas() {
    if (!this->canvas || !XtIsRealized(this->canvas)) return;

    // Ensure graphics contexts are initialized
    CanvasInterface::InitializeGraphicContexts(this->canvas);
    if (!graphicsContextsInitialized) return;

    Display* display = XtDisplay(this->canvas);
    Window win = XtWindow(this->canvas);

    Logger::log("Refreshing the canvas by now!");

    Dimension w, h;
    Arg args[2];
    int n = 0;
    XtSetArg(args[n], XmNwidth, &w); n++;
    XtSetArg(args[n], XmNheight, &h); n++;
    XtGetValues(this->canvas, args, 2);

    CanvasInterface::DrawGrid(display, win, w, h);
    for (const auto& widget : widgets) {
        Logger::log("Drawing widgets!");
        CanvasInterface::DrawWidgetElement(display, win, widget);
    }
}

void CanvasInterface::SetPropertyPanel() {
    Logger::log("Setting property values");
    if (selectedIndex < 0 || selectedIndex >= (int)widgets.size()) return;
    PropertiesPanel::instanceName.UpdateFieldValue(widgets[selectedIndex].name.c_str());
    PropertiesPanel::valueContent.UpdateFieldValue(widgets[selectedIndex].value.c_str());
    
    std::string xPosStr = std::to_string(widgets[selectedIndex].x);
    std::string yPosStr = std::to_string(widgets[selectedIndex].y);
    std::string widthStr = std::to_string(widgets[selectedIndex].width);
    std::string heightStr = std::to_string(widgets[selectedIndex].height);
    
    PropertiesPanel::xPos.UpdateFieldValue(xPosStr.c_str());
    PropertiesPanel::yPos.UpdateFieldValue(yPosStr.c_str());
    PropertiesPanel::width.UpdateFieldValue(widthStr.c_str());
    PropertiesPanel::height.UpdateFieldValue(heightStr.c_str());
}

void CanvasInterface::ClearPropertyPanel() {
    Logger::log("Clearing property values");
    PropertiesPanel::instanceName.UpdateFieldValue("");
    PropertiesPanel::valueContent.UpdateFieldValue("");
    PropertiesPanel::xPos.UpdateFieldValue("");
    PropertiesPanel::yPos.UpdateFieldValue("");
    PropertiesPanel::width.UpdateFieldValue("");
    PropertiesPanel::height.UpdateFieldValue("");
}

void CanvasInterface::SelectWidget(int index) {
    Logger::log("Selecting widget");
    if (selectedIndex >= 0 && selectedIndex < (int)widgets.size()) {
        widgets[selectedIndex].selected = false;
    }

    selectedIndex = index;

    if (selectedIndex >= 0 && selectedIndex < (int)widgets.size()) {
        widgets[selectedIndex].selected = true;
        StatusBar::UpdateStatusBar("Drag either handles to modify the size, or tackle at the properties panel!");
        SetPropertyPanel();
    } else {
        ClearPropertyPanel();
        StatusBar::UpdateStatusBar("Select a component to modify its properties");
    }
    RefreshCanvas();
}

void CanvasInterface::ShiftComponentPositionByKeystroke(int directionX, int directionY) {
    if (selectedIndex < 0 || selectedIndex >= (int)widgets.size()) return;
    auto& w = widgets[selectedIndex];
    // Clamp the movement to x = 0, y= 0
    int newX = w.x + directionX;
    int newY = w.y + directionY;
    if (newX < 0) newX = 0;
    if (newY < 0) newY = 0;
    w.x = newX;
    w.y = newY;
    SetPropertyPanel();
    RefreshCanvas();
}

void CanvasInterface::SetTool(ToolTypes tool) {
    activeTool = tool;
    if (tool == ToolTypes::Select && (selectedIndex < 0 || selectedIndex >= (int)widgets.size())) {
        StatusBar::UpdateStatusBar("Select a component to modify its properties");
    } else if (tool == ToolTypes::Select && selectedIndex >= 0 && selectedIndex < (int)widgets.size()) {
        StatusBar::UpdateStatusBar("Drag either handles to modify the size, or tackle at the properties panel!"); 
    } else {
        StatusBar::UpdateStatusBar("Click anywhere on the canvas to create a widget");
    }
}

void CanvasInterface::SetSnapToGrid(bool enable) {
    snapToGrid = enable;
}

bool CanvasInterface::GetSnapToGridStatus() {
    return snapToGrid;
}

void CanvasInterface::HandleCanvasMouseDown(int mouseX, int mouseY, Time eventTime) {
    const bool isDoubleClickDetected = IsDoubleClick(mouseX, mouseY, eventTime);

    if (isDoubleClickDetected) {
        Logger::log("Double click detected - entering text edit mode");
        if (selectedIndex >= 0 && selectedIndex < (int)widgets.size() &&
            (widgets[selectedIndex].type == ToolTypes::TextField || 
             widgets[selectedIndex].type == ToolTypes::Label ||
             widgets[selectedIndex].type == ToolTypes::Button ||
             widgets[selectedIndex].type == ToolTypes::Toggle ||
             widgets[selectedIndex].type == ToolTypes::Frame)) {
            const auto& w = widgets[selectedIndex];
            int relativeX = mouseX - (w.x + 6);
            int clickIndex = relativeX / textCharWidth;
            if (clickIndex < 0) clickIndex = 0;
            if (clickIndex > (int)w.value.length()) clickIndex = (int)w.value.length();

            // Expand outward from the click position to the surrounding word's boundaries
            int wordStart = clickIndex;
            int wordEnd = clickIndex;
            while (wordStart > 0 && !std::isspace((unsigned char)w.value[wordStart - 1])) wordStart--;
            while (wordEnd < (int)w.value.length() && !std::isspace((unsigned char)w.value[wordEnd])) wordEnd++;

            // Clicked on whitespace / empty value: fall back to selecting everything
            if (wordStart == wordEnd) {
                wordStart = 0;
                wordEnd = (int)w.value.length();
            }

            g_canvas->SetSelectedTextRange(wordStart, wordEnd);
            g_canvas->TeleportCursorToIndex(wordEnd);
            selectionAnchorIndex = wordStart;
            isEnteringVisualStyleTextInput = true;
            g_canvas->ScheduleRedraw();
        }
        return;
    }

    if (g_canvas == nullptr) return;
    if (activeTool == ToolTypes::Select) {
        for (int i = widgets.size() - 1; i >= 0; --i) {
            
            const auto& w = widgets[i];
            if (w.selected) {
                
                // Define the X and Y coordinates for the edges
                int leftX   = w.x - 4;
                int rightX  = w.x + w.width - 4;
                int topY    = w.y - 4;
                int bottomY = w.y + w.height - 4;

                ResizeHandle hitHandle = ResizeHandle::Nothing;

                // 1. Top-Left
                if (mouseX >= leftX && mouseX <= leftX + resizeHandleSize &&
                    mouseY >= topY  && mouseY <= topY + resizeHandleSize) {
                    hitHandle = ResizeHandle::TopLeft;
                }
                // 2. Top-Right
                else if (mouseX >= rightX && mouseX <= rightX + resizeHandleSize &&
                        mouseY >= topY   && mouseY <= topY + resizeHandleSize) {
                    hitHandle = ResizeHandle::TopRight;
                }
                // 3. Bottom-Left
                else if (mouseX >= leftX   && mouseX <= leftX + resizeHandleSize &&
                        mouseY >= bottomY && mouseY <= bottomY + resizeHandleSize) {
                    hitHandle = ResizeHandle::BottomLeft;
                }
                // 4. Bottom-Right
                else if (mouseX >= rightX   && mouseX <= rightX + resizeHandleSize &&
                        mouseY >= bottomY && mouseY <= bottomY + resizeHandleSize) {
                    hitHandle = ResizeHandle::BottomRight;
                }

                if (hitHandle != ResizeHandle::Nothing) {
                    isResizing = true;
                    activeHandle = hitHandle; // Store this to use during the mouseMove event
                    dragStartX = mouseX;
                    dragStartY = mouseY;
                    
                    // Critical: If resizing from top or left, you must also store original X/Y
                    widget_originalX = w.x;
                    widget_originalY = w.y;
                    widget_originalWidth = w.width;
                    widget_originalHeight = w.height; 
                    g_canvas->SelectWidget(i);
                    return;
                }
            }

            // --- Dragging logic remains the same ---
            if (mouseX >= w.x && mouseX <= w.x + w.width &&
                mouseY >= w.y && mouseY <= w.y + w.height) {
                isDragging = true;
                dragStartX = mouseX;
                dragStartY = mouseY;
                widget_originalX = w.x;
                widget_originalY = w.y;
                g_canvas->SelectWidget(i);
                return;
            }
        }
        g_canvas->SelectWidget(-1);
    } else {
        std::string defaultName, defaultValue;
        int defaultWidth = 120, defaultHeight = 35;

        int spawnX = snapToGrid ? (mouseX / gridSize) * gridSize : mouseX;
        int spawnY = snapToGrid ? (mouseY / gridSize) * gridSize : mouseY;

        switch (activeTool) {
            case ToolTypes::Button:
                defaultName = "PushButton";
                defaultValue = "Button";
                break;
            case ToolTypes::Label:
                defaultName = "Label";
                defaultValue = "Some text";
                defaultHeight = 20;
                break;
            case ToolTypes::TextField:
                defaultName = "TextField";
                defaultValue = "Text Field";
                defaultHeight = 30;
                break;
            case ToolTypes::Toggle:
                defaultName = "ToggleButton";
                defaultValue = "Toggle";
                defaultHeight = 20;
                break;
            case ToolTypes::Frame:
                defaultName = "Frame";
                defaultValue = "Frame";
                defaultWidth = 200;
                defaultHeight = 100;
                break;
            default:
                return;
        }
        std::stringstream prependedName;
        prependedName << "Widget_" << defaultName << (widgets.size() + 1);
        EditorWidgetInstance newWidget(activeTool, prependedName.str(), defaultValue, spawnX, spawnY, defaultWidth, defaultHeight);
        widgets.push_back(newWidget);
        g_canvas->SelectWidget(widgets.size() - 1);
        SetTool(ToolTypes::Select);
    }
}


void CanvasInterface::HandleCanvasMouseMove(int mouseX, int mouseY) {
    if (g_canvas == nullptr) return;
    if (selectedIndex < 0 || selectedIndex >= (int)widgets.size()) return;
    auto& w = widgets[selectedIndex];
    int dx = mouseX - dragStartX;
    int dy = mouseY - dragStartY;

    if (isDragging) {
        int newX = widget_originalX + dx;
        int newY = widget_originalY + dy;
        if (snapToGrid) {
            newX = (newX / gridSize) * gridSize;
            newY = (newY / gridSize) * gridSize;
        }
        if (newX < 0) newX = 0;
        if (newY < 0) newY = 0;
        if (w.x != newX || w.y != newY) {
            w.x = newX;
            w.y = newY;
            requiresRedraw = true;
        }
        g_canvas->SetPropertyPanel();
    } else if (isResizing) {
        int newWidth = widget_originalWidth;
        int newHeight = widget_originalHeight;
        int newX = widget_originalX;
        int newY = widget_originalY;

        // Handle each corner differently
        if (activeHandle == ResizeHandle::TopLeft) {
            newX = widget_originalX + dx;
            newY = widget_originalY + dy;
            newWidth = widget_originalWidth - dx;
            newHeight = widget_originalHeight - dy;
        }
        else if (activeHandle == ResizeHandle::TopRight) {
            newY = widget_originalY + dy;
            newWidth = widget_originalWidth + dx;
            newHeight = widget_originalHeight - dy;
        }
        else if (activeHandle == ResizeHandle::BottomLeft) {
            newX = widget_originalX + dx;
            newWidth = widget_originalWidth - dx;
            newHeight = widget_originalHeight + dy;
        }
        else if (activeHandle == ResizeHandle::BottomRight) {
            newWidth = widget_originalWidth + dx;
            newHeight = widget_originalHeight + dy;
        }

        // Apply grid snapping
        if (snapToGrid) {
            newX = (newX / gridSize) * gridSize;
            newY = (newY / gridSize) * gridSize;
            newWidth = (newWidth / gridSize) * gridSize;
            newHeight = (newHeight / gridSize) * gridSize;
        }

        // Apply minimum size constraints
        if (newWidth < 20) newWidth = 20;
        if (newHeight < 10) newHeight = 10;

        // Prevent negative position
        if (newX < 0) newX = 0;
        if (newY < 0) newY = 0;

        if (w.x != newX || w.y != newY || w.width != newWidth || w.height != newHeight) {
            w.x = newX;
            w.y = newY;
            w.width = newWidth;
            w.height = newHeight;
            requiresRedraw = true;
        }
    }
    if (requiresRedraw) {
        g_canvas->SetPropertyPanel();
        g_canvas->ScheduleRedraw();
        requiresRedraw = false;
    }
}

void CanvasInterface::HandleCanvasMouseUp() {
    isDragging = false;
    isResizing = false;
}

void CanvasInterface::ApplyPropertyPanelChanges() {
    Logger::log("Applying property changes");
    if (selectedIndex < 0 || selectedIndex >= (int)widgets.size()) return;

    auto& w = widgets[selectedIndex];

    std::string nameStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::instanceName);
    std::string valueStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::valueContent);
    std::string xStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::xPos);
    std::string yStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::yPos);
    std::string widthStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::width);
    std::string heightStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::height);

    if (!nameStr.empty()) { w.name = nameStr; }
    if (!valueStr.empty()) { w.value = valueStr; }
    if (!xStr.empty()) { w.x = atoi(xStr.c_str()); }
    if (!yStr.empty()) { w.y = atoi(yStr.c_str()); }
    if (!widthStr.empty()) { w.width = atoi(widthStr.c_str()); }
    if (!heightStr.empty()) { w.height = atoi(heightStr.c_str()); }
    RefreshCanvas();
}

void CanvasInterface::DeleteSelectedWidget() {
    if (isDeletingWidget) return;  // Prevent multiple rapid deletions
    isDeletingWidget = true;

    Logger::log("Deleting a component");
    if (selectedIndex >= 0 && selectedIndex < (int)widgets.size()) {
        int indexToDelete = selectedIndex;
        Logger::log("Selecting widget of -1");
        SelectWidget(-1);
        widgets.erase(widgets.begin() + indexToDelete);
        RefreshCanvas();
    }

    isDeletingWidget = false;
}
std::vector<CanvasInterface::EditorWidgetInstance> CanvasInterface::GetWidgetList() {
    return widgets;
}

// Now the real canvas stuffs
void ExposeCanvasCallback(Widget w, XtPointer clientData, XtPointer callData) {
    CanvasInterface* canvas = static_cast<CanvasInterface*>(clientData);
    // Ensure graphics contexts are initialized when widget is first exposed
    CanvasInterface::InitializeGraphicContexts(w);
    canvas->RefreshCanvas();
}
void CanvasInputCallback(Widget w, XtPointer clientData, XtPointer callData) {
    CanvasInterface* canvas = static_cast<CanvasInterface*>(clientData);
    XmDrawingAreaCallbackStruct* cbs = (XmDrawingAreaCallbackStruct*)callData;
    XEvent* event = cbs->event;
    if (event->type == ButtonPress) {
        int x = event->xbutton.x;
        int y = event->xbutton.y;
        Time t = event->xbutton.time;
        canvas->HandleCanvasMouseDown(x, y, t);
    } else if (event->type == MotionNotify) {
        int x = event->xbutton.x;
        int y = event->xbutton.y;
        canvas->HandleCanvasMouseMove(x, y);
    } else if (event->type == ButtonRelease) {
        canvas->HandleCanvasMouseUp();
    }
}

void CanvasKeyPressCallback(Widget w, XtPointer clientData, XEvent* event, Boolean* continueDispatch) {
    Logger::log("Key callback fired");
    if (event->type == KeyPress) {
        char lookupBuffer[32];
        KeySym sym;
        int typedLen = XLookupString(&event->xkey, lookupBuffer, sizeof(lookupBuffer) - 1, &sym, nullptr);
        lookupBuffer[typedLen] = '\0';

        bool shiftHeld = (event->xkey.state & ShiftMask) != 0;
        bool ctrlHeld = (event->xkey.state & ControlMask) != 0;
        bool editingText = g_canvas->IsEnteringDoubleClickValueEdit();

        // Clipboard shortcuts only apply while actively editing a text field's value
        if (editingText && ctrlHeld) {
            switch (sym) {
                case XK_a: case XK_A:
                    g_canvas->SelectAllText();
                    g_canvas->ScheduleRedraw();
                    return;
                case XK_c: case XK_C:
                    g_canvas->CopySelectionToClipboard();
                    return;
                case XK_x: case XK_X:
                    g_canvas->CutSelectionToClipboard();
                    return;
                case XK_v: case XK_V:
                    g_canvas->PasteFromClipboard();
                    return;
                default:
                    break;
            }
        }

        switch (sym) {
            case XK_Delete : {
                if (editingText) {
                    g_canvas->DeleteSelectedText();
                } else {
                    g_canvas->DeleteSelectedWidget();
                }
            } break;
            case XK_BackSpace : {
                if (editingText) {
                    g_canvas->DeleteSelectedText();
                } else {
                    g_canvas->DeleteSelectedWidget();
                }
            } break;
            case XK_Up : {
                if (!editingText) g_canvas->ShiftComponentPositionByKeystroke(0, -10);
            } break;
            case XK_Down : {
                if (!editingText) g_canvas->ShiftComponentPositionByKeystroke(0, 10);
            } break;
            case XK_Left : {
                if (editingText) {
                    if (shiftHeld) {
                        g_canvas->ExtendSelectionByKeystroke(-1);
                    } else {
                        CanvasInterface::SelectedTextRegion region = g_canvas->GetSelectedTextRegion();
                        if (region.startCurIndex != region.endCurIndex) {
                            g_canvas->TeleportCursorToIndex(std::min(region.startCurIndex, region.endCurIndex));
                        } else {
                            g_canvas->ShiftCurrentCursorIndexPos(-1);
                        }
                        g_canvas->ClearTextSelection();
                    }
                    g_canvas->ScheduleRedraw();
                } else {
                    g_canvas->ShiftComponentPositionByKeystroke(-10, 0);
                }
            } break;
            case XK_Right : {
                if (editingText) {
                    if (shiftHeld) {
                        g_canvas->ExtendSelectionByKeystroke(1);
                    } else {
                        CanvasInterface::SelectedTextRegion region = g_canvas->GetSelectedTextRegion();
                        if (region.startCurIndex != region.endCurIndex) {
                            g_canvas->TeleportCursorToIndex(std::max(region.startCurIndex, region.endCurIndex));
                        } else {
                            g_canvas->ShiftCurrentCursorIndexPos(1);
                        }
                        g_canvas->ClearTextSelection();
                    }
                    g_canvas->ScheduleRedraw();
                } else {
                    g_canvas->ShiftComponentPositionByKeystroke(10, 0);
                }
            } break;
            case XK_Home : {
                if (editingText) {
                    if (shiftHeld) {
                        g_canvas->ExtendSelectionByKeystroke(-g_canvas->GetCurrentCursorPositionIndex());
                    } else {
                        g_canvas->TeleportCursorToIndex(0);
                        g_canvas->ClearTextSelection();
                    }
                    g_canvas->ScheduleRedraw();
                }
            } break;
            case XK_End : {
                if (editingText) {
                    int len = g_canvas->ReturnWidgetValueLength();
                    if (shiftHeld) {
                        g_canvas->ExtendSelectionByKeystroke(len - g_canvas->GetCurrentCursorPositionIndex());
                    } else {
                        g_canvas->TeleportCursorToIndex(len);
                        g_canvas->ClearTextSelection();
                    }
                    g_canvas->ScheduleRedraw();
                }
            } break;
            case XK_Return : case XK_KP_Enter : {
                if (editingText) {
                    g_canvas->EnterDoubleClickValueEdit(false);
                    g_canvas->ClearTextSelection();
                    g_canvas->SetPropertyPanel();
                    g_canvas->ScheduleRedraw();
                }
            } break;
            case XK_Escape : {
                g_canvas->EnterDoubleClickValueEdit(false);
                g_canvas->ClearTextSelection();
                g_canvas->ScheduleRedraw();
            } break;
            default: {
                // Printable character typed while editing: replaces the active selection, if any
                if (editingText && !ctrlHeld && typedLen > 0) {
                    unsigned char typedChar = (unsigned char)lookupBuffer[0];
                    if (typedChar >= 0x20 && typedChar < 0x7F) {
                        g_canvas->InsertTextAtCursor(std::string(1, (char)typedChar));
                        g_canvas->ScheduleRedraw();
                    }
                }
            } break;
        }
    }
}

Widget Components::RenderCanvas(Widget parent, Widget leftWidget, Widget rightWidget) {
    Arg args[16];
    int n = 0;

    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, leftWidget); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNrightWidget, rightWidget); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    Widget canvasFrame = XmCreateFrame(parent, (char*)"CanvasFrame", args, n);
    XtManageChild(canvasFrame);

    n = 0;
    XtSetArg(args[n], XmNtraversalOn, True); n++;
    Widget canvasField = XmCreateDrawingArea(canvasFrame, (char*)"CanvasDrawingArea", args, n);
    XtManageChild(canvasField);

    // Create and initialize the global canvas instance
    g_canvas = new CanvasInterface();
    g_canvas->canvas = canvasField;

    XtAddCallback(canvasField, XmNexposeCallback, ExposeCanvasCallback, (XtPointer)g_canvas);
    XtAddCallback(canvasField, XmNinputCallback, CanvasInputCallback, (XtPointer)g_canvas);
    XtAddEventHandler(canvasField, PointerMotionMask, False, [](Widget w, XtPointer clientData, XEvent* event, Boolean* continueDispatch) {
        if (event->type == MotionNotify) {
            CanvasInterface::HandleCanvasMouseMove(event->xmotion.x, event->xmotion.y);
        }
    }, NULL);
    XtAddEventHandler(canvasField, KeyPressMask, False, CanvasKeyPressCallback, NULL);
    XtSetKeyboardFocus(canvasField, canvasField);

    return canvasFrame;
}
