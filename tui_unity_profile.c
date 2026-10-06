/*
 * La demo para MiniCPU con la medicion de eventos pendientes en cada repintado
 * (make mini-profile). Es tui_unity_mini.c con TUI_PROFILE_EVENTS: nada de esto
 * entra en la demo normal.
 */
#define TUI_PROFILE_EVENTS
#define TUI_BACKEND_MMIO
#include "tui_unity.c"
