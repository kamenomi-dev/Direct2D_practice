#pragma once
#include "component.h"
#include "../direct2d/resource_manager.h"

namespace Direct2D_UI::Components {
// Notice: 这里的标签组件实现基于 TextLayout，所以说，如果更改了文本内容，需要重新设置样式的。
class DurableLabel : public Component {
  public:
    DurableLabel()  = default;
    ~DurableLabel() = default;

    void CreateResources(
        Direct2D::Graphics& graphics
    ) override {
        Direct2D::DWriteFactory::GetInstance().CreateTextLayout(_content, Direct2D::Resource::ResourceManager::GeneralFont_Normal, GetSize(), _layout);
        graphics.CreateSolidColorBrush(D2D1::ColorF::White, _textBrush);
    };
    void DiscardResources() override { _layout.Discard(); };

    bool Render(
        Direct2D::Graphics& graphics
    ) override {
        graphics.DrawTextLayout({0.f, 0.f}, _layout, _textBrush);
        return true;
    };

    std::wstring_view GetContent() const { return _content; }

    void SetContent(
        std::wstring_view newContent
    ) {
        _layout.Discard();

        _content = newContent;
        Direct2D::DWriteFactory::GetInstance().CreateTextLayout(_content, Direct2D::Resource::ResourceManager::GeneralFont_Normal, GetSize(), _layout);
    }

    void SetTextColor(
        const D2D1::ColorF& color
    ) {
        _textBrush.SetColor(color);
    }

    _Success_(return == true)
    bool SetParagraphAlign(
        float leadingSpacing, float trailingSpacing, float minimumAdvanceWidth, const DWRITE_TEXT_RANGE& range
    ) {
        return _layout.SetCharacterSpacing(leadingSpacing, trailingSpacing, minimumAdvanceWidth, range);
    }

    _Success_(return == true)
    bool SetParagraphAlign(
        const DWRITE_PARAGRAPH_ALIGNMENT& align
    ) {
        return _layout.SetParagraphAlignment(align);
    }

    _Success_(return == true)
    bool SetTextAlignment(
        const DWRITE_TEXT_ALIGNMENT& align
    ) {
        return _layout.SetTextAlignment(align);
    }

    _Success_(return == true)
    bool SetWordWrapping(
        const DWRITE_WORD_WRAPPING& wordWrapping
    ) {
        return _layout.SetWordWrapping(wordWrapping);
    }

    auto GetTextColor() { return _textBrush.GetColor(); }

  private:
    Direct2D::TextLayout      _layout;
    Direct2D::SolidColorBrush _textBrush;
    std::wstring              _content;
};
} // namespace Direct2D_UI::Components