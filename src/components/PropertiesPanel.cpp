#include "CanvasInterface.h"
#include "Components.h"
#include <X11/Composite.h>
#include <X11/Intrinsic.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>
#include <Xm/TextF.h>
#include <Xm/XmAll.h>
#include <cstddef>
#include <regex>
#include <string>

// Static member definitions
PropertiesPanelField PropertiesPanel::instanceName;
PropertiesPanelField PropertiesPanel::valueContent;
PropertiesPanelField PropertiesPanel::xPos;
PropertiesPanelField PropertiesPanel::yPos;
PropertiesPanelField PropertiesPanel::width;
PropertiesPanelField PropertiesPanel::height;

void UnisonOnChange(Widget widget, XtPointer clientData, XtPointer callData) {
    g_canvas->ApplyPropertyPanelChanges();
}

void ToggleSnapToGridCheckbox(Widget w, XtPointer clientData, XtPointer callData) {
    XmToggleButtonCallbackStruct *cbData = (XmToggleButtonCallbackStruct *)callData;
    bool newState = (cbData->set == XmSET);
    g_canvas->SetSnapToGrid(newState);
    Logger::log((std::string("Snap to Grid " + std::string(newState ? "enabled" : "disabled")).c_str()));
}

Widget Components::RenderPropertiesPanel(Widget parent) {
    Arg args[10];
    int n = 0;

    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNwidth, 40); n++;
    Widget propertiesPanel = XmCreateFrame(parent, (char*)"PropsPanel", args, n);
    XtManageChild(propertiesPanel);

    n = 0;
    XtSetArg(args[n], XmNmarginWidth, 4); n++;
    XtSetArg(args[n], XmNmarginHeight, 4); n++;
    XtSetArg(args[n], XmNwidth, 160); n++;
    Widget propertiesForm = XmCreateForm(propertiesPanel, (char*)"PropsForm", args, n);
    XtManageChild(propertiesForm);
    PropertiesPanel::instanceName.RenderField(propertiesForm, (char*)"instanceName", (char*)"Instance Name", (char*)"", UnisonOnChange, true);
    PropertiesPanel::valueContent.RenderField(propertiesForm, (char*)"valueContent", (char*)"Value Content", (char*)"", UnisonOnChange);
    PropertiesPanel::xPos.RenderField(propertiesForm, (char*)"xPos", (char*)"X Position", (char*)"", UnisonOnChange);
    PropertiesPanel::yPos.RenderField(propertiesForm, (char*)"yPos", (char*)"Y Position", (char*)"", UnisonOnChange);
    PropertiesPanel::width.RenderField(propertiesForm, (char*)"width", (char*)"Width", (char*)"", UnisonOnChange);
    PropertiesPanel::height.RenderField(propertiesForm, (char*)"height", (char*)"Height", (char*)"", UnisonOnChange);
    // todo 0.8: add snap to grid checkmark option here
    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, PropertiesPanel::height.GetWidget()); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNmarginHeight, 6); n++;
    XtSetArg(args[n], XmNlabelString, XmStringCreateLocalized((char*)"Snap to Grid")); n++;
    XtSetArg(args[n], XmNset, g_canvas->GetSnapToGridStatus() ? XmSET : XmUNSET); n++;
    Widget snapToGridCheckbox = XmCreateToggleButton(propertiesForm, (char*)"SnapToGridCheckbox", args, n);
    XtManageChild(snapToGridCheckbox);
    XtAddCallback(snapToGridCheckbox, XmNvalueChangedCallback, ToggleSnapToGridCheckbox, NULL);

    return propertiesPanel;
}
std::string PropertiesPanel::GetWidgetValue(PropertiesPanelField field) {
    return field.GetValue();
}
