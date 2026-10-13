#pragma once
#include "interface.h"
#include "dwrite.h"
#include "../window_const_scale.h"

namespace Direct2D_UI::Direct2D::Resource {
// Todo: if necessary could we make this class support each window.
class ResourceManager {
  public:
    static auto& GetInstance() {
        static ResourceManager manager{};
        return manager;
    }

    void ReleaseResources() const {
        GeneralFont_Normal.Discard();
        GeneralSymbolFont.Discard();
        NormalControlPanelLayout.Discard();
        MaximumControlPanelLayout.Discard();
    }

    void UpdateResources(
        const uint32_t& dpi
    ) {
        ReleaseResources();

        auto& dwrite = DWriteFactory::GetInstance();

        const auto generalFontSize = 24.f / 96.f * dpi;
        dwrite.CreateTextFormat(L"Segoe UI", DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, generalFontSize, GeneralFont_Normal);
        GeneralFont_Normal.SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        const auto buttonSymbolSize = Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_SYMBOL_SIZE / 96.f * dpi;
        dwrite.CreateTextFormat(L"Segoe MDL2 Assets", DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, buttonSymbolSize, GeneralSymbolFont);
        GeneralSymbolFont.SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        const auto buttonWidth  = Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_WIDTH / 96.f * dpi;
        const auto buttonHeight = Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_HEIGHT / 96.f * dpi;

        dwrite.CreateTextLayout(L"\u{E921}\u{E922}\u{E8BB}", GeneralSymbolFont, {.width = buttonWidth * 3, .height = buttonHeight}, NormalControlPanelLayout);
        dwrite.CreateTextLayout(L"\u{E921}\u{E923}\u{E8BB}", GeneralSymbolFont, {.width = buttonWidth * 3, .height = buttonHeight}, MaximumControlPanelLayout);

        NormalControlPanelLayout.SetCharacterSpacing((buttonWidth - buttonSymbolSize) * 0.5f, (buttonWidth - buttonSymbolSize) * 0.5f, 0, DWRITE_TEXT_RANGE{0, 3});
        NormalControlPanelLayout.SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        NormalControlPanelLayout.SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        MaximumControlPanelLayout.SetCharacterSpacing((buttonWidth - buttonSymbolSize) * 0.5f, (buttonWidth - buttonSymbolSize) * 0.5f, 0, DWRITE_TEXT_RANGE{0, 3});
        MaximumControlPanelLayout.SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        MaximumControlPanelLayout.SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

  public:
    inline static Direct2D::TextFormat GeneralFont_Normal;
    inline static Direct2D::TextFormat GeneralSymbolFont;
    inline static Direct2D::TextLayout NormalControlPanelLayout;
    inline static Direct2D::TextLayout MaximumControlPanelLayout;

  private:
    ResourceManager() = default;
    ~ResourceManager() { ReleaseResources(); }
};
} // namespace Direct2D_UI::Direct2D::Resource