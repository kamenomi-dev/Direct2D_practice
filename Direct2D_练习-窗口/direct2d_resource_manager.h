#pragma once
#include "direct2d_base.h"
#include "direct2d_dwrite.h"
#include "direct2d_window_const_scale.h"

namespace Direct2D_UI::Resource {
class ResourceManager {
  private:
    ResourceManager() { Initialize(); };

  public:
    static auto& GetResourceManager() {
        static ResourceManager manager{};
        return manager;
    }

  private:
    void Initialize() const {
        const auto currentDpi = GetDpiForSystem();

        const auto buttonSymbolSize = Direct2D_UI::Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_SYMBOL_SIZE / 96.f * currentDpi;
        GetDWriteFactory().CreateTextFormat(
            L"Segoe MDL2 Assets", DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, buttonSymbolSize, GeneralSymbolFont
        );
        GeneralSymbolFont.SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        const auto buttonWidth  = Direct2D_UI::Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_WIDTH / 96.f * currentDpi;
        const auto buttonHeight = Direct2D_UI::Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_HEIGHT / 96.f * currentDpi;

        GetDWriteFactory().CreateTextLayout(L"\u{E921}\u{E922}\u{E8BB}", GeneralSymbolFont, {.width = buttonWidth * 3, .height = buttonHeight}, NormalControlPanelLayout);
        GetDWriteFactory().CreateTextLayout(L"\u{E921}\u{E923}\u{E8BB}", GeneralSymbolFont, {.width = buttonWidth * 3, .height = buttonHeight}, MaximumControlPanelLayout);

        NormalControlPanelLayout.SetCharacterSpacing((buttonWidth - buttonSymbolSize) * 0.5f, (buttonWidth - buttonSymbolSize) * 0.5f, 0, DWRITE_TEXT_RANGE{0, 3});
        NormalControlPanelLayout.SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        NormalControlPanelLayout.SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        MaximumControlPanelLayout.SetCharacterSpacing((buttonWidth - buttonSymbolSize) * 0.5f, (buttonWidth - buttonSymbolSize) * 0.5f, 0, DWRITE_TEXT_RANGE{0, 3});
        MaximumControlPanelLayout.SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        MaximumControlPanelLayout.SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

  public:
    inline static Direct2D_UI::TextFormat GeneralSymbolFont;
    inline static Direct2D_UI::TextLayout NormalControlPanelLayout;
    inline static Direct2D_UI::TextLayout MaximumControlPanelLayout;
};
} // namespace Direct2D_UI::Resource