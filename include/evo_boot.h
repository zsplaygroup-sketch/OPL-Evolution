#ifndef __EVO_BOOT_H
#define __EVO_BOOT_H

enum evo_boot_platform {
    EVO_BOOT_PS2 = 0,
    EVO_BOOT_PS1
};

void evoBootPlay(enum evo_boot_platform platform, int ps1Device,
                 const char *ps1VcdName);

#endif
