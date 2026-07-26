/*
  Copyright 2009, Volca
  Copyright 2026, OPL Evolution contributors
  Licenced under Academic Free License version 3.0
  Review OpenUsbLd README & LICENSE files for further details.

  OPL Evolution standalone application entrypoint.
*/

#include "include/opl.h"
#include "include/evo_engine.h"
#include "include/evo_startup.h"
#include "include/evo_ui.h"
#include "include/gui.h"

int main(int argc, char *argv[])
{
    /*
     * Initialize the reusable PS2 runtime and device services without enabling
     * OPL's command-line auto-launch application path. Evolution owns the
     * application entrypoint and all foreground UI state from this point.
     */
    oplRuntimeInitialize(argc, argv, 0);
    if (evoStartupBegin() == 0) {
        evoEngineInit();
        evoStartupAdvance(58);
        evoUiSetEnabled(1);
        evoStartupAdvance(82);
        evoStartupFinish();
    } else {
        evoEngineInit();
        evoUiSetEnabled(1);
        guiIntroLoop();
    }

    guiMainLoop();
    return 0;
}
