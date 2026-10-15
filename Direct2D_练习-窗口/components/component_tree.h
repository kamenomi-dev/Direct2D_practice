#pragma once
#include "../direct2d/render.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <vector>

namespace Direct2D_UI {
class BaseWindow;
}

namespace Direct2D_UI::Components {
class Window;
class Component;

using ComponentPtr = std::shared_ptr<Component>;

// ============================================================================
// 路由事件
// ============================================================================
enum class EventPhase : unsigned char {
    None = 0,
    Capture, // 从根向下：捕获 / 隧道
    Target,
    Bubble // 从目标向上：冒泡
};

enum class MouseButton : unsigned char {
    None = 0,
    Left,
    Right,
    Middle
};

struct RoutedEvent {
    EventPhase    phase    = EventPhase::None;
    bool          handled  = false;
    D2D1_POINT_2F position = {0, 0};
    MouseButton   button   = MouseButton::None;
    bool          isDown   = false;
    bool          isUp     = false;
    bool          isMove   = false;

    void StopPropagation() { handled = true; }
};

// ============================================================================
// 缓动 / 动画
// ============================================================================
namespace Easing {
inline float Linear(
    float t
) {
    return t;
}

inline float EaseInOutCubic(
    float t
) {
    return t < 0.5f ? 4.f * t * t * t : 1.f - std::pow(-2.f * t + 2.f, 3.f) / 2.f;
}
} // namespace Easing

// 把 0~1 的进度映射到组件属性上；由宿主窗口每帧 Tick
class Animation {
  public:
    using Clock    = std::chrono::steady_clock;
    using Function = std::function<float(float)>;
    using Apply    = std::function<void(float)>;

    Animation(
        Clock::time_point start, Clock::duration duration, Function easing, Apply apply
    )
    : _start(start), _duration(duration), _easing(std::move(easing)), _apply(std::move(apply)) {}

    // 返回 false 表示已播放结束
    bool Tick(
        Clock::time_point now
    ) {
        if (_finished) {
            return false;
        }

        const auto total    = std::chrono::duration<double>(_duration).count();
        const auto elapsed  = std::chrono::duration<double>(now - _start).count();
        const auto progress = total <= 0.0 ? 1.0 : std::clamp(elapsed / total, 0.0, 1.0);

        _apply(_easing((float)progress));
        _finished = progress >= 1.0;
        return !_finished;
    }

    bool IsFinished() const { return _finished; }

  private:
    Clock::time_point _start;
    Clock::duration   _duration;
    Function          _easing;
    Apply             _apply;
    bool              _finished = false;
};

// ============================================================================
// 组件：树（双链表子节点） + 事件 + 区域失效 + 动画
// ============================================================================
class Component {
  public:
    using Clock     = Animation::Clock;
    using ChildList = std::list<ComponentPtr>;

    Component(const Component&) = delete;
    Component(Component&&)      = delete;
    Component()                 = default;
    virtual ~Component() {
        for (auto& child : _children) {
            child->_parent = nullptr;
        }
    }

    // ------------------------------ 树 ------------------------------
    Component*       GetParent() const { return _parent; }
    const ChildList& GetChildren() const { return _children; }
    Component*       GetFirstChild() const { return _children.empty() ? nullptr : _children.front().get(); }
    Component*       GetLastChild() const { return _children.empty() ? nullptr : _children.back().get(); }
    Component*       GetPrevSibling() const { return SearchSibling(false); }
    Component*       GetNextSibling() const { return SearchSibling(true); }

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
        const ComponentPtr& child, Component* reference
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
        Invalidate();
    }

    bool RemoveChild(
        const Component* child
    ) {
        const auto position = FindChild(child);
        if (position == _children.end()) {
            return false;
        }

        (*position)->_parent = nullptr;
        _children.erase(position);
        Invalidate();
        return true;
    }

    void RemoveFromParent() {
        if (_parent != nullptr) {
            _parent->RemoveChild(this);
        }
    }

    void BringToFront() { MoveToEdge(true); }
    void SendToBack() { MoveToEdge(false); }

    // ------------------------------ 几何 / 命中 ------------------------------
    virtual D2D1_POINT_2F GetPosition() const { return _position; }
    virtual D2D1_SIZE_F   GetSize() const { return _size; }

    D2D1_RECT_F GetBounds() const {
        const auto position = GetPosition();
        const auto size     = GetSize();
        return {.left = position.x, .top = position.y, .right = position.x + size.width, .bottom = position.y + size.height};
    }

    void SetPosition(
        const D2D1_POINT_2F& position
    ) {
        _position = position;
        Invalidate();
    }

    void SetSize(
        const D2D1_SIZE_F& size
    ) {
        _size = size;
        Invalidate();
    }

    bool IsVisible() const { return _visible; }

    void SetVisible(
        bool visible
    ) {
        if (_visible != visible) {
            _visible = visible;
            Invalidate();
        }
    }

    virtual bool HitTest(
        const D2D1_POINT_2F& point
    ) const {
        const auto bounds = GetBounds();
        return point.x >= bounds.left && point.x < bounds.right && point.y >= bounds.top && point.y < bounds.bottom;
    }

    // 返回命中的最上层组件；子组件按链表倒序（后画的在上面）查找
    Component* HitTestRecursive(
        const D2D1_POINT_2F& point
    ) {
        if (!_visible || !HitTest(point)) {
            return nullptr;
        }

        for (auto it = _children.rbegin(); it != _children.rend(); ++it) {
            if (auto* hit = (*it)->HitTestRecursive(point)) {
                return hit;
            }
        }

        return this;
    }

    // ------------------------------ 事件 ------------------------------
    // 从 root 派发：capture（root -> target），再 bubble（target 的父级 -> root）
    static void RouteEvent(
        Component* root, RoutedEvent& event
    ) {
        if (root == nullptr) {
            return;
        }

        Component* target = root->HitTestRecursive(event.position);
        if (target == nullptr) {
            return;
        }

        std::vector<Component*> path;
        for (Component* node = target; node != nullptr; node = node->GetParent()) {
            path.push_back(node);
        }

        for (auto it = path.rbegin(); it != path.rend() && !event.handled; ++it) {
            event.phase = (*it == target) ? EventPhase::Target : EventPhase::Capture;
            (*it)->OnEvent(event);
        }

        for (auto index = (size_t)1; index < path.size() && !event.handled; ++index) {
            event.phase = EventPhase::Bubble;
            path[index]->OnEvent(event);
        }
    }

    // ------------------------------ 区域失效 ------------------------------
    using InvalidateNotifier = std::function<void(const D2D1_RECT_F&)>;

    void SetInvalidateNotifier(
        InvalidateNotifier notifier
    ) {
        _invalidateNotifier = std::move(notifier);
    }

    void Invalidate() { Invalidate(GetBounds()); }

    void Invalidate(
        const D2D1_RECT_F& rect
    ) {
        if (_hasDirtyRect) {
            _dirtyRect.left   = std::min(_dirtyRect.left, rect.left);
            _dirtyRect.top    = std::min(_dirtyRect.top, rect.top);
            _dirtyRect.right  = std::max(_dirtyRect.right, rect.right);
            _dirtyRect.bottom = std::max(_dirtyRect.bottom, rect.bottom);
        } else {
            _dirtyRect    = rect;
            _hasDirtyRect = true;
        }

        _dirty = true;

        // 一直向上冒泡到根，由根通过 notifier 通知宿主窗口
        if (_parent != nullptr) {
            _parent->Invalidate(rect);
        } else if (_invalidateNotifier) {
            _invalidateNotifier(_dirtyRect);
            _hasDirtyRect = false;
        }
    }

    // ------------------------------ 动画 ------------------------------
    void Animate(
        Clock::duration duration, Animation::Function easing, Animation::Apply apply
    ) {
        _animations.push_back(std::make_unique<Animation>(Clock::now(), duration, std::move(easing), std::move(apply)));
        Invalidate();
    }

    // 返回 true 表示树里还有动画在跑
    bool TickAnimations(
        Clock::time_point now
    ) {
        bool running = false;

        for (auto& animation : _animations) {
            if (animation->Tick(now)) {
                running = true;
            }
        }

        _animations.erase(
            std::remove_if(_animations.begin(), _animations.end(), [](const std::unique_ptr<Animation>& animation) { return animation->IsFinished(); }), _animations.end()
        );

        for (auto& child : _children) {
            running |= child->TickAnimations(now);
        }

        if (running) {
            Invalidate();
        }

        return running;
    }

    // ------------------------------ 资源 / 渲染 ------------------------------
    HWND GetRenderTargetWindow() {
        if (!_graphicsRef.IsValid()) {
            return nullptr;
        }

        return _graphicsRef.GetPointer()->GetHwnd();
    }

    virtual bool Render() = 0;

    virtual void CreateResources()  = 0;
    virtual void DiscardResources() = 0;

  protected:
    // 子类自绘完之后调用，递归绘制所有子组件（子组件盖在父组件之上）
    void RenderChildren() {
        for (const auto& child : _children) {
            child->Render();
        }
    }

    virtual void OnEvent(
        RoutedEvent& /*event*/
    ) {}

  protected:
    Direct2D::Graphics _graphicsRef;
    float_t            _renderDpi = 96.f;
    D2D1_POINT_2F      _position  = {0, 0};
    D2D1_SIZE_F        _size      = {0, 0};

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

        for (auto& child : _children) {
            child->UpdateRenderTarget(_graphicsRef);
        }
    }

    void UpdateRenderDpi(
        float dpi
    ) {
        _renderDpi = dpi;

        for (auto& child : _children) {
            child->UpdateRenderDpi(dpi);
        }
    }

    using ChildIterator = ChildList::iterator;

    ChildIterator FindChild(
        const Component* child
    ) {
        return std::find_if(_children.begin(), _children.end(), [child](const ComponentPtr& item) { return item.get() == child; });
    }

    Component* SearchSibling(
        bool forward
    ) const {
        if (_parent == nullptr) {
            return nullptr;
        }

        const auto& siblings = _parent->_children;
        const auto  position = std::find_if(siblings.begin(), siblings.end(), [this](const ComponentPtr& item) { return item.get() == this; });
        if (position == siblings.end()) {
            return nullptr;
        }

        if (forward) {
            const auto next = std::next(position);
            return next == siblings.end() ? nullptr : next->get();
        }

        return position == siblings.begin() ? nullptr : std::prev(position)->get();
    }

    void MoveToEdge(
        bool front
    ) {
        if (_parent == nullptr) {
            return;
        }

        auto&      siblings = _parent->_children;
        const auto position = _parent->FindChild(this);
        if (position == siblings.end()) {
            return;
        }

        // std::list 是双链表，splice 调整 z 序是 O(1)
        siblings.splice(front ? siblings.end() : siblings.begin(), siblings, position);
        _parent->Invalidate();
    }

  private:
    Component* _parent = nullptr;
    ChildList  _children;

    bool        _visible      = true;
    bool        _dirty        = false;
    bool        _hasDirtyRect = false;
    D2D1_RECT_F _dirtyRect    = {0, 0, 0, 0};

    InvalidateNotifier                      _invalidateNotifier;
    std::vector<std::unique_ptr<Animation>> _animations;
};

// ============================================================================
// 示例：纯色矩形组件
// ============================================================================
class BoxComponent : public Component {
  public:
    D2D1::ColorF Color = {D2D1::ColorF::CornflowerBlue, 1.f};

    bool Render() override {
        if (!_graphicsRef.IsValid()) {
            return false;
        }

        if (!_brush.IsValid()) {
            _graphicsRef.CreateSolidColorBrush(Color, _brush);
        }

        _brush.SetColor(Color);
        _graphicsRef.FillRectangle(GetBounds(), _brush);

        RenderChildren();
        return true;
    }

    void CreateResources() override { _graphicsRef.CreateSolidColorBrush(Color, _brush); }
    void DiscardResources() override { _brush.Discard(); }

  protected:
    Direct2D::SolidColorBrush _brush;
};

// ============================================================================
// 示例：以中心缩放 + 渐显的动画矩形，点击后播放一次
// ============================================================================
class AnimatedBoxComponent final : public BoxComponent {
  public:
    void Play() {
        _progress = 0.f;
        Animate(std::chrono::milliseconds(600), Easing::EaseInOutCubic, [this](float progress) { _progress = progress; });
    }

    bool Render() override {
        if (!_graphicsRef.IsValid()) {
            return false;
        }

        if (!_brush.IsValid()) {
            _graphicsRef.CreateSolidColorBrush(Color, _brush);
        }

        const auto bounds  = GetBounds();
        const auto centerX = (bounds.left + bounds.right) * 0.5f;
        const auto centerY = (bounds.top + bounds.bottom) * 0.5f;
        const auto halfW   = (bounds.right - bounds.left) * 0.5f * (0.5f + 0.5f * _progress);
        const auto halfH   = (bounds.bottom - bounds.top) * 0.5f * (0.5f + 0.5f * _progress);

        auto color = Color;
        color.a *= _progress;

        _brush.SetColor(color);
        _graphicsRef.FillRectangle({.left = centerX - halfW, .top = centerY - halfH, .right = centerX + halfW, .bottom = centerY + halfH}, _brush);

        RenderChildren();
        return true;
    }

  private:
    float _progress = 1.f;
};
} // namespace Direct2D_UI::Components
