#ifndef CONFIG_H
#define CONFIG_H

void LoadSettings();
void SaveSettings();
void SaveWindowPlacement(const WINDOWPLACEMENT &wp);
bool LoadWindowPlacement(WINDOWPLACEMENT &wp);
#endif // CONFIG_H
