#ifndef COMPONENTS_H
#define COMPONENTS_H
#include "Logger.h"
#include <X11/Intrinsic.h>
#include <Xm/Form.h>
#include <X11/Intrinsic.h>
#include <Xm/MessageB.h>
#include <Xm/Label.h>
#include <Xm/TextF.h>
#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>
#include <iostream>
#include <regex>
#include <string>

class Components {
    public:
        static Widget RenderToolbar(Widget parent);
        static Widget RenderMenubar(Widget parent);
        static Widget RenderPropertiesPanel(Widget parent);
        static Widget RenderCanvas(Widget parent, Widget leftWidget, Widget rightWidget);
};

class PropertiesPanelField {
    private:
        Widget field;
        std::string lastValidValue;
        bool requireStrictValues;
        void(*externalCallback)(Widget, XtPointer, XtPointer);
        
        static void InternalValidationCallback(Widget w, XtPointer clientData, XtPointer callData) {
            PropertiesPanelField* pThis = (PropertiesPanelField*)clientData;
            if (!pThis->requireStrictValues) {
                // If strict validation not required, just update and call external callback
                char* widgetValue = XmTextFieldGetString(w);
                if (widgetValue != NULL) {
                    pThis->lastValidValue = widgetValue;
                }
                if (pThis->externalCallback != NULL) {
                    pThis->externalCallback(w, clientData, callData);
                }
                return;
            }
            
            // Get the current field value
            char* widgetValue = XmTextFieldGetString(w);
            if (widgetValue == NULL) return;
            
            std::string currentValue(widgetValue);
            
            // Validate against strict format
            if (!pThis->ValidateStrictFormat(currentValue.c_str())) {
                // Invalid - revert to last valid value
                Arg args[1];
                XtSetArg(args[0], XmNmessageString, XmStringCreateLocalized((char*)"Invalid format! Use only letters, numbers, and underscores."));
                Widget errorDialog = XmCreateErrorDialog(XtParent(w), (char*)"ErrorDialog", args, 1);
                Widget helpButton = XmMessageBoxGetChild(errorDialog, XmDIALOG_HELP_BUTTON);
                XtUnmanageChild(helpButton);    
                XtManageChild(errorDialog);
                pThis->UpdateFieldValue(pThis->lastValidValue.c_str());
                return;
            }
            
            // Valid - update last valid value and call external callback
            pThis->lastValidValue = currentValue;
            if (pThis->externalCallback != NULL) {
                pThis->externalCallback(w, clientData, callData);
            }
        }
        
    public:
        PropertiesPanelField() : field(NULL), lastValidValue(""), requireStrictValues(false), externalCallback(NULL) {}
        ~PropertiesPanelField() {
            // No cleanup needed - std::string manages its own memory
        }
        PropertiesPanelField& RenderField(Widget parent, const char* label, const char* hint, const char* value, void(*callback)(Widget, XtPointer, XtPointer), bool strict = false) {
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
            lastValidValue = (value != NULL) ? value : "";
            requireStrictValues = strict;
            externalCallback = callback;
            
            // Always attach internal validation callback if strict mode is enabled or external callback exists
            if (strict || callback != NULL) {
                XtAddCallback(field, XmNactivateCallback, InternalValidationCallback, (XtPointer)this);
            }
            XtManageChild(field);
            return *this;
        }
        void UpdateFieldValue(const char* value) {
            if (field == NULL) return;
            if (value == NULL) return;

            // Assume programmatic updates are always valid
            lastValidValue = value;
            
            Arg args[1];
            XtSetArg(args[0], XmNvalue, value);
            XtSetValues(field, args, 1);
        }
        
        std::string GetValue() {
            Logger::log("Getting value of field");
            if (field == NULL) return "";

            char* widgetValue = XmTextFieldGetString(field);
            if (widgetValue == NULL) return "";

            // Create a copy immediately and don't hold the reference
            std::string result(widgetValue);
            // Note: DO NOT free widgetValue - it's managed by Motif

            return result;
        }
        bool ValidateStrictFormat(const char* value) {
            static const std::regex nameRegex("^[A-Za-z0-9_]+$");
            if (value == NULL) return false;
            return std::regex_match(value, nameRegex);
        }


};
class PropertiesPanel {
    public:
        static PropertiesPanelField instanceName;
        static PropertiesPanelField valueContent;
        static PropertiesPanelField xPos;
        static PropertiesPanelField yPos;
        static PropertiesPanelField width;
        static PropertiesPanelField height;
        static std::string GetWidgetValue(PropertiesPanelField field);
};
class StatusBar {
    public:
        static Widget RenderStatusBar(Widget parent);
        static void UpdateStatusBar(const char* message);
};
#endif
