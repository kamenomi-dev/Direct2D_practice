#pragma once
#include "direct2d_base.h"
#include "direct2d_dwrite.h"

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
        // I dont know size of font.
        const auto buttonSymbolSize = 8.f;
        GetDWriteFactory().CreateTextFormat(
            L"Segoe MDL2 Assets", DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, buttonSymbolSize, GeneralSystemSymbolFont
        );
        GeneralSystemSymbolFont.SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        const auto buttonWidth  = GetSystemMetrics(SM_CXSIZE);
        const auto buttonHeight = (float)GetSystemMetrics(SM_CYCAPTION) + (float)GetSystemMetrics(SM_CYSIZEFRAME) + (float)GetSystemMetrics(SM_CXPADDEDBORDER);

        GetDWriteFactory().CreateTextLayout(
            L"\u{E921}\u{E922}\u{E8BB}", GeneralSystemSymbolFont, {.width = buttonWidth * 3.f, .height = buttonHeight * 1.f}, SystemControlPanelLayout
        );

        auto result = SystemControlPanelLayout.SetCharacterSpacing(
            buttonWidth * 0.5f - buttonSymbolSize * 0.5f, buttonWidth * 0.5f - buttonSymbolSize * 0.5f, 0.f, DWRITE_TEXT_RANGE{0, 3}
        );
        SystemControlPanelLayout.GetPointer()->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        SystemControlPanelLayout.GetPointer()->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

  public:
    inline static Direct2D_UI::TextFormat GeneralSystemSymbolFont;
    inline static Direct2D_UI::TextLayout SystemControlPanelLayout;
};
} // namespace Direct2D_UI::Resource