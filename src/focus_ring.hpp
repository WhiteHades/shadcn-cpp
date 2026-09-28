// SPDX-License-Identifier: MIT
#pragma once

// Internal focus indicator shared by every component that suppresses the platform's own
// focus painting. Not a public interface: include this from src/ only.
//
// A control that calls setFlat(true) or paints `border: 0` removes Qt's focus frame. If
// nothing replaces it the control is keyboard reachable and announces nothing on focus,
// which is worse than a plain focus ring. Attach one of these instead.

#include <QEvent>
#include <QFocusFrame>
#include <QKeyEvent>
#include <QApplication>
#include <QPainter>
#include <QPointer>
#include <QWidget>

#include <algorithm>
#include <cmath>

#include <shadcn/controls.hpp>

namespace shadcn::detail {

// The theme lookup and colour conversion are private to widgets.cpp. They are repeated
// here rather than promoted to a public header, because the ring is the only thing outside
// that translation unit that needs them.
inline const Theme& ringTheme(const QWidget& widget) {
    if (const auto* current = qobject_cast<const Style*>(widget.style())) return current->theme();
    if (const auto* current = qobject_cast<const Style*>(QApplication::style())) return current->theme();
    static const auto fallback = Theme::neutral();
    return fallback;
}
inline QColor ringColor(const Theme& theme, Role role) {
    const auto c = theme.color(role);
    return QColor::fromRgbF(static_cast<float>(c.r), static_cast<float>(c.g),
                            static_cast<float>(c.b), static_cast<float>(c.a));
}
inline QColor ringAlpha(QColor c, double factor) {
    c.setAlphaF(static_cast<float>(std::clamp(static_cast<double>(c.alphaF()) * factor, 0.0, 1.0)));
    return c;
}

inline double focusRadiusFor(const QWidget& widget) {
    const auto fixed = widget.property("shadcnRadius");
    return fixed.isValid() ? fixed.toDouble() : ringTheme(widget).radius();
}

/// A ring that follows its target and paints just outside the target's bounds. It shows on
/// keyboard focus and not on pointer focus, so a mouse user does not see it. Pass
/// textInput when the ring should also be visible while the field has focus by pointer,
/// which is what a text field needs to show where typing will land.
class FocusRing final : public QFocusFrame {
public:
    explicit FocusRing(QWidget& target, bool textInput = false)
        : QFocusFrame(&target), target_(&target), always_(textInput) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFocusPolicy(Qt::NoFocus);
        setWidget(&target);
        target.installEventFilter(this);
        QObject::connect(&target, &QObject::destroyed, this, &QObject::deleteLater);
        hide();
    }

    [[nodiscard]] bool tracksKeyboardFocusOnly() const noexcept { return !always_; }

protected:
    bool eventFilter(QObject* object, QEvent* event) override {
        const auto result = QFocusFrame::eventFilter(object, event);
        if (!target_ || object != target_) return result;
        if (event->type() == QEvent::ParentChange || event->type() == QEvent::StyleChange) {
            // A widget may have been parentless when the component was constructed. Rebind
            // after parenting or style changes, and keep observing an unsupported top-level
            // target so a later move into a layout starts tracking again.
            setWidget(nullptr);
            setWidget(target_.data());
            target_->installEventFilter(this);
        }
        if (event->type() == QEvent::FocusIn)
            keyboard_ = static_cast<QFocusEvent*>(event)->reason() != Qt::MouseFocusReason;
        if (event->type() == QEvent::DynamicPropertyChange) {
            const auto name = static_cast<QDynamicPropertyChangeEvent*>(event)->propertyName();
            if (name != "shadcnInvalid" && name != "shadcnDestructive") return result;
        }
        switch (event->type()) {
        case QEvent::DynamicPropertyChange:
        case QEvent::FocusIn: case QEvent::FocusOut: case QEvent::Show: case QEvent::Hide:
        case QEvent::Move: case QEvent::Resize: case QEvent::EnabledChange:
        case QEvent::StyleChange: case QEvent::PaletteChange: case QEvent::ParentChange: {
            const auto visible = target_->hasFocus() && target_->isVisible() && target_->isEnabled()
                                 && (always_ || keyboard_);
            target_->setProperty("shadcnFocusVisible", visible);
            const auto invalid = target_->property("shadcnInvalid").toBool();
            setVisible((visible || invalid) && target_->isVisible()
                       && widget() == target_.data());
            target_->update();
            update();
            break;
        }
        default: break;
        }
        return result;
    }

    void paintEvent(QPaintEvent*) override {
        if (!target_ || widget() != target_.data() || !parentWidget()) return;
        QPainter painter(this);
        if (!target_->isEnabled()) painter.setOpacity(.5);
        painter.setRenderHint(QPainter::Antialiasing);
        const auto origin = target_->mapTo(parentWidget(), QPoint(0, 0)) - pos();
        auto bounds = QRectF(QPointF(origin), QSizeF(target_->size()));
        bounds.adjust(-.75, -.75, .75, .75);
        const auto radius = std::min(focusRadiusFor(*target_),
                                     std::min(target_->width(), target_->height()) / 2.0);
        const auto& theme = ringTheme(*target_);
        const auto destructive = target_->property("invalid").toBool() ||
                                 target_->property("shadcnDestructive").toBool();
        const auto pen = destructive
                ? ringAlpha(ringColor(theme, Role::Destructive),
                            theme.mode() == ColorMode::Dark ? .4 : .2)
                : ringAlpha(ringColor(theme, Role::Ring), .5);
        painter.setPen(QPen(pen, 1.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(bounds, radius + .75, radius + .75);
    }

private:
    QPointer<QWidget> target_;
    bool always_ = false;
    bool keyboard_ = false;
};

} // namespace shadcn::detail
