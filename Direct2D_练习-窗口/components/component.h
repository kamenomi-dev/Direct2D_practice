#pragma once
#include "../direct2d/render.h"

namespace Direct2D_UI {
class BaseWindow;
}

namespace Direct2D_UI::Components {
class Window;

class Component {
  public:
    Component(const Component&) = delete;
    Component(Component&&)      = delete;
    Component()                 = default;
    ~Component()                = default;

    virtual bool Render() = 0;

    virtual void CreateResources()  = 0;
    virtual void DiscardResources() = 0;

    HWND GetRenderTargetWindow() {
        if (!_graphicsRef.IsValid()) {
            return nullptr;
        }

        return _graphicsRef.GetPointer()->GetHwnd();
    }

    virtual D2D1_POINT_2F GetPosition() const { return _position; };
    virtual D2D1_SIZE_F   GetSize() const { return _size; };

  private:
    friend class Components::Window;
    friend class Direct2D_UI::BaseWindow;
    void UpdateRenderTarget(
        _In_ Direct2D::Graphics& source
    ) {
        _graphicsRef.Discard();
        DiscardResources();

        source.AddRef(_graphicsRef);
        CreateResources();
    }

  private:
    Direct2D::Graphics _graphicsRef;
    float_t            _renderDpi = 96.f; // Todo: 当祖父组件传递事件更新状态时，要更新这个dpi
    D2D1_POINT_2F      _position;
    D2D1_SIZE_F        _size;
};
} // namespace Direct2D_UI::Components