#pragma once
#include "../direct2d/render.h"

#include <list>
#include <memory>

namespace Direct2D_UI {
class BaseWindow;
}

namespace Direct2D_UI::Components {
class Window;
}

namespace Direct2D_UI::Components {
enum class EventTypes : uint16_t {
    RenderEvent,
    MouseEvent
};

struct ComponentEvent {
    EventTypes type;
};

struct ComponentRenderEvent : public ComponentEvent {
    EventTypes type = EventTypes::RenderEvent;
};

struct ComponentMouseEvent : ComponentEvent {};

class Component {
    using ComponentPtr = std::shared_ptr<Component>;

  public:
    Component(const Component&) = delete;
    Component(Component&&)      = delete;
    Component()                 = default;
    ~Component()                = default;

    // Node operation

  public:
    auto* GetParent() const { return _parent; }
    auto& GetChildren() const { return _children; }

    void AppendChild(
        const ComponentPtr& child
    ) {
        InsertChildBefore(child, nullptr);
    }

    void PrependChild(
        const ComponentPtr& child
    ) {
        InsertChildBefore(child, _children.empty() ? nullptr : _children.front().get());
    }

    void InsertChildBefore(
        const ComponentPtr& child, _In_ const Component* reference
    ) {
        if (!child || child.get() == this || child->_parent != nullptr) {
            return;
        }

        auto position = _children.end();
        if (reference != nullptr) {
            position = FindChild(reference);
            if (position == _children.end()) {
                return;
            }
        }

        child->_parent = this;
        _children.insert(position, child);
        InvalidateSelf();
    }

    _Success_(return == true);
    bool RemoveChild(
        const Component* child
    ) {
        const auto position = FindChild(child);
        if (position == _children.end()) {
            return false;
        }

        (*position)->_parent = nullptr;
        _children.erase(position);
        InvalidateSelf();
        return true;
    }

    void RemoveFromParent() {
        if (_parent != nullptr) {
            _parent->RemoveChild(this);
        }
    }

  private:
    using ChildIterator = std::list<ComponentPtr>::iterator;

    ChildIterator FindChild(
        const Component* child
    ) {
        return std::find_if(_children.begin(), _children.end(), [child](const ComponentPtr& item) { return item.get() == child; });
    }

  private:
    Component*              _parent;
    std::list<ComponentPtr> _children;

  public:
    // Comment: 组件的左上角是渲染原点，里面的渲染就不用带入 GetPosition 的值了。
    virtual bool Render(Direct2D::Graphics&) = 0;

    virtual void CreateResources(Direct2D::Graphics& graphics) = 0;
    virtual void DiscardResources()                            = 0;

    HWND GetRenderTargetWindow() {
        if (!_graphicsRef.IsValid()) {
            return nullptr;
        }

        return _graphicsRef.GetPointer()->GetHwnd();
    }

    virtual D2D1_POINT_2F GetPosition() const { return _position; };
    virtual D2D1_SIZE_F   GetSize() const { return _size; };

    virtual void Invalidate(
        const D2D1_RECT_F& rect
    ) {
        if (_hasDirtyRect) {
            _dirtyRect.left   = (std::min)(_dirtyRect.left, rect.left);
            _dirtyRect.top    = (std::min)(_dirtyRect.top, rect.top);
            _dirtyRect.right  = (std::max)(_dirtyRect.right, rect.right);
            _dirtyRect.bottom = (std::max)(_dirtyRect.bottom, rect.bottom);
        } else {
            _dirtyRect    = rect;
            _hasDirtyRect = true;
        }

        _isDirty = true;
        if (_parent != nullptr) {
            _parent->Invalidate(rect);
        }

        // Then, window component will override this function to notify the real window.
    }

    void InvalidateSelf() { Invalidate(Direct2D::PointAndSizeFToRectF(GetPosition(), GetSize())); }

  private:
    friend class Components::Window;
    friend class Direct2D_UI::BaseWindow;
    void UpdateRenderTarget(
        _In_ Direct2D::Graphics& source
    ) {
        _graphicsRef.Discard();
        DiscardResources();

        source.AddRef(_graphicsRef);
        CreateResources(_graphicsRef);

        for (const auto& child : _children) {
            child->UpdateRenderTarget(source);
        }
    }

    Direct2D::Graphics _graphicsRef;
    float_t            _renderDpi;
    D2D1_POINT_2F      _position;
    D2D1_SIZE_F        _size;

    bool        _isDirty;
    bool        _hasDirtyRect;
    D2D1_RECT_F _dirtyRect;

    // Event operation

    void PreRender(
        const ComponentRenderEvent* event
    ) {
        const auto rect = D2D1::RectF(0.f, 0.f, GetSize().width, GetSize().height);
        //_graphicsRef.PushAxisAlignedClip(rect);
        _graphicsRef.SetTransform(D2D1::Matrix3x2F::Translation(GetSize()));

        Render(_graphicsRef);

        if (_isDirty) {
            _isDirty      = false;
            _hasDirtyRect = false;
            _dirtyRect    = {};
        };

        for (const auto& child : _children) {
            child->MessageProcedure(event);
        }

        _graphicsRef.SetTransform(D2D1::Matrix3x2F::Identity());
        //_graphicsRef.PopAxisAlignedClip();
    }

    void MessageProcedure(
        const ComponentEvent* event
    ) {
        if (event == nullptr) {
            return;
        }

        if (const auto* renderEvent = (const ComponentRenderEvent*)event; event->type == EventTypes::RenderEvent) {
            PreRender(renderEvent);
            return;
        }
    }
};
} // namespace Direct2D_UI::Components