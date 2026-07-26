#ifndef __EVO_UI_H
#define __EVO_UI_H

#define OPL_EVO_APP_NAME      "OPL Evolution"
#define OPL_EVO_APP_ID        "OPL-EVO"
#define OPL_EVO_VERSION_MAJOR 0
#define OPL_EVO_VERSION_MINOR 27
#define OPL_EVO_VERSION_PATCH 0
#define OPL_EVO_VERSION_STAGE "alpha"
#define OPL_EVO_VERSION       "0.27.0-alpha"

void evoUiSetEnabled(int enabled);
int evoUiIsEnabled(void);

void evoUiRenderMain(void);
void evoUiHandleInputMain(void);
void evoUiRenderControllers(void);
void evoUiHandleInputControllers(void);
void evoUiRenderDiagnostics(void);
void evoUiHandleInputDiagnostics(void);

#endif
