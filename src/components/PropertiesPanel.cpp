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

class PropertiesPanelField {
    private:
        Widget field;
        char* lastValidValue;
    public:
        PropertiesPanelField() : field(NULL), lastValidValue(NULL) {}
        ~PropertiesPanelField() {
            if (lastValidValue != NULL) {
                XtFree(lastValidValue);
            }
        }
        PropertiesPanelField& RenderField(Widget parent, const char* label, const char* hint, const char* value, void(*callback)(Widget, XtPointer, XtPointer), bool requireStrictValues = false) {
            Arg args[16];
            int n = 0;

            Widget *child = NULL;
            Cardinal childQuantity = 0;
            XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;

            Arg qargs[2];
            XtSetArg(qargs[0], XmNchildren, &child);
            XtSetArg(qargs[1], XmNnumChildren, &childQuantity);
            XtGetValues(parent, qargs, 2);
            if (childQuantity > 0) {
                n--;
                XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
                XtSetArg(args[n], XmNtopWidget, child[childQuantity - 1]); n++;
                XtSetArg(args[n], XmNtopOffset, 10); n++;
            }

            XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
            XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
            Widget propertyFieldParent = XmCreateForm(parent, (char*)"PropertyFieldDummy", args, n);
            XtManageChild(propertyFieldParent);

            n = 0;
            XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
            XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
            XtSetArg(args[n], XmNlabelString, XmStringCreateLocalized((char*)hint)); n++;
            Widget propertyHint = XmCreateLabel(propertyFieldParent, (char*)"label", args, n);
            XtManageChild(propertyHint);

            n = 0;
            XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
            XtSetArg(args[n], XmNtopWidget, propertyHint); n++;
            XtSetArg(args[n], XmNtopOffset, 5); n++;
            XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
            XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
            XtSetArg(args[n], XmNvalue, value); n++;

            field = XmCreateTextField(propertyFieldParent, (char*)"field", args, n);

            // Store initial value as the last known valid value
            if (lastValidValue != NULL) {
                XtFree(lastValidValue);
            }
            lastValidValue = XtNewString(value);

            // Create a callback wrapper that validates and manages state
            struct CallbackData {
                void (*userCallback)(Widget, XtPointer, XtPointer);
                char** pLastValidValue;
                bool requireStrict;
                std::regex* validationRegex;
            };

            CallbackData* cbData = new CallbackData{
                callback,
                &lastValidValue,
                requireStrictValues,
                new std::regex("^[A-Za-z0-9_]+$")
            };

            auto ValidateBeforeCallback = [](Widget w, XtPointer clientData, XtPointer callData) {
                CallbackData* data = static_cast<CallbackData*>(clientData);
                const char* currentValue = XmTextFieldGetString(w);

                if (data->requireStrict) {
                    // Validate against regex pattern
                    if (!std::regex_match(currentValue, *(data->validationRegex))) {
                        // Invalid: restore last known valid value and skip callback
                        XmTextFieldSetString(w, *(data->pLastValidValue));
                        XtFree((char*)currentValue);
                        return;
                    }
                }

                // Valid: update last known value and execute callback
                if (*(data->pLastValidValue) != NULL) {
                    XtFree(*(data->pLastValidValue));
                }
                *(data->pLastValidValue) = XtNewString(currentValue);

                if (data->userCallback != NULL) {
                    data->userCallback(w, NULL, callData);
                }
                XtFree((char*)currentValue);
            };

            if (callback != NULL || requireStrictValues) {
                XtAddCallback(field, XmNactivateCallback, ValidateBeforeCallback, (XtPointer)cbData);
            }
            XtManageChild(field);
            return *this;
        }
        void UpdateFieldValue(const char* value) {
            Arg args[1];
            XtSetArg(args[0], XmNvalue, value);
            XtSetValues(field, args, 1);
        }
};

PropertiesPanelField instanceName;
PropertiesPanelField valueContent;
PropertiesPanelField xPos;
PropertiesPanelField yPos;
PropertiesPanelField width;
PropertiesPanelField height;

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
    instanceName.RenderField(propertiesForm, (char*)"instanceName", (char*)"Instance Name", (char*)"PushButton1", NULL, true);
    valueContent.RenderField(propertiesForm, (char*)"valueContent", (char*)"Value Content", (char*)"", NULL);
    xPos.RenderField(propertiesForm, (char*)"xPos", (char*)"X Position", (char*)"", NULL);
    yPos.RenderField(propertiesForm, (char*)"yPos", (char*)"Y Position", (char*)"", NULL);
    width.RenderField(propertiesForm, (char*)"width", (char*)"Width", (char*)"", NULL);
    height.RenderField(propertiesForm, (char*)"height", (char*)"Height", (char*)"", NULL);
    return propertiesPanel;
}
