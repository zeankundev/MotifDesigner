#include "CanvasInterface.h"
#include "Components.h"
#include <X11/Composite.h>
#include <X11/Intrinsic.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>
#include <Xm/TextF.h>
#include <Xm/XmAll.h>
#include <cstddef>
#include <regex>

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
    return propertiesPanel;
}
std::string PropertiesPanel::GetWidgetValue(PropertiesPanelField field) {
    return field.GetValue();
}
