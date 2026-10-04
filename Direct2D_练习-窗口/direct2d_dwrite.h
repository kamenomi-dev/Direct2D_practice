#pragma once
#include "direct2d_base.h"

#include <string>
#include <dwrite.h>
#pragma comment(lib, "Dwrite.lib")

namespace Direct2D_UI {
class TextFormat : public D2DInterface<IDWriteTextFormat> {
  public:
    TextFormat() = default;

    void SetTextAlignment(
        const DWRITE_TEXT_ALIGNMENT& textAlignment
    ) {
        SetLastResult(GetPointer()->SetTextAlignment(textAlignment));
    }

    void SetParagraphAlignment(
        const DWRITE_PARAGRAPH_ALIGNMENT& paragraphAlignment
    ) {
        SetLastResult(GetPointer()->SetParagraphAlignment(paragraphAlignment));
    }

    void SetWordWrapping(
        const DWRITE_WORD_WRAPPING& wordWrapping
    ) {
        SetLastResult(GetPointer()->SetWordWrapping(wordWrapping));
    }

  private:
    friend class DWriteFactory;
    TextFormat(
        IDWriteFactory* factory, const std::wstring& fontFamily, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style, DWRITE_FONT_STRETCH stretch, float size
    ) {
        SetLastResult(factory->CreateTextFormat(fontFamily.c_str(), nullptr, weight, style, stretch, size, L"", &GetPointerRef()));
    }
};

class DWriteFactory : public D2DInterface<IDWriteFactory> {
  public:
    DWriteFactory() { SetLastResult(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&GetPointerRef())); }

    void CreateTextFormat(
        const std::wstring& fontFamily, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style, DWRITE_FONT_STRETCH stretch, float size, _Out_ TextFormat& textFormat
    ) {
        TextFormat{GetPointer(), fontFamily, weight, style, stretch, size}.Move(textFormat);
    }
};

inline auto& GetDWriteFactory() {
    static DWriteFactory factory{};
    return factory;
}
} // namespace Direct2D_UI