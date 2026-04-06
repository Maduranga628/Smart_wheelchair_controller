#pragma once
#include "control_cmd.h"

void ui_init();
void ui_task(void* pv);
control_mode_t ui_get_selected_mode();
bool ui_is_armed();

