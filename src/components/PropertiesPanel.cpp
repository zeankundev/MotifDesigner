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
    PropertiesPanel::instanceName.RenderField(propertiesForm, (char*)"instanceName", (char*)"Instance Name", (char*)"PushButton1", NULL, true);
    PropertiesPanel::valueContent.RenderField(propertiesForm, (char*)"valueContent", (char*)"Value Content", (char*)"", NULL);
    PropertiesPanel::xPos.RenderField(propertiesForm, (char*)"xPos", (char*)"X Position", (char*)"", NULL);
    PropertiesPanel::yPos.RenderField(propertiesForm, (char*)"yPos", (char*)"Y Position", (char*)"", NULL);
    PropertiesPanel::width.RenderField(propertiesForm, (char*)"width", (char*)"Width", (char*)"", NULL);
    PropertiesPanel::height.RenderField(propertiesForm, (char*)"height", (char*)"Height", (char*)"", NULL);
    return propertiesPanel;
}
char* PropertiesPanel::GetWidgetValue(PropertiesPanelField field) {
    return field.GetValue();
}
