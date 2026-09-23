// SPDX-License-Identifier: MIT
// Design source: shadcn-ui/ui @ 98a1fe67b439324ddc857f47fbdce056600a4329.
#include <shadcn/widgets.hpp>

#include <QAccessible>
#include <QApplication>
#include <QColor>
#include <QVariant>
#include <QFocusEvent>
#include <QDynamicPropertyChangeEvent>
#include <QEasingCurve>
#include <QFocusFrame>
#include <QFontMetrics>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyleFactory>
#include <QThread>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <stdexcept>

static void initialiseFonts() { Q_INIT_RESOURCE(fonts); }

namespace shadcn {
namespace {
const Theme& themeFor(const QWidget& widget) {
    if (const auto* current = qobject_cast<const Style*>(widget.style())) return current->theme();
    if (const auto* current = qobject_cast<const Style*>(QApplication::style())) return current->theme();
    static const auto fallback = Theme::neutral();
    return fallback;
}
bool reduced(const QWidget& widget) {
    if (const auto* current = qobject_cast<const Style*>(widget.style()))
        return current->motion() == MotionPolicy::Reduced;
    if (const auto* current = qobject_cast<const Style*>(QApplication::style()))
        return current->motion() == MotionPolicy::Reduced;
    return false;
}
QColor color(const Theme& theme, Role role) {
    const auto c = theme.color(role);
    return QColor::fromRgbF(static_cast<float>(c.r), static_cast<float>(c.g),
                           static_cast<float>(c.b), static_cast<float>(c.a));
}
QColor alpha(QColor c, double factor) {
    c.setAlphaF(static_cast<float>(std::clamp(static_cast<double>(c.alphaF()) * factor, 0.0, 1.0)));
    return c;
}
QColor blend(const QColor& a, const QColor& b, double t) {
    // Premultiplied-alpha interpolation prevents a dark fringe when fading transparency.
    const auto opacity = a.alphaF() * (1 - t) + b.alphaF() * t;
    if (opacity <= 0) return QColor(Qt::transparent);
    const auto channel = [=](double av, double bv) {
        return static_cast<float>((av * a.alphaF() * (1 - t) + bv * b.alphaF() * t) / opacity);
    };
    return QColor::fromRgbF(channel(a.redF(), b.redF()), channel(a.greenF(), b.greenF()),
                           channel(a.blueF(), b.blueF()), static_cast<float>(opacity));
}
double radiusFor(const QWidget& widget) {
    const auto fixed = widget.property("shadcnRadius");
    return fixed.isValid() ? fixed.toDouble() : themeFor(widget).radius();
}
bool focusVisible(const QWidget& widget) {
    return widget.hasFocus() && widget.property("shadcnFocusVisible").toBool();
}
void rounded(QPainter& painter, const QRectF& rect, double radius,
             const QColor& fill, const QColor& border = QColor(Qt::transparent)) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(fill);
    if (border.alpha() == 0) painter.setPen(Qt::NoPen);
    else painter.setPen(QPen(border, 1));
    const auto bounded = std::min(radius, std::min(rect.width(), rect.height()) / 2);
    painter.drawRoundedRect(rect, bounded, bounded);
}
struct Appearance { QColor fill; QColor text; QColor border; };
Appearance appearance(const Theme& theme, Variant variant, double hover, bool badge = false) {
    const auto c = [&](Role role) { return color(theme, role); };
    const auto dark = theme.mode() == ColorMode::Dark;
    const QColor clear(Qt::transparent);
    switch (variant) {
    case Variant::Default:
        return {alpha(c(Role::Primary), 1 - .2 * hover), c(Role::PrimaryForeground), clear};
    case Variant::Destructive:
        return {alpha(c(Role::Destructive), (dark ? .2 : .1) + .1 * hover), c(Role::Destructive), clear};
    case Variant::Secondary:
        return {badge ? alpha(c(Role::Secondary), 1 - .2 * hover)
                      : blend(c(Role::Secondary), c(Role::Foreground), .05 * hover), c(Role::SecondaryForeground), clear};
    case Variant::Outline:
        if (badge) return {clear, c(Role::Foreground), c(Role::Border)};
        return {dark ? alpha(c(Role::Input), .3 + .2 * hover)
                     : blend(c(Role::Background), c(Role::Accent), hover),
                c(Role::Foreground), c(dark ? Role::Input : Role::Border)};
    case Variant::Ghost:
        return {alpha(c(Role::Accent), hover * (dark ? .5 : 1)), c(Role::Foreground), clear};
    case Variant::Link:
        return {clear, c(Role::Primary), clear};
    }
    return {c(Role::Primary), c(Role::PrimaryForeground), clear};
}
QColor focusColor(const QWidget& widget) {
    const auto& theme = themeFor(widget);
    const auto destructive = widget.property("invalid").toBool() ||
                             widget.property("shadcnDestructive").toBool();
    return alpha(color(theme, destructive ? Role::Destructive : Role::Ring),
                 destructive ? (theme.mode() == ColorMode::Dark ? .4 : .2) : .5);
}

// QFocusFrame follows its target and paints beyond the target's bounds.
// QPointer protects the observer if the target is destroyed before a queued event.
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
protected:
    bool eventFilter(QObject* object, QEvent* event) override {
        const auto result = QFocusFrame::eventFilter(object, event);
        if (!target_ || object != target_) return result;
        if (event->type() == QEvent::ParentChange || event->type() == QEvent::StyleChange) {
            // A widget may have been parentless when the component was constructed.
            // Rebind after parenting or style changes. Keep observing unsupported
            // top-level targets so a later move into a layout starts tracking again.
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
        bounds.adjust(-1.5, -1.5, 1.5, 1.5);
        const auto radius = std::min(radiusFor(*target_),
                                    std::min(target_->width(), target_->height()) / 2.0);
        painter.setPen(QPen(focusColor(*target_), 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(bounds, radius + 1.5, radius + 1.5);
    }
private:
    QPointer<QWidget> target_;
    bool always_ = false;
    bool keyboard_ = false;
};

void animate(QVariantAnimation& animation, QWidget& widget, double current, double target) {
    animation.stop();
    animation.setStartValue(current);
    animation.setEndValue(target);
    animation.setDuration(reduced(widget) || !widget.isVisible() ? 0 : 150);
    animation.start();
}
void setTiming(QVariantAnimation& animation) {
    QEasingCurve curve;
    curve.setCustomType(&transition_easing);
    animation.setEasingCurve(curve);
}
QFont buttonFont(const QWidget& widget, ButtonSize size) {
    auto font = widget.font();
    font.setWeight(QFont::Medium);
    const auto metrics = button_metrics(size);
    if (metrics.fontSize != 14) {
        if (font.pixelSize() > 0) font.setPixelSize(std::max(1, qRound(font.pixelSize() * metrics.fontSize / 14)));
        else font.setPointSizeF(std::max(1.0, font.pointSizeF() * metrics.fontSize / 14));
    }
    return font;
}
void setInvalidProperty(QWidget& widget, bool invalid) {
    widget.setProperty("shadcnInvalid", invalid);
    widget.update();
    // The external focus ring is a sibling after QFocusFrame reparents it.
    if (auto* parent = widget.parentWidget()) parent->update();
}
} // namespace

Style::Style(Theme theme, MotionPolicy motion)
    : QProxyStyle(QStyleFactory::create("Fusion")), theme_(std::move(theme)), motion_(motion) {}
int Style::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const {
    if (metric == PM_FocusFrameHMargin || metric == PM_FocusFrameVMargin) return 3;
    return QProxyStyle::pixelMetric(metric, option, widget);
}
int Style::styleHint(StyleHint hint, const QStyleOption* option,
                     const QWidget* widget, QStyleHintReturn* data) const {
    if (hint == SH_FocusFrame_AboveWidget) return 1;
    if (hint == SH_FocusFrame_Mask) return 0;
    return QProxyStyle::styleHint(hint, option, widget, data);
}
void install(QApplication& app, Theme theme, MotionPolicy motion, int fontPixels) {
    if (QThread::currentThread() != app.thread())
        throw std::logic_error("shadcn::install must run on the GUI thread");
    auto palette = app.palette();
    palette.setColor(QPalette::Window, color(theme, Role::Background));
    palette.setColor(QPalette::WindowText, color(theme, Role::Foreground));
    palette.setColor(QPalette::Base, color(theme, Role::Background));
    palette.setColor(QPalette::Text, color(theme, Role::Foreground));
    palette.setColor(QPalette::Button, color(theme, Role::Secondary));
    palette.setColor(QPalette::ButtonText, color(theme, Role::SecondaryForeground));
    palette.setColor(QPalette::Highlight, color(theme, Role::Primary));
    palette.setColor(QPalette::HighlightedText, color(theme, Role::PrimaryForeground));
    palette.setColor(QPalette::PlaceholderText, color(theme, Role::MutedForeground));
    app.setStyle(new Style(std::move(theme), motion));
    app.setPalette(palette);
    if (fontPixels > 0) {
        static const int fontId = [] {
            initialiseFonts();
            return QFontDatabase::addApplicationFont(QStringLiteral(":/shadcn/Geist.ttf"));
        }();
        auto font = app.font();
        if (fontId >= 0) {
            const auto families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) font.setFamily(families.first());
        }
        font.setPixelSize(fontPixels);
        app.setFont(font);
    }
}

Button::Button(const QString& text, QWidget* parent)
    : QPushButton(text, parent), hover_(new QVariantAnimation(this)) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setAutoDefault(false);
    setTiming(*hover_);
    connect(hover_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        hoverAmount_ = value.toDouble();
        update();
    });
    new FocusRing(*this);
}
void Button::setVariant(Variant variant) {
    if (variant_ == variant) return;
    variant_ = variant;
    setProperty("shadcnDestructive", variant == Variant::Destructive);
    update();
}
void Button::setButtonSize(ButtonSize size) {
    if (size_ == size) return;
    size_ = size;
    updateGeometry();
    update();
}
void Button::setInvalid(bool invalid) {
    if (invalid_ == invalid) return;
    invalid_ = invalid;
    setInvalidProperty(*this, invalid);
}
QSize Button::sizeHint() const {
    const auto m = button_metrics(size_);
    const QFontMetrics fm(buttonFont(*this, size_));
    const auto height = std::max(static_cast<int>(m.height), fm.height() + 4);
    if (m.iconOnly) return {height, height};
    const auto hasIcon = !icon().isNull();
    const auto padding = hasIcon ? m.iconPadding + m.padding : m.padding * 2;
    const auto textWidth = fm.size(Qt::TextShowMnemonic, text()).width();
    const auto content = textWidth + (hasIcon ? static_cast<int>(m.iconSize + (text().isEmpty() ? 0 : m.gap)) : 0);
    return {content + static_cast<int>(padding) + 2, height};
}
QSize Button::minimumSizeHint() const { return sizeHint(); }
void Button::updateHover() {
    animate(*hover_, *this, hoverAmount_, isEnabled() && underMouse() ? 1 : 0);
}
bool Button::event(QEvent* event) {
    const auto result = QPushButton::event(event);
    switch (event->type()) {
    case QEvent::Enter: case QEvent::Leave: case QEvent::EnabledChange:
    case QEvent::StyleChange: case QEvent::Hide: updateHover(); break;
    default: break;
    }
    return result;
}
void Button::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (!event->isAutoRepeat()) click();
        event->accept();
        return;
    }
    QPushButton::keyPressEvent(event);
}
void Button::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    if (isDown() && !menu()) painter.translate(0, 1);
    const auto& theme = themeFor(*this);
    auto look = appearance(theme, variant_, hoverAmount_);
    if (invalid_) look.border = color(theme, Role::Destructive);
    else if (focusVisible(*this)) look.border = variant_ == Variant::Destructive
        ? alpha(color(theme, Role::Destructive), .4) : color(theme, Role::Ring);
    if (!isEnabled()) painter.setOpacity(.5);
    const auto radius = size_ == ButtonSize::Xs || size_ == ButtonSize::Sm ||
                        size_ == ButtonSize::IconXs || size_ == ButtonSize::IconSm
        ? theme.radius() * .8 : radiusFor(*this);
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius, look.fill, look.border);
    const auto m = button_metrics(size_);
    const auto font = buttonFont(*this, size_);
    painter.setFont(font);
    painter.setPen(look.text);
    const QFontMetrics fm(font);
    const auto visibleText = m.iconOnly ? QString{} : text();
    const auto textWidth = fm.size(Qt::TextShowMnemonic, visibleText).width();
    const auto iconWidth = icon().isNull() ? 0 : static_cast<int>(m.iconSize);
    const auto gap = iconWidth && !visibleText.isEmpty() ? static_cast<int>(m.gap) : 0;
    const auto total = textWidth + iconWidth + gap;
    const auto start = (width() - total) / 2;
    const auto rtl = layoutDirection() == Qt::RightToLeft;
    if (iconWidth) {
        const auto x = rtl ? start + textWidth + gap : start;
        icon().paint(&painter, QRect(x, (height() - iconWidth) / 2, iconWidth, iconWidth),
                     Qt::AlignCenter, isEnabled() ? QIcon::Normal : QIcon::Disabled,
                     isChecked() ? QIcon::On : QIcon::Off);
    }
    const auto x = rtl ? start : start + iconWidth + gap;
    const QRect textRect(x, 0, textWidth, height());
    painter.drawText(textRect, Qt::AlignCenter | Qt::TextShowMnemonic, visibleText);
    if (variant_ == Variant::Link && hoverAmount_ > .5 && !visibleText.isEmpty()) {
        const auto y = (height() - fm.height()) / 2 + fm.ascent() + 4;
        painter.drawLine(x, y, x + textWidth, y);
    }
}

Input::Input(QWidget* parent) : QLineEdit(parent) {
    setFrame(false);
    setStyleSheet("QLineEdit { background: transparent; border: none; }");
    setTextMargins(10, 4, 10, 4);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    updatePalette();
    new FocusRing(*this, true);
}
void Input::updatePalette() {
    const auto& theme = themeFor(*this);
    auto p = palette();
    p.setColor(QPalette::Base, Qt::transparent);
    p.setColor(QPalette::Text, color(theme, Role::Foreground));
    p.setColor(QPalette::Disabled, QPalette::Text, alpha(color(theme, Role::Foreground), .5));
    p.setColor(QPalette::PlaceholderText, color(theme, Role::MutedForeground));
    p.setColor(QPalette::Highlight, color(theme, Role::Primary));
    p.setColor(QPalette::HighlightedText, color(theme, Role::PrimaryForeground));
    setPalette(p);
}
void Input::setInvalid(bool invalid) {
    if (invalid_ == invalid) return;
    invalid_ = invalid;
    setInvalidProperty(*this, invalid);
}
void Input::setError(const QString& message) {
    setInvalid(!message.isEmpty());
    setAccessibleDescription(message);
    QAccessibleEvent event(this, QAccessible::DescriptionChanged);
    QAccessible::updateAccessibility(&event);
}
QSize Input::sizeHint() const { return {240, std::max(32, fontMetrics().height() + 10)}; }
QSize Input::minimumSizeHint() const { return {24, sizeHint().height()}; }
void Input::changeEvent(QEvent* event) {
    QLineEdit::changeEvent(event);
    if (event->type() == QEvent::StyleChange) updatePalette();
    if (event->type() == QEvent::FontChange) updateGeometry();
}
void Input::paintEvent(QPaintEvent* event) {
    if (!property("shadcnEmbedded").toBool()) {
        QPainter painter(this);
        const auto& theme = themeFor(*this);
        auto border = color(theme, invalid_ ? Role::Destructive : hasFocus() ? Role::Ring : Role::Input);
        auto fill = theme.mode() == ColorMode::Dark ? alpha(color(theme, Role::Input), .3)
                                                   : QColor(Qt::transparent);
        if (!isEnabled()) painter.setOpacity(.5);
        rounded(painter, QRectF(rect()).adjusted(.5,.5,-.5,-.5), radiusFor(*this), fill, border);
    }
    QLineEdit::paintEvent(event);
}

Badge::Badge(const QString& text, QWidget* parent) : QLabel(text, parent) {
    setTextFormat(Qt::PlainText);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    auto f = font(); f.setPixelSize(12); f.setWeight(QFont::Medium); setFont(f);
}
void Badge::setVariant(Variant variant) { variant_ = variant; update(); }
QSize Badge::sizeHint() const { return {fontMetrics().horizontalAdvance(text()) + 18, std::max(20, fontMetrics().height() + 2)}; }
QSize Badge::minimumSizeHint() const { return sizeHint(); }
void Badge::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const auto look = appearance(themeFor(*this), variant_, 0, true);
    if (!isEnabled()) painter.setOpacity(.5);
    rounded(painter, QRectF(rect()).adjusted(.5,.5,-.5,-.5), height() / 2.0, look.fill, look.border);
    painter.setFont(font()); painter.setPen(look.text);
    painter.drawText(rect().adjusted(9, 3, -9, -3), Qt::AlignCenter, text());
}

Label::Label(const QString& text, QWidget* parent) : QLabel(text, parent) {
    setTextFormat(Qt::PlainText);
    auto f = font(); f.setWeight(QFont::Medium); setFont(f);
}
void Label::setBuddy(QWidget* buddy) {
    if (buddy_) buddy_->removeEventFilter(this);
    buddy_ = buddy;
    QLabel::setBuddy(buddy);
    if (buddy_) {
        buddy_->installEventFilter(this);
        setEnabled(buddy_->isEnabled());
    } else setEnabled(true);
}
bool Label::eventFilter(QObject* object, QEvent* event) {
    if (object == buddy_ && event->type() == QEvent::EnabledChange) setEnabled(buddy_->isEnabled());
    return QLabel::eventFilter(object, event);
}
void Label::mousePressEvent(QMouseEvent* event) {
    pressed_ = event->button() == Qt::LeftButton;
    if (pressed_) event->accept();
    else QLabel::mousePressEvent(event);
}
void Label::mouseReleaseEvent(QMouseEvent* event) {
    const auto activate = std::exchange(pressed_, false);
    if (activate && event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()) &&
        buddy_ && buddy_->isEnabled()) {
        buddy_->setFocus(Qt::MouseFocusReason);
        if (auto* button = qobject_cast<QAbstractButton*>(buddy_.data())) button->click();
        event->accept();
        return;
    }
    QLabel::mouseReleaseEvent(event);
}

Checkbox::Checkbox(const QString& text, QWidget* parent) : QCheckBox(text, parent) {
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setProperty("shadcnRadius", 4.0);
    new FocusRing(*this);
}
void Checkbox::setInvalid(bool invalid) {
    invalid_ = invalid;
    setInvalidProperty(*this, invalid);
}
QSize Checkbox::sizeHint() const {
    return {16 + (text().isEmpty() ? 0 : 8 + fontMetrics().size(Qt::TextShowMnemonic, text()).width()),
            text().isEmpty() ? 16 : std::max(16, fontMetrics().height())};
}
QSize Checkbox::minimumSizeHint() const { return sizeHint(); }
bool Checkbox::hitButton(const QPoint& position) const { return rect().contains(position); }
void Checkbox::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    if (!isEnabled()) painter.setOpacity(.5);
    const auto& theme = themeFor(*this);
    const auto checked = checkState() != Qt::Unchecked;
    auto fill = checked ? color(theme, Role::Primary)
                        : theme.mode() == ColorMode::Dark ? alpha(color(theme, Role::Input), .3)
                                                         : QColor(Qt::transparent);
    auto border = color(theme, invalid_ ? Role::Destructive : focusVisible(*this) ? Role::Ring
                                    : checked ? Role::Primary : Role::Input);
    const auto rtl = layoutDirection() == Qt::RightToLeft;
    const auto x = rtl ? width() - 16 : 0;
    const auto y = (height() - 16) / 2.0;
    rounded(painter, QRectF(x + .5, y + .5, 15, 15), 4, fill, border);
    if (checked) {
        painter.setPen(QPen(color(theme, Role::PrimaryForeground), 1.75, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        QPainterPath mark;
        if (checkState() == Qt::PartiallyChecked) {
            mark.moveTo(x + 4, y + 8); mark.lineTo(x + 12, y + 8);
        } else {
            mark.moveTo(x + 4, y + 8); mark.lineTo(x + 7, y + 11); mark.lineTo(x + 12, y + 5);
        }
        painter.drawPath(mark);
    }
    if (!text().isEmpty()) {
        painter.setPen(color(theme, Role::Foreground));
        painter.drawText(rtl ? rect().adjusted(0, 0, -24, 0) : rect().adjusted(24, 0, 0, 0),
                         static_cast<int>(Qt::AlignVCenter | (rtl ? Qt::AlignRight : Qt::AlignLeft) |
                                          Qt::TextShowMnemonic), text());
    }
}

Switch::Switch(QWidget* parent) : QCheckBox(parent), transition_(new QVariantAnimation(this)) {
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setTristate(false);
    setProperty("shadcnRadius", 1000.0);
    setTiming(*transition_);
    connect(transition_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        position_ = value.toDouble(); update();
    });
    connect(this, &QCheckBox::toggled, this, [this] { updatePosition(); });
    new FocusRing(*this);
}
void Switch::setSwitchSize(SwitchSize size) { size_ = size; updateGeometry(); update(); }
QSize Switch::sizeHint() const { return size_ == SwitchSize::Sm ? QSize(24, 14) : QSize(32, 19); }
QSize Switch::minimumSizeHint() const { return sizeHint(); }
void Switch::updatePosition() { animate(*transition_, *this, position_, isChecked() ? 1 : 0); }
bool Switch::event(QEvent* event) {
    const auto result = QCheckBox::event(event);
    if (event->type() == QEvent::Hide || event->type() == QEvent::StyleChange) updatePosition();
    return result;
}
void Switch::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (!event->isAutoRepeat()) click();
        event->accept(); return;
    }
    QCheckBox::keyPressEvent(event);
}
bool Switch::hitButton(const QPoint& position) const { return rect().contains(position); }
void Switch::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    if (!isEnabled()) painter.setOpacity(.5);
    const auto& theme = themeFor(*this);
    const auto dark = theme.mode() == ColorMode::Dark;
    const auto h = size_ == SwitchSize::Sm ? 14.0 : 18.4;
    const auto w = size_ == SwitchSize::Sm ? 24.0 : 32.0;
    const auto thumb = size_ == SwitchSize::Sm ? 12.0 : 16.0;
    const auto left = (width() - w) / 2.0;
    const auto top = (height() - h) / 2.0;
    const auto unchecked = alpha(color(theme, Role::Input), dark ? .8 : 1);
    const auto fill = blend(unchecked, color(theme, Role::Primary), position_);
    rounded(painter, QRectF(left + .5, top + .5, w - 1, h - 1), h / 2, fill,
            focusVisible(*this) ? color(theme, Role::Ring) : QColor(Qt::transparent));
    const auto fraction = layoutDirection() == Qt::RightToLeft ? 1 - position_ : position_;
    const auto thumbFill = dark ? blend(color(theme, Role::Foreground), color(theme, Role::PrimaryForeground), position_)
                                : color(theme, Role::Background);
    rounded(painter, QRectF(left + 1 + fraction * (w - thumb - 2), top + (h - thumb) / 2, thumb, thumb), thumb / 2, thumbFill);
}

Separator::Separator(Qt::Orientation orientation, QWidget* parent) : QFrame(parent), orientation_(orientation) {
    setOrientation(orientation);
    setFocusPolicy(Qt::NoFocus);
}
void Separator::setOrientation(Qt::Orientation orientation) {
    orientation_ = orientation;
    setSizePolicy(orientation == Qt::Horizontal ? QSizePolicy::Expanding : QSizePolicy::Fixed,
                  orientation == Qt::Horizontal ? QSizePolicy::Fixed : QSizePolicy::Expanding);
    updateGeometry(); update();
}
QSize Separator::sizeHint() const { return orientation_ == Qt::Horizontal ? QSize(100, 1) : QSize(1, 100); }
void Separator::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), color(themeFor(*this), Role::Border));
}

Progress::Progress(QWidget* parent) : QProgressBar(parent), transition_(new QVariantAnimation(this)) {
    setTextVisible(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setTiming(*transition_);
    connect(transition_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        fraction_ = value.toDouble(); update();
    });
    connect(this, &QProgressBar::valueChanged, this, [this] { updateFraction(); });
    QProgressBar::setValue(0);
}
QSize Progress::sizeHint() const { return {160, 4}; }
QSize Progress::minimumSizeHint() const { return {0, 4}; }
void Progress::setRange(int minimum, int maximum) { QProgressBar::setRange(minimum, maximum); updateFraction(); }
void Progress::setMinimum(int minimum) { QProgressBar::setMinimum(minimum); updateFraction(); }
void Progress::setMaximum(int maximum) { QProgressBar::setMaximum(maximum); updateFraction(); }
void Progress::updateFraction() {
    target_ = progress_fraction(value(), minimum(), maximum()).value_or(0);
    animate(*transition_, *this, fraction_, target_);
}
bool Progress::event(QEvent* event) {
    const auto result = QProgressBar::event(event);
    if (event->type() == QEvent::StyleChange || event->type() == QEvent::Hide) updateFraction();
    return result;
}
void Progress::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const auto& theme = themeFor(*this);
    const auto bounds = QRectF(rect());
    rounded(painter, bounds, height() / 2.0, color(theme, Role::Muted));
    QPainterPath clip; clip.addRoundedRect(bounds, height() / 2.0, height() / 2.0);
    painter.setClipPath(clip);
    const auto actual = progress_fraction(value(), minimum(), maximum()).value_or(0);
    const auto current = actual == target_ ? fraction_ : actual;
    const auto amount = width() * std::clamp(current, 0.0, 1.0);
    const auto rtl = (layoutDirection() == Qt::RightToLeft) != invertedAppearance();
    painter.fillRect(QRectF(rtl ? width() - amount : 0, 0, amount, height()), color(theme, Role::Primary));
}

Skeleton::Skeleton(QWidget* parent) : QWidget(parent), pulse_(new QVariantAnimation(this)) {
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    pulse_->setStartValue(0.0); pulse_->setEndValue(2.0);
    pulse_->setDuration(2000); pulse_->setLoopCount(-1);
    connect(pulse_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        opacity_ = pulse_opacity(value.toDouble()); update();
    });
}
QSize Skeleton::sizeHint() const { return {160, 20}; }
void Skeleton::updateAnimation() {
    if (isVisible() && isEnabled() && !reduced(*this)) {
        if (pulse_->state() != QAbstractAnimation::Running) pulse_->start();
    } else {
        pulse_->stop(); opacity_ = 1; update();
    }
}
bool Skeleton::event(QEvent* event) {
    const auto result = QWidget::event(event);
    switch (event->type()) {
    case QEvent::Show: case QEvent::Hide: case QEvent::EnabledChange: case QEvent::StyleChange:
        updateAnimation(); break;
    default: break;
    }
    return result;
}
void Skeleton::paintEvent(QPaintEvent*) {
    QPainter painter(this); painter.setOpacity(opacity_);
    rounded(painter, QRectF(rect()), themeFor(*this).radius() * .8, color(themeFor(*this), Role::Muted));
}

Card::Card(QWidget* parent) : QFrame(parent) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 16, 0, 16); outer->setSpacing(16);
    header_ = new QWidget(this);
    auto* headerLayout = new QGridLayout(header_);
    headerLayout->setContentsMargins(16, 0, 16, 0); headerLayout->setSpacing(4);
    headerLayout->setColumnStretch(0, 1);
    title_ = new QLabel(header_); title_->setTextFormat(Qt::PlainText); title_->setWordWrap(true);
    auto titleFont = font(); titleFont.setWeight(QFont::Medium); titleFont.setPixelSize(16); title_->setFont(titleFont);
    description_ = new QLabel(header_); description_->setTextFormat(Qt::PlainText); description_->setWordWrap(true);
    actionHost_ = new QWidget(header_); action_ = new QVBoxLayout(actionHost_); action_->setContentsMargins(0,0,0,0);
    headerLayout->addWidget(title_, 0, 0); headerLayout->addWidget(description_, 1, 0);
    headerLayout->addWidget(actionHost_, 0, 1, 2, 1, Qt::AlignTop);
    actionHost_->hide();
    outer->addWidget(header_); header_->hide();
    contentHost_ = new QWidget(this); content_ = new QVBoxLayout(contentHost_);
    content_->setContentsMargins(16,0,16,0); content_->setSpacing(12);
    outer->addWidget(contentHost_); contentHost_->hide();
    footerHost_ = new QWidget(this); footer_ = new QHBoxLayout(footerHost_);
    footer_->setContentsMargins(16,16,16,16); footer_->setSpacing(8);
    outer->addWidget(footerHost_); footerHost_->hide();
    updatePalette();
}
void Card::updatePalette() {
    const auto& theme = themeFor(*this);
    auto p = palette(); p.setColor(QPalette::WindowText, color(theme, Role::CardForeground)); setPalette(p);
    auto muted = description_->palette(); muted.setColor(QPalette::WindowText, color(theme, Role::MutedForeground));
    description_->setPalette(muted);
}
void Card::updateHeader() {
    title_->setVisible(!title_->text().isEmpty());
    description_->setVisible(!description_->text().isEmpty());
    header_->setVisible(!title_->text().isEmpty() || !description_->text().isEmpty() || !actionHost_->isHidden());
}
void Card::setTitle(const QString& title) { title_->setText(title); updateHeader(); }
void Card::setDescription(const QString& description) { description_->setText(description); updateHeader(); }
QVBoxLayout& Card::content() { contentHost_->show(); return *content_; }
QHBoxLayout& Card::footer() { footerHost_->show(); layout()->setContentsMargins(0,16,0,0); return *footer_; }
QVBoxLayout& Card::action() { actionHost_->show(); header_->show(); return *action_; }
void Card::changeEvent(QEvent* event) {
    QFrame::changeEvent(event);
    if (event->type() == QEvent::StyleChange) updatePalette();
}
void Card::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const auto& theme = themeFor(*this);
    rounded(painter, QRectF(rect()).adjusted(.5,.5,-.5,-.5), theme.radius() * 1.4,
            color(theme, Role::Card), color(theme, Role::Border));
    if (!footerHost_->isHidden()) {
        QPainterPath clip;
        clip.addRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5), theme.radius()*1.4, theme.radius()*1.4);
        painter.setClipPath(clip);
        painter.fillRect(footerHost_->geometry(), alpha(color(theme, Role::Muted), .5));
        painter.setPen(color(theme,Role::Border));
        painter.drawLine(0,footerHost_->y(),width(),footerHost_->y());
    }
}
} // namespace shadcn
