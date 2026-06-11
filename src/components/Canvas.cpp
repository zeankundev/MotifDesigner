#include "CanvasInterface.h"
#include "Components.h"
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
#include <sstream>
#include <string>
#include <vector>

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
GC selectBorderGraphicsContext;
bool graphicsContextsInitialized = false;

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

    XAllocNamedColor(display, cmap, "#e0e0e0", &color, &exact);
    values.foreground = color.pixel;
    gridContext = XCreateGC(display, win, GCForeground, &values);

    XAllocNamedColor(display, cmap, "#808080", &color, &exact);
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

    XAllocNamedColor(display, cmap, "#008080", &color, &exact);
    values.foreground = color.pixel;
    values.line_style = LineOnOffDash;
    selectBorderGraphicsContext = XCreateGC(display, win, GCForeground | GCLineStyle, &values);

    graphicsContextsInitialized = true;
}

void CanvasInterface::DrawGrid(Display* display, Window win, int width, int height) {
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

    XDrawLine(display, win, topContext, x, y, x+width-1, y);
    XDrawLine(display, win, topContext, x, y, x, y+height-1);
    XDrawLine(display, win, bottomContext, x+width-1, y+height-1, x+width-1, y);
    XDrawLine(display, win, bottomContext, x+width-1, y+height-1, x, y+height-1);
}

void CanvasInterface::DrawWidgetElement(Display* display, Window win, const EditorWidgetInstance& widget) {
    XFillRectangle(display, win, widgetBackgroundContext, widget.x, widget.y, widget.width, widget.height);

    switch (widget.type) {
        case ToolTypes::Button: {
            CanvasInterface::DrawBevel(display, win, widget.x, widget.y, widget.width, widget.height, false);
            int textY = widget.y + (widget.height / 2) + 4;
            int textX = widget.x + (widget.width - (widget.value.length() * 6)) / 2;
            if (textX < widget.x + 4) textX = widget.x + 4;
            XDrawString(display, win, textContext, textX, textY, widget.value.c_str(), widget.value.length());
            break;
        }
        case ToolTypes::Label: {
            int textY = widget.y + (widget.height / 2) + 4;
            int textX = widget.x + 4;
            XDrawString(display, win, textContext, textX, textY, widget.value.c_str(), widget.value.length());
            break;
        }
        case ToolTypes::TextField: {
            CanvasInterface::DrawBevel(display, win, widget.x, widget.y, widget.width, widget.height, true);
            int textY = widget.y + (widget.height / 2) + 4;
            XDrawString(display, win, textContext, widget.x + 6, textY, widget.value.c_str(), widget.value.length());
            int cursorX = widget.x + 8 + (widget.value.length() * 6);
            if (cursorX < widget.x + widget.width - 6) {
                XDrawLine(display, win, textContext, cursorX, widget.y + 4, cursorX, widget.y + widget.height - 4);
            }
            break;
        }
        case ToolTypes::Toggle: {
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
            CanvasInterface::DrawBevel(display, win, widget.x, widget.y, widget.width, widget.height, true);
            XDrawString(display, win, textContext, widget.x + 8, widget.y + 14, widget.value.c_str(), widget.value.length());
            break;
        }
        default:
            break;
    }
    if (widget.selected) {
        XDrawRectangle(display, win, selectBorderGraphicsContext, widget.x - 2, widget.y - 2, widget.width + 4, widget.height + 4);
        XFillRectangle(display, win, textContext, widget.x - 2, widget.y - 2, resizeHandleSize, resizeHandleSize);
    }
}

void CanvasInterface::RefreshCanvas() {
    if (!this->canvas || !XtIsRealized(this->canvas)) return;
    Display* display = XtDisplay(this->canvas);
    Window win = XtWindow(this->canvas);

    Dimension w, h;
    Arg args[2];
    int n = 0;
    XtSetArg(args[n], XmNwidth, &w); n++;
    XtSetArg(args[n], XmNheight, &h); n++;
    XtGetValues(this->canvas, args, 2);

    CanvasInterface::DrawGrid(display, win, w, h);
    for (const auto& widget : widgets) {
        CanvasInterface::DrawWidgetElement(display, win, widget);
    }
}

void CanvasInterface::SetPropertyPanel() {
    PropertiesPanel::instanceName.UpdateFieldValue(widgets[selectedIndex].name.c_str());
    PropertiesPanel::valueContent.UpdateFieldValue(widgets[selectedIndex].value.c_str());
    PropertiesPanel::xPos.UpdateFieldValue(std::to_string(widgets[selectedIndex].x).c_str());
    PropertiesPanel::yPos.UpdateFieldValue(std::to_string(widgets[selectedIndex].y).c_str());
    PropertiesPanel::width.UpdateFieldValue(std::to_string(widgets[selectedIndex].width).c_str());
    PropertiesPanel::height.UpdateFieldValue(std::to_string(widgets[selectedIndex].height).c_str());
}

void CanvasInterface::ClearPropertyPanel() {
    PropertiesPanel::instanceName.UpdateFieldValue("");
    PropertiesPanel::valueContent.UpdateFieldValue("");
    PropertiesPanel::xPos.UpdateFieldValue("");
    PropertiesPanel::yPos.UpdateFieldValue("");
    PropertiesPanel::width.UpdateFieldValue("");
    PropertiesPanel::height.UpdateFieldValue("");
}

void CanvasInterface::SelectWidget(int index) {
    if (selectedIndex >= 0 && selectedIndex < (int)widgets.size()) {
        widgets[selectedIndex].selected = false;
    }

    selectedIndex = index;

    if (selectedIndex >= 0 && selectedIndex < (int)widgets.size()) {
        widgets[selectedIndex].selected = true;
        SetPropertyPanel();
    } else {
        ClearPropertyPanel();
    }
    RefreshCanvas();
}

void CanvasInterface::SetTool(ToolTypes tool) {
    activeTool = tool;
}

void CanvasInterface::HandleCanvasMouseDown(int mouseX, int mouseY) {
    if (g_canvas == nullptr) return;
    if (activeTool == ToolTypes::Select) {
        for (int i = widgets.size() - 1; i >= 0; --i) {
            const auto& w = widgets[i];
            if (w.selected) {
                int handleX = w.x + w.width - 2;
                int handleY = w.y + w.height - 2;
                if (mouseX >= handleX && mouseX <= handleX + resizeHandleSize &&
                mouseY >= handleY && mouseY <= handleY + resizeHandleSize) {
                    isResizing = true;
                    dragStartX = mouseX;
                    dragStartY = mouseY;
                    widget_originalWidth = w.width;
                    widget_originalHeight = w.height;
                    g_canvas->SelectWidget(i);
                    return;
                }
            }
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
                defaultHeight = 24;
                break;
            case ToolTypes::TextField:
                defaultName = "TextField";
                defaultValue = "Text Field";
                defaultHeight = 30;
                break;
            case ToolTypes::Toggle:
                defaultName = "ToggleButton";
                defaultValue = "Toggle";
                defaultHeight = 24;
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
        w.x = newX;
        w.y = newY;
        g_canvas->SetPropertyPanel();
        g_canvas->RefreshCanvas();
    } else if (isResizing) {
        int newWidth = widget_originalWidth + dx;
        int newHeight = widget_originalHeight + dy;

        if (snapToGrid) {
            newWidth = (newWidth / gridSize) * gridSize;
            newHeight = (newHeight / gridSize) * gridSize;
        }

        if (newWidth < 20) newWidth = 20;
        if (newHeight < 10) newHeight = 10;

        w.width = newWidth;
        w.height = newHeight;
        g_canvas->SetPropertyPanel();
        g_canvas->RefreshCanvas();
    }
    g_canvas->RefreshCanvas();
}

void CanvasInterface::HandleCanvasMouseUp() {
    isDragging = false;
    isResizing = false;
}

void CanvasInterface::ApplyPropertyPanelChanges() {
    if (selectedIndex < 0 || selectedIndex >= (int)widgets.size()) return;

    auto& w = widgets[selectedIndex];

    char* nameStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::instanceName);
    char* valueStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::valueContent);
    char* xStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::xPos);
    char* yStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::yPos);
    char* widthStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::width);
    char* heightStr = PropertiesPanel::GetWidgetValue(PropertiesPanel::height);

    if (nameStr) { w.name = nameStr; XtFree(nameStr); };
    if (valueStr) { w.value = valueStr; XtFree(valueStr); };
    if (xStr) { w.x = atoi(xStr); XtFree(xStr); };
    if (yStr) { w.y = atoi(yStr); XtFree(yStr); };
    if (widthStr) { w.width = atoi(widthStr); XtFree(widthStr); };
    if (heightStr) { w.height = atoi(heightStr); XtFree(heightStr); };
    RefreshCanvas();
}

void CanvasInterface::DeleteSelectedWidget() {
    if (selectedIndex >= 0 && selectedIndex < (int)widgets.size()) {
        widgets.erase(widgets.begin() + selectedIndex);
        SelectWidget(-1);
    }
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
        canvas->HandleCanvasMouseDown(x, y);
    } else if (event->type == MotionNotify) {
        int x = event->xbutton.x;
        int y = event->xbutton.y;
        canvas->HandleCanvasMouseMove(x, y);
    } else if (event->type == ButtonRelease) {
        canvas->HandleCanvasMouseUp();
    }
}

void CanvasKeyPressCallback(Widget w, XtPointer clientData, XEvent* event, Boolean* continueDispatch) {
    CanvasInterface* canvas = static_cast<CanvasInterface*>(clientData);
    if (event->type == KeyPress) {
        KeySym sym = XLookupKeysym(&event->xkey, 0);
        if (sym == XK_Delete || sym == XK_BackSpace) {
            canvas->DeleteSelectedWidget();
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

    return canvasFrame;
}
