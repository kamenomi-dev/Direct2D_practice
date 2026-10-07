#pragma once
#include "direct2d_base.h"

#include <string>
#include <dwrite.h>
#include <dwrite_1.h>
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

class TextLayout : public D2DInterface<IDWriteTextLayout> {
  public:
    TextLayout() = default;

    bool SetCharacterSpacing(
        float leadingSpacing, float trailingSpacing, float minimumAdvanceWidth, const DWRITE_TEXT_RANGE& range
    ) {
        if (D2DInterface<IDWriteTextLayout1> layout; As<IDWriteTextLayout1>(layout)) {
            return SUCCEEDED(layout.GetPointer()->SetCharacterSpacing(leadingSpacing, trailingSpacing, minimumAdvanceWidth, range));
        }

        return false;
    }

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
    TextLayout(
        IDWriteFactory1* factory, const std::wstring& string, IDWriteTextFormat* textFormat, D2D1_SIZE_F maximum
    ) {
        SetLastResult(factory->CreateTextLayout(string.c_str(), (uint32_t)string.size(), textFormat, maximum.width, maximum.height, &GetPointerRef()));
    }
};

class DWriteFactory : public D2DInterface<IDWriteFactory1> {
  public:
    DWriteFactory() { SetLastResult(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory1), (IUnknown**)&GetPointerRef())); }

    void CreateTextFormat(
        const std::wstring& fontFamily, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style, DWRITE_FONT_STRETCH stretch, float size, _Out_ TextFormat& textFormat
    ) {
        TextFormat{GetPointer(), fontFamily, weight, style, stretch, size}.Move(textFormat);
    }

    void CreateTextLayout(
        const std::wstring& string, _In_ TextFormat& textFormat, D2D1_SIZE_F maximum, _Out_ TextLayout& textLayout
    ) {
        TextLayout{GetPointer(), string, textFormat.GetPointer(), maximum}.Move(textLayout);
    }
};

inline auto& GetDWriteFactory() {
    static DWriteFactory factory{};
    return factory;
}
} // namespace Direct2D_UI