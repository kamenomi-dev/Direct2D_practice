#pragma once
#include <Windows.h>
#include <stdint.h>
namespace Direct2D_UI::Window::ConstScale {
// 常量列表，尺寸大小以96dpi像素为基准
const static uint32_t WINDOW_CAPTION_HEIGHT = GetSystemMetricsForDpi(SM_CYCAPTION, 96) + GetSystemMetricsForDpi(SM_CYSIZEFRAME, 96) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, 96);
const static uint32_t WINDOW_CONTROL_PANEL_BUTTON_WIDTH       = 44;
const static uint32_t WINDOW_CONTROL_PANEL_BUTTON_HEIGHT      = WINDOW_CAPTION_HEIGHT;
const static float    WINDOW_CONTROL_PANEL_BUTTON_SYMBOL_SIZE = 10.f;
} // namespace Direct2D_UI::Window::ConstScale