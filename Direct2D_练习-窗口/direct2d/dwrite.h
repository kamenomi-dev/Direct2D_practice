#pragma once
#include "interface.h"

#include <string>
#include <dwrite.h>
#include <dwrite_1.h>
#pragma comment(lib, "Dwrite.lib")

namespace Direct2D_UI::Direct2D {
class TextFormat : public D2DInterface<IDWriteTextFormat> {
  public:
    TextFormat()          = default;
    virtual ~TextFormat() = default;

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
    TextLayout()          = default;
    virtual ~TextLayout() = default;

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

class DWriteFactory final : public D2DInterface<IDWriteFactory1> {
  public:
    static auto& GetInstance() {
        static DWriteFactory factory{};
        return factory;
    }

    _Success_(return == true)
    bool CreateTextFormat(
        const std::wstring& fontFamily, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style, DWRITE_FONT_STRETCH stretch, float size, _Out_ TextFormat& textFormat
    ) {
        if (!IsValid()) {
            return false;
        }

        TextFormat{GetPointer(), fontFamily, weight, style, stretch, size}.Move(textFormat);
        return true;
    }

    _Success_(return == true)
    bool CreateTextLayout(
        const std::wstring& string, _In_ TextFormat& textFormat, D2D1_SIZE_F maximum, _Out_ TextLayout& textLayout
    ) {
        if (!IsValid() && !textFormat.IsValid()) {
            return false;
        }

        TextLayout{GetPointer(), string, textFormat.GetPointer(), maximum}.Move(textLayout);
        return true;
    }

  private:
    DWriteFactory() { SetLastResult(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory1), (IUnknown**)&GetPointerRef())); }
    ~DWriteFactory() = default;
};
} // namespace Direct2D_UI::Direct2D