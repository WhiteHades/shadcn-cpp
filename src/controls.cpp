// SPDX-License-Identifier: MIT
// Design source: shadcn-ui/ui local stock wrappers and style rules.
#include <shadcn/controls.hpp>

#include <QApplication>
#include <QAccessibleWidget>
#include <QAccessible>
#include <QBoxLayout>
#include <QFontMetrics>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QStyleOption>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace shadcn {
namespace {

const Theme& themeFor(const QWidget& widget) {
    if (const auto* style = qobject_cast<const Style*>(widget.style())) return style->theme();
    if (const auto* style = qobject_cast<const Style*>(QApplication::style())) return style->theme();
    static const auto fallback = Theme::neutral();
    return fallback;
}

bool reducedMotion(const QWidget& widget) {
    if (const auto* style = qobject_cast<const Style*>(widget.style()))
        return style->motion() == MotionPolicy::Reduced;
    if (const auto* style = qobject_cast<const Style*>(QApplication::style()))
        return style->motion() == MotionPolicy::Reduced;
    return false;
}

QColor colour(const QWidget& widget, Role role) {
    const auto value = themeFor(widget).color(role);
    return QColor::fromRgbF(static_cast<float>(value.r), static_cast<float>(value.g),
                            static_cast<float>(value.b), static_cast<float>(value.a));
}

QColor withAlpha(QColor value, double amount) {
    const auto factor = std::clamp(amount, 0.0, 1.0);
    value.setAlphaF(static_cast<float>(value.alphaF() * factor));
    return value;
}

double radius(const QWidget& widget) {
    const auto custom = widget.property("shadcnRadius");
    if (custom.isValid()) return custom.toDouble();
    return std::max(0.0, themeFor(widget).radius() - 2.0);
}

void rounded(QPainter& painter, QRectF rect, double cornerRadius, const QColor& fill,
             const QColor& border = QColor(Qt::transparent), double borderWidth = 1.0) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(fill);
    painter.setPen(border.alpha() == 0 ? Qt::NoPen : QPen(border, borderWidth));
    cornerRadius = std::min(cornerRadius, std::min(rect.width(), rect.height()) / 2.0);
    painter.drawRoundedRect(rect, cornerRadius, cornerRadius);
}

QColor foregroundFor(const QWidget& widget, bool destructive = false) {
    return colour(widget, destructive ? Role::Destructive : Role::Foreground);
}

QColor borderFor(const QWidget& widget, bool invalid = false) {
    return colour(widget, invalid ? Role::Destructive : Role::Border);
}

void paintFocus(QPainter& painter, const QWidget& widget, QRectF rect) {
    if (!widget.hasFocus() && !widget.isAncestorOf(QApplication::focusWidget())) return;
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(withAlpha(colour(widget, Role::Ring), .55), 2));
    painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), radius(widget) + 1, radius(widget) + 1);
}

void setInvalidProperty(QWidget& widget, bool invalid) {
    widget.setProperty("shadcnInvalid", invalid);
    widget.update();
    if (auto* parent = widget.parentWidget()) parent->update();
}

int toggleHeight(ToggleSize size) {
    switch (size) {
    case ToggleSize::Sm: return 28;
    case ToggleSize::Lg: return 36;
    case ToggleSize::Default: return 32;
    }
    return 36;
}

int togglePadding(ToggleSize size) {
    switch (size) {
    case ToggleSize::Sm: return 10;
    case ToggleSize::Lg: return 10;
    case ToggleSize::Default: return 10;
    }
    return 8;
}

QFont mediumFont(const QWidget& widget, ToggleSize size = ToggleSize::Default) {
    auto font = widget.font();
    font.setWeight(QFont::Medium);
    if (size == ToggleSize::Sm && font.pixelSize() > 0)
        font.setPixelSize(std::max(1, font.pixelSize() - 1));
    if (size == ToggleSize::Lg && font.pixelSize() > 0)
        font.setPixelSize(font.pixelSize() + 1);
    return font;
}

QColor muted(const QWidget& widget) { return colour(widget, Role::MutedForeground); }

} // namespace

Textarea::Textarea(QWidget* parent) : QPlainTextEdit(parent) {
    setFrameStyle(QFrame::NoFrame);
    setViewportMargins(10, 8, 10, 8);
    setMinimumHeight(64);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setTabChangesFocus(true);
    updatePalette();
}

void Textarea::setInvalid(bool invalid) {
    if (invalid_ == invalid) return;
    invalid_ = invalid;
    setInvalidProperty(*this, invalid);
}

void Textarea::setError(const QString& message) {
    setInvalid(!message.isEmpty());
    setAccessibleDescription(message);
}

QSize Textarea::sizeHint() const {
    const auto base = QPlainTextEdit::sizeHint();
    return {std::max(200, base.width()), std::max(64, base.height())};
}

QSize Textarea::minimumSizeHint() const { return {80, 64}; }

void Textarea::updatePalette() {
    auto palette = this->palette();
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Text, foregroundFor(*this));
    palette.setColor(QPalette::PlaceholderText, muted(*this));
    palette.setColor(QPalette::Highlight, colour(*this, Role::Primary));
    palette.setColor(QPalette::HighlightedText, colour(*this, Role::PrimaryForeground));
    setPalette(palette);
}

void Textarea::changeEvent(QEvent* event) {
    QPlainTextEdit::changeEvent(event);
    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange)
        updatePalette();
}

bool Textarea::event(QEvent* event) {
    // QPlainTextEdit paints text on its viewport. The surrounding frame must
    // be painted during the outer widget's own paint event.
    if (event->type() != QEvent::Paint) {
        const bool handled = QPlainTextEdit::event(event);
        if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut)
            update();
        return handled;
    }
    if (property("shadcnEmbedded").toBool()) return true;
    QPainter painter(this);
    const auto fill = themeFor(*this).mode() == ColorMode::Dark
        ? withAlpha(colour(*this, Role::Input), .3) : QColor(Qt::transparent);
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius(*this), fill,
            borderFor(*this, invalid_));
    if (isEnabled()) paintFocus(painter, *this, QRectF(rect()));
    if (!isEnabled()) painter.fillRect(rect(), withAlpha(colour(*this, Role::Background), .18));
    return true;
}

Toggle::Toggle(const QString& text, QWidget* parent) : QPushButton(text, parent) {
    setCheckable(true);
    setAutoDefault(false);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setAttribute(Qt::WA_Hover);
}

void Toggle::setVariant(ToggleVariant variant) {
    if (variant_ == variant) return;
    variant_ = variant;
    update();
}

void Toggle::setToggleSize(ToggleSize size) {
    if (size_ == size) return;
    size_ = size;
    updateGeometry();
    update();
}

void Toggle::setInvalid(bool invalid) {
    if (invalid_ == invalid) return;
    invalid_ = invalid;
    setInvalidProperty(*this, invalid);
}

QSize Toggle::sizeHint() const {
    const auto font = mediumFont(*this, size_);
    const QFontMetrics metrics(font);
    const auto height = std::max(toggleHeight(size_), metrics.height() + 8);
    return {metrics.horizontalAdvance(text()) + 2 * togglePadding(size_), height};
}

QSize Toggle::minimumSizeHint() const { return sizeHint(); }

void Toggle::keyPressEvent(QKeyEvent* event) { QPushButton::keyPressEvent(event); }

void Toggle::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const auto dark = themeFor(*this).mode() == ColorMode::Dark;
    QColor fill = Qt::transparent;
    QColor textColor = foregroundFor(*this);
    QColor border = Qt::transparent;
    if (isChecked()) {
        fill = colour(*this, Role::Muted);
        textColor = foregroundFor(*this);
    } else if (underMouse()) {
        fill = withAlpha(colour(*this, Role::Muted), dark ? .45 : .8);
        textColor = muted(*this);
    }
    if (variant_ == ToggleVariant::Outline) {
        border = borderFor(*this, invalid_);
        if (!isChecked() && underMouse()) fill = withAlpha(colour(*this, Role::Accent), .7);
    }
    if (invalid_) border = colour(*this, Role::Destructive);
    if (!isEnabled()) painter.setOpacity(.5);
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius(*this), fill, border);
    paintFocus(painter, *this, QRectF(rect()));
    painter.setFont(mediumFont(*this, size_));
    painter.setPen(textColor);
    painter.drawText(rect(), Qt::AlignCenter, this->text());
}

ToggleGroup::ToggleGroup(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent), layout_(nullptr), orientation_(orientation) {
    layout_ = orientation == Qt::Horizontal
        ? static_cast<QBoxLayout*>(new QHBoxLayout(this))
        : static_cast<QBoxLayout*>(new QVBoxLayout(this));
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);
    setFocusPolicy(Qt::NoFocus);
}

ToggleGroup::~ToggleGroup() {
    for (const auto& toggle : toggles_) {
        if (!toggle) continue;
        toggle->removeEventFilter(this);
        disconnect(toggle, nullptr, this, nullptr);
    }
}

void ToggleGroup::addToggle(Toggle& toggle, const QString& value) {
    if (toggles_.contains(&toggle)) return;
    layout_->addWidget(&toggle);
    values_.insert(&toggle, value.isEmpty() ? toggle.text() : value);
    toggles_.append(QPointer<Toggle>(&toggle));
    toggle.installEventFilter(this);
    connect(&toggle, &QObject::destroyed, this, [this, key = &toggle](QObject* destroyed) {
        values_.remove(key);
        toggles_.removeIf([destroyed](const auto& item) {
            return QPointer<QObject>(item).data() == destroyed;
        });
        updateTabStop();
    });
    const QPointer<Toggle> observed(&toggle);
    connect(&toggle, &QAbstractButton::toggled, this, [this, observed](bool) {
        if (observed) onToggleChanged(observed.data());
    });
    if (mode_ == ToggleGroupMode::Single && toggle.isChecked()) {
        for (const auto& other : toggles_) {
            if (other && other != &toggle) other->setChecked(false);
        }
    }
    updateTabStop();
}

void ToggleGroup::removeToggle(Toggle& toggle) {
    if (!toggles_.contains(&toggle)) return;
    layout_->removeWidget(&toggle);
    values_.remove(&toggle);
    toggles_.removeAll(QPointer<Toggle>(&toggle));
    toggle.removeEventFilter(this);
    toggle.setFocusPolicy(Qt::StrongFocus);
    toggle.setParent(nullptr);
    toggle.hide();
    updateTabStop();
    emit valuesChanged(checkedValues());
}

void ToggleGroup::updateTabStop(Toggle* preferred) {
    const auto available = [](const Toggle* toggle) {
        return toggle && toggle->isEnabled() && !toggle->isHidden();
    };
    Toggle* entry = available(preferred) ? preferred : nullptr;
    if (!entry && !focusEntered_) {
        for (const auto& toggle : toggles_)
            if (available(toggle) && toggle->isChecked()) { entry = toggle; break; }
    }
    if (!entry) {
        for (const auto& toggle : toggles_) {
            if (available(toggle) && toggle->focusPolicy() == Qt::StrongFocus) {
                entry = toggle;
                break;
            }
        }
    }
    if (!entry) {
        for (const auto& toggle : toggles_) {
            if (available(toggle)) { entry = toggle; break; }
        }
    }
    for (const auto& toggle : toggles_)
        if (toggle) toggle->setFocusPolicy(toggle == entry ? Qt::StrongFocus : Qt::ClickFocus);
}

bool ToggleGroup::eventFilter(QObject* watched, QEvent* event) {
    auto* current = qobject_cast<Toggle*>(watched);
    if (!current || !toggles_.contains(current)) return QWidget::eventFilter(watched, event);
    if (event->type() == QEvent::FocusIn) {
        focusEntered_ = true;
        updateTabStop(current);
    }
    else if (event->type() == QEvent::EnabledChange || event->type() == QEvent::Show ||
             event->type() == QEvent::Hide) updateTabStop();
    else if (event->type() == QEvent::KeyPress) {
        const auto* keyEvent = static_cast<QKeyEvent*>(event);
        const auto key = keyEvent->key();
        if (keyEvent->modifiers() != Qt::NoModifier &&
            (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up ||
             key == Qt::Key_Down || key == Qt::Key_Home || key == Qt::Key_End)) {
            event->ignore();
            return true;
        }
        QList<Toggle*> available;
        for (const auto& toggle : toggles_)
            if (toggle && toggle->isEnabled() && !toggle->isHidden()) available.append(toggle);
        if (available.isEmpty()) return false;
        qsizetype target = -1;
        const auto index = available.indexOf(current);
        if (key == Qt::Key_Home) target = 0;
        else if (key == Qt::Key_End) target = available.size() - 1;
        else {
            int direction = 0;
            if (orientation_ == Qt::Horizontal && (key == Qt::Key_Right || key == Qt::Key_Left))
                direction = (key == Qt::Key_Right) != (layoutDirection() == Qt::RightToLeft) ? 1 : -1;
            if (orientation_ == Qt::Vertical && (key == Qt::Key_Down || key == Qt::Key_Up))
                direction = key == Qt::Key_Down ? 1 : -1;
            if (direction) target = (index + direction + available.size()) % available.size();
        }
        if (target >= 0) {
            available.at(target)->setFocus(Qt::OtherFocusReason);
            event->accept();
            return true;
        }
        if (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down) {
            event->ignore();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

QList<Toggle*> ToggleGroup::toggles() const {
    QList<Toggle*> result;
    for (const auto& toggle : toggles_) if (toggle) result.append(toggle.data());
    return result;
}

void ToggleGroup::setMode(ToggleGroupMode mode) {
    if (mode_ == mode) return;
    mode_ = mode;
    if (mode_ == ToggleGroupMode::Single) {
        bool found = false;
        for (const auto& toggle : toggles_) {
            if (!toggle) continue;
            if (toggle->isChecked() && found) toggle->setChecked(false);
            else if (toggle->isChecked()) found = true;
        }
    }
    emit valuesChanged(checkedValues());
}

void ToggleGroup::setOrientation(Qt::Orientation orientation) {
    if (orientation_ == orientation) return;
    orientation_ = orientation;
    layout_->setDirection(orientation == Qt::Horizontal ? QBoxLayout::LeftToRight
                                                        : QBoxLayout::TopToBottom);
    updateGeometry();
}

void ToggleGroup::setSpacing(int spacing) { layout_->setSpacing(std::max(0, spacing)); }

QString ToggleGroup::valueFor(const Toggle& toggle) const {
    return values_.value(const_cast<Toggle*>(&toggle), toggle.text());
}

QStringList ToggleGroup::checkedValues() const {
    QStringList result;
    for (const auto& toggle : toggles_)
        if (toggle && toggle->isChecked()) result.append(valueFor(*toggle));
    return result;
}

void ToggleGroup::setCheckedValues(const QStringList& values) {
    for (const auto& toggle : toggles_) {
        if (!toggle) continue;
        const auto value = valueFor(*toggle);
        toggle->setChecked(values.contains(value));
    }
    if (mode_ == ToggleGroupMode::Single) {
        auto selected = checkedValues();
        if (selected.size() > 1) {
            bool kept = false;
            for (const auto& toggle : toggles_) {
                if (!toggle || !toggle->isChecked()) continue;
                if (kept) toggle->setChecked(false);
                else kept = true;
            }
        }
    }
    emit valuesChanged(checkedValues());
}

void ToggleGroup::onToggleChanged(Toggle* changed) {
    if (!changed || !toggles_.contains(changed)) return;
    if (mode_ == ToggleGroupMode::Single && changed->isChecked()) {
        for (const auto& toggle : toggles_) {
            if (toggle && toggle.data() != changed && toggle->isChecked()) {
                const QSignalBlocker blocker(toggle.data());
                toggle->setChecked(false);
            }
        }
    }
    updateTabStop();
    emit valuesChanged(checkedValues());
}

RadioGroupItem::RadioGroupItem(const QString& text, QWidget* parent) : QRadioButton(text, parent) {
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

QSize RadioGroupItem::sizeHint() const {
    const auto metrics = QFontMetrics(font());
    return {metrics.horizontalAdvance(text()) + 28, std::max(20, metrics.height() + 8)};
}

void RadioGroupItem::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const auto center = QPointF(10, height() / 2.0);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(isChecked() ? colour(*this, Role::Primary) : Qt::transparent);
    painter.setPen(QPen(isEnabled() ? borderFor(*this, property("shadcnInvalid").toBool())
                                    : withAlpha(borderFor(*this), .5), 1));
    painter.drawEllipse(center, 8, 8);
    if (isChecked()) {
        painter.setBrush(colour(*this, Role::PrimaryForeground));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(center, 3, 3);
    }
    if (hasFocus()) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(withAlpha(colour(*this, Role::Ring), .55), 2));
        painter.drawEllipse(center, 10, 10);
    }
    painter.setPen(isEnabled() ? foregroundFor(*this) : muted(*this));
    painter.drawText(QRect(26, 0, width() - 26, height()), Qt::AlignVCenter | Qt::TextShowMnemonic,
                     text());
}

RadioGroup::RadioGroup(QWidget* parent) : QWidget(parent), layout_(new QVBoxLayout(this)),
    buttons_(new QButtonGroup(this)) {
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(8);
    buttons_->setExclusive(true);
}

RadioGroup::~RadioGroup() {
    for (const auto& item : items_) {
        if (!item) continue;
        item->removeEventFilter(this);
        disconnect(item, nullptr, this, nullptr);
    }
}

void RadioGroup::addItem(RadioGroupItem& item, const QString& value) {
    if (items_.contains(&item)) return;
    layout_->addWidget(&item);
    const auto itemValue = value.isEmpty() ? item.text() : value;
    values_.insert(&item, itemValue);
    items_.append(QPointer<RadioGroupItem>(&item));
    item.installEventFilter(this);
    connect(&item, &QObject::destroyed, this, [this, key = &item](QObject* destroyed) {
        values_.remove(key);
        items_.removeIf([destroyed](const auto& observed) {
            return QPointer<QObject>(observed).data() == destroyed;
        });
        updateTabStop();
    });
    buttons_->addButton(&item, static_cast<int>(buttons_->buttons().size()));
    const QPointer<RadioGroupItem> observed(&item);
    connect(&item, &QAbstractButton::toggled, this, [this, observed](bool checked) {
        if (!observed || !items_.contains(observed)) return;
        updateTabStop();
        if (checked) emit valueChanged(values_.value(observed.data(), observed->text()));
    });
    updateTabStop();
}

void RadioGroup::removeItem(RadioGroupItem& item) {
    if (!items_.contains(&item)) return;
    layout_->removeWidget(&item);
    buttons_->removeButton(&item);
    values_.remove(&item);
    items_.removeAll(QPointer<RadioGroupItem>(&item));
    item.removeEventFilter(this);
    item.setFocusPolicy(Qt::StrongFocus);
    item.setParent(nullptr);
    item.hide();
    updateTabStop();
}

void RadioGroup::updateTabStop() {
    RadioGroupItem* entry = nullptr;
    for (const auto& item : items_) {
        if (!item || !item->isEnabled() || item->isHidden()) continue;
        if (!entry || item->isChecked()) entry = item;
        if (item->isChecked()) break;
    }
    for (const auto& item : items_)
        if (item) item->setFocusPolicy(item == entry ? Qt::StrongFocus : Qt::ClickFocus);
}

bool RadioGroup::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::EnabledChange || event->type() == QEvent::Show ||
        event->type() == QEvent::Hide) updateTabStop();
    if (event->type() == QEvent::KeyPress) {
        const auto* keyEvent = static_cast<QKeyEvent*>(event);
        const auto key = keyEvent->key();
        if (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down) {
            if (keyEvent->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
                event->ignore();
                return true;
            }
            QList<RadioGroupItem*> available;
            for (const auto& item : items_)
                if (item && item->isEnabled() && !item->isHidden()) available.append(item);
            const auto index = available.indexOf(qobject_cast<RadioGroupItem*>(watched));
            if (index < 0) return false;
            int direction = key == Qt::Key_Down || key == Qt::Key_Right ? 1 : -1;
            if ((key == Qt::Key_Left || key == Qt::Key_Right) && layoutDirection() == Qt::RightToLeft)
                direction = -direction;
            const QPointer<RadioGroupItem> next = available.at(
                (index + direction + available.size()) % available.size());
            next->setFocus(Qt::OtherFocusReason);
            if (next) next->setChecked(true);
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

QList<RadioGroupItem*> RadioGroup::items() const {
    QList<RadioGroupItem*> result;
    for (const auto& item : items_) if (item) result.append(item.data());
    return result;
}

QString RadioGroup::checkedValue() const {
    for (const auto& item : items_)
        if (item && item->isChecked()) return values_.value(item.data(), item->text());
    return {};
}

void RadioGroup::setCheckedValue(const QString& value) {
    for (const auto& item : items_)
        if (item) item->setChecked(values_.value(item.data(), item->text()) == value);
}

class AccessibleSliderThumb final : public QAccessibleInterface,
                                    public QAccessibleValueInterface,
                                    public QAccessibleActionInterface {
public:
    AccessibleSliderThumb(Slider* slider, int index) : slider_(slider), index_(index) {}
    bool isValid() const override { return slider_ && index_ < slider_->values_.size(); }
    QObject* object() const override { return nullptr; }
    QWindow* window() const override { return slider_ ? slider_->window()->windowHandle() : nullptr; }
    QAccessibleInterface* parent() const override { return QAccessible::queryAccessibleInterface(slider_); }
    QAccessibleInterface* child(int) const override { return nullptr; }
    QAccessibleInterface* childAt(int, int) const override { return nullptr; }
    int childCount() const override { return 0; }
    int indexOfChild(const QAccessibleInterface*) const override { return -1; }
    QAccessible::Role role() const override { return QAccessible::Slider; }
    QString text(QAccessible::Text type) const override {
        if (!isValid()) return {};
        if (type == QAccessible::Value) return QString::number(slider_->values_[index_]);
        if (type == QAccessible::Name)
            return slider_->accessibleName() + Slider::tr(" thumb %1").arg(index_ + 1);
        return {};
    }
    void setText(QAccessible::Text type, const QString& value) override {
        if (type == QAccessible::Value) setCurrentValue(value);
    }
    QRect rect() const override {
        if (!isValid()) return {};
        const auto position = qRound(slider_->positionFor(slider_->values_[index_]));
        const auto centre = slider_->orientation_ == Qt::Horizontal
            ? QPoint(position, slider_->height() / 2) : QPoint(slider_->width() / 2, position);
        return QRect(slider_->mapToGlobal(centre - QPoint(8, 8)), QSize(16, 16));
    }
    QAccessible::State state() const override {
        QAccessible::State result;
        result.invalid = !isValid();
        if (!isValid()) return result;
        result.focusable = true;
        result.focused = slider_->hasFocus() && std::max(0, slider_->activeThumb_) == index_;
        result.disabled = !slider_->isEnabled();
        result.invisible = !slider_->isVisible();
        return result;
    }
    void* interface_cast(QAccessible::InterfaceType type) override {
        if (type == QAccessible::ValueInterface) return static_cast<QAccessibleValueInterface*>(this);
        if (type == QAccessible::ActionInterface) return static_cast<QAccessibleActionInterface*>(this);
        return nullptr;
    }
    QVariant currentValue() const override { return isValid() ? QVariant(slider_->values_[index_]) : QVariant{}; }
    QVariant minimumValue() const override {
        if (!isValid()) return {};
        return index_ ? slider_->values_[index_ - 1] : slider_->minimum_;
    }
    QVariant maximumValue() const override {
        if (!isValid()) return {};
        return index_ + 1 < slider_->values_.size() ? slider_->values_[index_ + 1] : slider_->maximum_;
    }
    QVariant minimumStepSize() const override { return slider_ ? QVariant(slider_->singleStep_) : QVariant{}; }
    void setCurrentValue(const QVariant& value) override {
        bool valid = false;
        const double number = value.toDouble(&valid);
        if (!isValid() || !slider_->isEnabled() || !valid || !std::isfinite(number) ||
            number < minimumValue().toDouble() || number > maximumValue().toDouble()) return;
        slider_->setValueAt(index_, number);
    }
    QStringList actionNames() const override { return {setFocusAction(), increaseAction(), decreaseAction()}; }
    QStringList keyBindingsForAction(const QString&) const override { return {}; }
    void doAction(const QString& action) override {
        if (!isValid() || !slider_->isEnabled()) return;
        if (action == setFocusAction()) {
            slider_->activeThumb_ = index_;
            slider_->setFocus(Qt::OtherFocusReason);
            slider_->updateAccessibleValue();
            slider_->update();
        } else if (action == increaseAction()) setCurrentValue(currentValue().toDouble() + slider_->singleStep_);
        else if (action == decreaseAction()) setCurrentValue(currentValue().toDouble() - slider_->singleStep_);
    }
private:
    QPointer<Slider> slider_;
    int index_;
};

class AccessibleSlider final : public QAccessibleWidget {
public:
    explicit AccessibleSlider(Slider* slider) : QAccessibleWidget(slider, QAccessible::Grouping), slider_(slider) {}
    ~AccessibleSlider() override { clearChildren(); }
    int childCount() const override { return slider_ ? static_cast<int>(slider_->values_.size()) : 0; }
    QAccessibleInterface* child(int index) const override {
        if (index < 0 || index >= childCount()) return nullptr;
        if (children_.size() != childCount()) {
            clearChildren();
            for (int i = 0; i < childCount(); ++i)
                children_.append(QAccessible::registerAccessibleInterface(new AccessibleSliderThumb(slider_, i)));
        }
        return QAccessible::accessibleInterface(children_[index]);
    }
    int indexOfChild(const QAccessibleInterface* candidate) const override {
        for (int i = 0; i < childCount(); ++i) if (child(i) == candidate) return i;
        return -1;
    }
    QAccessibleInterface* childAt(int x, int y) const override {
        for (int i = 0; i < childCount(); ++i) if (child(i)->rect().contains(x, y)) return child(i);
        return nullptr;
    }
    QAccessibleInterface* focusChild() const override {
        return slider_ && slider_->hasFocus() ? child(std::max(0, slider_->activeThumb_)) : nullptr;
    }
private:
    void clearChildren() const {
        const auto ids = std::exchange(children_, {});
        for (auto id : ids) QAccessible::deleteAccessibleInterface(id);
    }
    QPointer<Slider> slider_;
    mutable QList<QAccessible::Id> children_;
};

Slider::Slider(QWidget* parent) : QWidget(parent) {
    static const bool registered = [] {
        QAccessible::installFactory([](const QString& name, QObject* object) -> QAccessibleInterface* {
            if (name == "shadcn::Slider")
                return new AccessibleSlider(qobject_cast<Slider*>(object));
            return nullptr;
        });
        return true;
    }();
    (void)registered;
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("Slider"));
    updateAccessibleValue();
}

Slider::Slider(double minimum, double maximum, QWidget* parent) : Slider(parent) {
    setRange(minimum, maximum);
}

void Slider::setMinimum(double minimum) { setRange(minimum, maximum_); }
void Slider::setMaximum(double maximum) { setRange(minimum_, maximum); }

void Slider::setRange(double minimum, double maximum) {
    if (!std::isfinite(minimum) || !std::isfinite(maximum)) return;
    if (maximum <= minimum) return;
    minimum_ = minimum;
    maximum_ = maximum;
    setValues(values_);
    updateAccessibleValue();
    update();
}

void Slider::setValues(const QVector<double>& values) {
    if (values.isEmpty()) return;
    QVector<double> next;
    next.reserve(values.size());
    for (const auto value : values)
        if (std::isfinite(value)) next.append(std::clamp(value, minimum_, maximum_));
    if (next.isEmpty()) return;
    std::sort(next.begin(), next.end());
    if (next == values_) return;
    values_ = next;
    if (activeThumb_ >= values_.size()) activeThumb_ = static_cast<int>(values_.size()) - 1;
    updateAccessibleValue();
    update();
    emit valuesChanged(values_);
}

void Slider::setOrientation(Qt::Orientation orientation) {
    if (orientation_ == orientation) return;
    orientation_ = orientation;
    setSizePolicy(orientation == Qt::Horizontal ? QSizePolicy::Expanding : QSizePolicy::Fixed,
                  orientation == Qt::Horizontal ? QSizePolicy::Fixed : QSizePolicy::Expanding);
    updateAccessibleValue();
    updateGeometry();
    update();
}

void Slider::setSingleStep(double step) {
    if (std::isfinite(step) && step > 0) singleStep_ = step;
}

void Slider::setPageStep(double step) {
    if (std::isfinite(step) && step > 0) pageStep_ = step;
}

void Slider::updateAccessibleValue() {
    if (values_.isEmpty()) return;
    const auto index = activeThumb_ >= 0 && activeThumb_ < values_.size() ? activeThumb_ : 0;
    QStringList values;
    for (const auto value : values_) values.append(QString::number(value));
    setAccessibleDescription(
        tr("Value %1. Thumb %2 of %3. Range %4 to %5. Use Tab to choose a thumb.")
            .arg(QString::number(values_.at(index)), QString::number(index + 1),
                 QString::number(values_.size()), QString::number(minimum_),
                 QString::number(maximum_)));
    if (auto* accessible = QAccessible::queryAccessibleInterface(this)) {
        if (auto* thumb = accessible->child(index)) {
            QAccessibleValueChangeEvent change(thumb, values_.at(index));
            QAccessible::updateAccessibility(&change);
        }
    }
}

QSize Slider::sizeHint() const { return orientation_ == Qt::Horizontal ? QSize(200, 24) : QSize(24, 160); }
QSize Slider::minimumSizeHint() const { return orientation_ == Qt::Horizontal ? QSize(48, 24) : QSize(24, 48); }

QRectF Slider::trackRect() const {
    if (orientation_ == Qt::Horizontal)
        return QRectF(8, height() / 2.0 - 2, std::max(0, width() - 16), 4);
    return QRectF(width() / 2.0 - 2, 8, 4, std::max(0, height() - 16));
}

double Slider::positionFor(double value) const {
    const auto track = trackRect();
    const double scale = std::max({std::abs(minimum_), std::abs(maximum_), 1.0});
    auto fraction = std::clamp((value / scale - minimum_ / scale) /
                              (maximum_ / scale - minimum_ / scale), 0.0, 1.0);
    if (orientation_ == Qt::Horizontal && layoutDirection() == Qt::RightToLeft)
        fraction = 1.0 - fraction;
    if (orientation_ == Qt::Horizontal) return track.left() + fraction * track.width();
    return track.bottom() - fraction * track.height();
}

double Slider::valueAt(const QPointF& point) const {
    const auto track = trackRect();
    double fraction = 0;
    if (orientation_ == Qt::Horizontal)
        fraction = (point.x() - track.left()) / std::max(1.0, track.width());
    else
        fraction = (track.bottom() - point.y()) / std::max(1.0, track.height());
    if (orientation_ == Qt::Horizontal && layoutDirection() == Qt::RightToLeft)
        fraction = 1.0 - fraction;
    return std::lerp(minimum_, maximum_, std::clamp(fraction, 0.0, 1.0));
}

int Slider::thumbAt(const QPointF& point) const {
    if (values_.isEmpty()) return -1;
    int selected = -1;
    double distance = std::numeric_limits<double>::infinity();
    for (int index = 0; index < values_.size(); ++index) {
        const auto position = positionFor(values_[index]);
        const auto candidate = orientation_ == Qt::Horizontal ? std::abs(point.x() - position)
                                                              : std::abs(point.y() - position);
        if (candidate <= distance) {
            distance = candidate;
            selected = index;
        }
    }
    return selected;
}

void Slider::setValueAt(int index, double value) {
    if (index < 0 || index >= values_.size()) return;
    value = std::clamp(value, minimum_, maximum_);
    if (index > 0) value = std::max(value, values_[index - 1]);
    if (index + 1 < values_.size()) value = std::min(value, values_[index + 1]);
    auto next = values_;
    next[index] = value;
    setValues(next);
}

void Slider::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const auto track = trackRect();
    rounded(painter, track, 2, colour(*this, Role::Muted));
    if (!values_.isEmpty()) {
        const auto start = values_.size() == 1 ? minimum_ : values_.front();
        const auto end = values_.back();
        if (end > start) {
            QRectF range = track;
            if (orientation_ == Qt::Horizontal) {
                const auto first = positionFor(start);
                const auto second = positionFor(end);
                range.setLeft(std::min(first, second));
                range.setRight(std::max(first, second));
            } else {
                range.setTop(positionFor(end));
                range.setBottom(positionFor(start));
            }
            rounded(painter, range, 2, colour(*this, Role::Primary));
        }
    }
    for (int index = 0; index < values_.size(); ++index) {
        const auto position = positionFor(values_[index]);
        const auto center = orientation_ == Qt::Horizontal
            ? QPointF(position, height() / 2.0) : QPointF(width() / 2.0, position);
        painter.setBrush(isEnabled() ? QColor(Qt::white) : withAlpha(QColor(Qt::white), .5));
        painter.setPen(QPen(colour(*this, Role::Primary), 1));
        painter.drawEllipse(center, 6, 6);
        if (hasFocus() && (activeThumb_ == index || activeThumb_ < 0)) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(withAlpha(colour(*this, Role::Ring), .55), 2));
            painter.drawEllipse(center, 9, 9);
        }
    }
    if (!isEnabled()) painter.fillRect(rect(), withAlpha(colour(*this, Role::Background), .2));
}

void Slider::mousePressEvent(QMouseEvent* event) {
    if (!isEnabled() || event->button() != Qt::LeftButton) return;
    setFocus(Qt::MouseFocusReason);
    activeThumb_ = thumbAt(event->position());
    updateAccessibleValue();
    setValueAt(activeThumb_, valueAt(event->position()));
    grabMouse();
    update();
}

void Slider::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons().testFlag(Qt::LeftButton) && activeThumb_ >= 0)
        setValueAt(activeThumb_, valueAt(event->position()));
}

void Slider::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && activeThumb_ >= 0) {
        releaseMouse();
        update();
    }
}

bool Slider::event(QEvent* event) {
    if (event->type() == QEvent::FocusIn) {
        const auto* focus = static_cast<QFocusEvent*>(event);
        activeThumb_ = focus->reason() == Qt::BacktabFocusReason ? static_cast<int>(values_.size()) - 1 : 0;
        updateAccessibleValue();
    } else if (event->type() == QEvent::FocusOut) activeThumb_ = -1;
    if (event->type() == QEvent::KeyPress && isEnabled() && values_.size() > 1) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab) {
            const auto count = static_cast<int>(values_.size());
            const auto forward = key->key() != Qt::Key_Backtab && !key->modifiers().testFlag(Qt::ShiftModifier);
            if (activeThumb_ < 0) activeThumb_ = forward ? 0 : count - 1;
            else {
                const auto next = activeThumb_ + (forward ? 1 : -1);
                if (next < 0 || next >= count) {
                    activeThumb_ = -1;
                    return QWidget::event(event);
                }
                activeThumb_ = next;
            }
            updateAccessibleValue();
            if (auto* accessible = QAccessible::queryAccessibleInterface(this)) {
                if (auto* thumb = accessible->child(activeThumb_)) {
                    QAccessibleEvent focus(thumb, QAccessible::Focus);
                    QAccessible::updateAccessibility(&focus);
                }
            }
            update();
            event->accept();
            return true;
        }
    }
    return QWidget::event(event);
}

void Slider::keyPressEvent(QKeyEvent* event) {
    if (!isEnabled() || values_.isEmpty()) return;
    auto index = activeThumb_ >= 0 ? activeThumb_ : 0;
    auto value = values_[index];
    const auto direction = (orientation_ == Qt::Horizontal && layoutDirection() == Qt::RightToLeft) ? -1 : 1;
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Down) value -= singleStep_ * direction;
    else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Up) value += singleStep_ * direction;
    else if (event->key() == Qt::Key_PageDown) value -= pageStep_ * direction;
    else if (event->key() == Qt::Key_PageUp) value += pageStep_ * direction;
    else if (event->key() == Qt::Key_Home) value = minimum_;
    else if (event->key() == Qt::Key_End) value = maximum_;
    else { QWidget::keyPressEvent(event); return; }
    setValueAt(index, value);
    activeThumb_ = index;
    updateAccessibleValue();
    event->accept();
}

void Slider::wheelEvent(QWheelEvent* event) {
    if (!isEnabled() || values_.isEmpty()) return;
    const auto steps = event->angleDelta().y() / 120;
    if (steps == 0) return;
    const auto index = activeThumb_ >= 0 ? activeThumb_ : 0;
    setValueAt(index, values_[index] + static_cast<double>(steps) * singleStep_);
    event->accept();
}

Alert::Alert(QWidget* parent) : QFrame(parent), title_(new QLabel(this)),
    description_(new QLabel(this)), content_(new QVBoxLayout(this)) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setFrameStyle(QFrame::NoFrame);
    setAccessibleName(QStringLiteral("Alert"));
    title_->setVisible(false);
    title_->setWordWrap(true);
    title_->setTextFormat(Qt::PlainText);
    description_->setVisible(false);
    description_->setWordWrap(true);
    description_->setTextFormat(Qt::PlainText);
    content_->setContentsMargins(10, 8, 10, 8);
    content_->setSpacing(4);
    content_->addWidget(title_);
    content_->addWidget(description_);
    updatePalette();
}

void Alert::setVariant(AlertVariant variant) {
    if (variant_ == variant) return;
    variant_ = variant;
    updatePalette();
    update();
}

void Alert::setTitle(const QString& title) {
    title_->setText(title);
    title_->setVisible(!title.isEmpty());
    if (!title.isEmpty()) setAccessibleName(title);
    updateGeometry();
}

void Alert::setDescription(const QString& description) {
    description_->setText(description);
    description_->setVisible(!description.isEmpty());
    setAccessibleDescription(description);
    updateGeometry();
}

QString Alert::title() const { return title_->text(); }
QString Alert::description() const { return description_->text(); }
QVBoxLayout& Alert::content() { return *content_; }
QSize Alert::sizeHint() const { return {std::max(220, content_->sizeHint().width()), content_->sizeHint().height()}; }

void Alert::updatePalette() {
    auto palette = this->palette();
    const auto text = variant_ == AlertVariant::Destructive ? colour(*this, Role::Destructive)
                                                             : foregroundFor(*this);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Text, text);
    title_->setPalette(palette);
    description_->setPalette(palette);
}

void Alert::changeEvent(QEvent* event) {
    QFrame::changeEvent(event);
    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange)
        updatePalette();
}

void Alert::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const auto destructive = variant_ == AlertVariant::Destructive;
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius(*this),
            colour(*this, Role::Card), borderFor(*this));
    if (destructive) {
        painter.setPen(QPen(withAlpha(colour(*this, Role::Destructive), .8), 1));
        painter.drawRoundedRect(QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius(*this), radius(*this));
    }
}

Avatar::Avatar(const QString& fallback, QWidget* parent) : QFrame(parent), fallback_(fallback) {
    setFrameStyle(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAccessibleName(fallback);
}

void Avatar::setAvatarSize(AvatarSize size) {
    if (size_ == size) return;
    size_ = size;
    updateGeometry();
    update();
}

void Avatar::setImage(const QPixmap& image) {
    image_ = image;
    update();
}

void Avatar::clearImage() {
    image_ = {};
    update();
}

void Avatar::setFallback(const QString& fallback) {
    fallback_ = fallback;
    setAccessibleName(fallback);
    update();
}

void Avatar::setBadge(const QString& text) {
    badge_ = text;
    update();
}

QSize Avatar::sizeHint() const {
    switch (size_) {
    case AvatarSize::Sm: return {24, 24};
    case AvatarSize::Lg: return {40, 40};
    case AvatarSize::Default: return {32, 32};
    }
    return {32, 32};
}

void Avatar::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const auto bounds = QRectF(rect()).adjusted(.5, .5, -.5, -.5);
    QPainterPath clip;
    clip.addEllipse(bounds);
    painter.save();
    painter.setClipPath(clip);
    if (!image_.isNull()) {
        const auto scaled = image_.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        painter.drawPixmap((width() - scaled.width()) / 2, (height() - scaled.height()) / 2, scaled);
    } else {
        painter.fillPath(clip, colour(*this, Role::Muted));
        painter.setPen(muted(*this));
        painter.setFont(font());
        painter.drawText(rect(), Qt::AlignCenter, fallback_);
    }
    painter.restore();
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(colour(*this, Role::Background), 2));
    painter.drawEllipse(bounds);
    if (!badge_.isEmpty()) {
        const auto badgeSize = std::max(6, width() / 3);
        const auto badge = QRectF(width() - badgeSize - 1, height() - badgeSize - 1, badgeSize, badgeSize);
        painter.setBrush(colour(*this, Role::Primary));
        painter.setPen(QPen(colour(*this, Role::Background), 1));
        painter.drawEllipse(badge);
        if (badgeSize >= 10) {
            painter.setPen(colour(*this, Role::PrimaryForeground));
            painter.setFont(font());
            painter.drawText(badge, Qt::AlignCenter, badge_);
        }
    }
}

AvatarGroup::AvatarGroup(QWidget* parent) : QFrame(parent), layout_(new QHBoxLayout(this)),
    overflowLabel_(new QLabel(this)) {
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(-8);
    overflowLabel_->setAlignment(Qt::AlignCenter);
    overflowLabel_->setVisible(false);
    setAccessibleName(QStringLiteral("Avatar group"));
}

void AvatarGroup::addAvatar(Avatar& avatar) {
    if (avatars_.contains(&avatar)) return;
    layout_->addWidget(&avatar);
    avatars_.append(QPointer<Avatar>(&avatar));
    updateGeometry();
}

void AvatarGroup::removeAvatar(Avatar& avatar) {
    if (!avatars_.contains(&avatar)) return;
    layout_->removeWidget(&avatar);
    avatars_.removeAll(QPointer<Avatar>(&avatar));
    avatar.setParent(nullptr);
    avatar.hide();
    updateGeometry();
}

void AvatarGroup::setOverflowCount(int count) {
    overflow_ = std::max(0, count);
    if (overflow_ == 0) {
        layout_->removeWidget(overflowLabel_);
        overflowLabel_->hide();
    } else {
        if (layout_->indexOf(overflowLabel_) < 0) layout_->addWidget(overflowLabel_);
        overflowLabel_->setText(QStringLiteral("+%1").arg(overflow_));
        overflowLabel_->setFixedSize(32, 32);
        overflowLabel_->setStyleSheet(QStringLiteral("border-radius:16px;"));
        overflowLabel_->setAutoFillBackground(true);
        auto palette = overflowLabel_->palette();
        palette.setColor(QPalette::Window, colour(*this, Role::Muted));
        palette.setColor(QPalette::WindowText, muted(*this));
        overflowLabel_->setPalette(palette);
        overflowLabel_->show();
    }
    updateGeometry();
}

QList<Avatar*> AvatarGroup::avatars() const {
    QList<Avatar*> result;
    for (const auto& avatar : avatars_) if (avatar) result.append(avatar.data());
    return result;
}

AspectRatio::AspectRatio(double ratio, QWidget* parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setRatio(ratio);
}

void AspectRatio::setRatio(double ratio) {
    if (!std::isfinite(ratio) || ratio <= 0) return;
    ratio_ = ratio;
    updateGeometry();
    if (child_) child_->setGeometry(rect());
}

void AspectRatio::setWidget(QWidget& widget) {
    if (child_ == &widget) return;
    if (child_) child_->setParent(nullptr);
    child_ = &widget;
    child_->setParent(this);
    child_->show();
    child_->setGeometry(rect());
}

QSize AspectRatio::sizeHint() const { return {320, heightForWidth(320)}; }
int AspectRatio::heightForWidth(int width) const { return std::max(1, qRound(static_cast<double>(width) / ratio_)); }
void AspectRatio::resizeEvent(QResizeEvent*) { if (child_) child_->setGeometry(rect()); }

Breadcrumb::Breadcrumb(QWidget* parent) : QFrame(parent), layout_(new QHBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(6);
    setAccessibleName(QStringLiteral("Breadcrumb"));
}

QPushButton& Breadcrumb::addLink(const QString& text) {
    auto* button = new QPushButton(text, this);
    button->setFlat(true);
    button->setAutoDefault(false);
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::StrongFocus);
    button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    auto palette = button->palette();
    palette.setColor(QPalette::ButtonText, muted(*this));
    button->setPalette(palette);
    layout_->addWidget(button);
    const QPointer<QPushButton> observed(button);
    connect(button, &QPushButton::clicked, this, [this, observed] {
        if (observed) emit linkActivated(observed->text());
    });
    return *button;
}

QLabel& Breadcrumb::addPage(const QString& text) {
    auto* label = new QLabel(text, this);
    label->setTextFormat(Qt::PlainText);
    label->setAccessibleName(QStringLiteral("Current page: ") + text);
    label->setStyleSheet(QStringLiteral("font-weight:500;"));
    auto palette = label->palette();
    palette.setColor(QPalette::WindowText, foregroundFor(*this));
    label->setPalette(palette);
    layout_->addWidget(label);
    return *label;
}

QLabel& Breadcrumb::addSeparator(const QString& text) {
    auto* label = new QLabel(text, this);
    label->setTextFormat(Qt::PlainText);
    label->setAccessibleName(QStringLiteral("Breadcrumb separator"));
    label->setAccessibleDescription(QStringLiteral("Presentation only"));
    auto palette = label->palette();
    palette.setColor(QPalette::WindowText, muted(*this));
    label->setPalette(palette);
    layout_->addWidget(label);
    return *label;
}

QLabel& Breadcrumb::addEllipsis() { return addSeparator(QStringLiteral("…")); }

void Breadcrumb::clear() {
    while (auto* item = layout_->takeAt(0)) {
        if (auto* widget = item->widget()) widget->deleteLater();
        delete item;
    }
}

int Breadcrumb::count() const { return layout_->count(); }

ButtonGroup::ButtonGroup(Qt::Orientation orientation, QWidget* parent)
    : QFrame(parent), layout_(nullptr), orientation_(orientation) {
    setFrameStyle(QFrame::NoFrame);
    layout_ = orientation == Qt::Horizontal
        ? static_cast<QBoxLayout*>(new QHBoxLayout(this))
        : static_cast<QBoxLayout*>(new QVBoxLayout(this));
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);
}

void ButtonGroup::addWidget(QWidget& widget) { layout_->addWidget(&widget); }
void ButtonGroup::addButton(Button& button) { addWidget(button); }

Separator& ButtonGroup::addSeparator(Qt::Orientation separatorOrientation) {
    auto* separator = new Separator(separatorOrientation, this);
    layout_->addWidget(separator);
    return *separator;
}

QLabel& ButtonGroup::addText(const QString& text) {
    auto* label = new QLabel(text, this);
    label->setTextFormat(Qt::PlainText);
    label->setAlignment(Qt::AlignCenter);
    auto palette = label->palette();
    palette.setColor(QPalette::WindowText, muted(*this));
    label->setPalette(palette);
    layout_->addWidget(label);
    return *label;
}

void ButtonGroup::setOrientation(Qt::Orientation orientation) {
    if (orientation_ == orientation) return;
    orientation_ = orientation;
    layout_->setDirection(orientation == Qt::Horizontal ? QBoxLayout::LeftToRight
                                                        : QBoxLayout::TopToBottom);
    updateGeometry();
}

InputGroup::InputGroup(QWidget* parent) : QFrame(parent), layout_(new QHBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(4, 0, 4, 0);
    layout_->setSpacing(2);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void InputGroup::addWidget(QWidget& widget, InputGroupAlign align) {
    if (qobject_cast<Input*>(&widget) || qobject_cast<Textarea*>(&widget))
        widget.setProperty("shadcnEmbedded", true);
    const auto alignment = align == InputGroupAlign::BlockStart ? Qt::AlignTop
        : align == InputGroupAlign::BlockEnd ? Qt::AlignBottom : Qt::Alignment{};
    layout_->addWidget(&widget, 1, alignment);
    widget.installEventFilter(this);
}

void InputGroup::addInput(Input& input) {
    addWidget(input);
}

void InputGroup::addTextarea(Textarea& textarea) {
    layout_->setContentsMargins(4, 4, 4, 4);
    addWidget(textarea);
}

Button& InputGroup::addButton(const QString& text, InputGroupButtonSize size) {
    auto* button = new Button(text, this);
    button->setVariant(Variant::Ghost);
    switch (size) {
    case InputGroupButtonSize::Xs: button->setButtonSize(ButtonSize::Xs); break;
    case InputGroupButtonSize::Sm: button->setButtonSize(ButtonSize::Sm); break;
    case InputGroupButtonSize::IconXs: button->setButtonSize(ButtonSize::IconXs); break;
    case InputGroupButtonSize::IconSm: button->setButtonSize(ButtonSize::IconSm); break;
    }
    layout_->addWidget(button, 0);
    return *button;
}

QLabel& InputGroup::addText(const QString& text) {
    auto* label = new QLabel(text, this);
    label->setTextFormat(Qt::PlainText);
    auto palette = label->palette();
    palette.setColor(QPalette::WindowText, muted(*this));
    label->setPalette(palette);
    layout_->addWidget(label, 0);
    return *label;
}

void InputGroup::setInvalid(bool invalid) {
    if (invalid_ == invalid) return;
    invalid_ = invalid;
    setInvalidProperty(*this, invalid);
}

void InputGroup::setError(const QString& message) {
    setInvalid(!message.isEmpty());
    setAccessibleDescription(message);
}

void InputGroup::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const auto fill = themeFor(*this).mode() == ColorMode::Dark
        ? withAlpha(colour(*this, Role::Input), .3) : QColor(Qt::transparent);
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius(*this), fill,
            borderFor(*this, invalid_));
    paintFocus(painter, *this, QRectF(rect()));
}

bool InputGroup::eventFilter(QObject* object, QEvent* event) {
    if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) update();
    return QFrame::eventFilter(object, event);
}

Kbd::Kbd(const QString& text, QWidget* parent) : QLabel(text, parent) {
    setTextFormat(Qt::PlainText);
    setAlignment(Qt::AlignCenter);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAccessibleName(text);
}

QSize Kbd::sizeHint() const {
    const auto base = QLabel::sizeHint();
    return {std::max(20, base.width() + 8), std::max(20, base.height() + 2)};
}

void Kbd::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5), 4, colour(*this, Role::Muted));
    painter.setPen(muted(*this));
    painter.setFont(font());
    painter.drawText(rect(), Qt::AlignCenter, text());
}

KbdGroup::KbdGroup(QWidget* parent) : QFrame(parent), layout_(new QHBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(4);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
}

void KbdGroup::addKey(Kbd& key) { layout_->addWidget(&key); }

Kbd& KbdGroup::addKey(const QString& text) {
    auto* key = new Kbd(text, this);
    addKey(*key);
    return *key;
}

int KbdGroup::count() const { return layout_->count(); }

Spinner::Spinner(QWidget* parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("Loading"));
    timer_.setInterval(80);
    connect(&timer_, &QTimer::timeout, this, [this] {
        angle_ = (angle_ + 24) % 360;
        update();
    });
    updateTimer();
}

void Spinner::setSpinning(bool spinning) {
    if (spinning_ == spinning) return;
    spinning_ = spinning;
    updateTimer();
}

QSize Spinner::sizeHint() const { return {16, 16}; }

void Spinner::updateTimer() {
    if (spinning_ && isVisible() && !reducedMotion(*this)) timer_.start();
    else timer_.stop();
}

void Spinner::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    updateTimer();
}

void Spinner::hideEvent(QHideEvent* event) {
    timer_.stop();
    QWidget::hideEvent(event);
}

void Spinner::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange)
        updateTimer();
}

void Spinner::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width() / 2.0, height() / 2.0);
    painter.rotate(angle_);
    painter.setPen(QPen(colour(*this, Role::Foreground), 2));
    painter.drawArc(QRectF(-6, -6, 12, 12), 45 * 16, 270 * 16);
}

Empty::Empty(QWidget* parent) : QFrame(parent), title_(new QLabel(this)),
    description_(new QLabel(this)), content_(new QVBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    title_->setVisible(false);
    description_->setVisible(false);
    description_->setWordWrap(true);
    description_->setTextFormat(Qt::PlainText);
    title_->setAlignment(Qt::AlignCenter);
    description_->setAlignment(Qt::AlignCenter);
    content_->setContentsMargins(24, 24, 24, 24);
    content_->setSpacing(8);
    content_->setAlignment(Qt::AlignCenter);
    content_->addWidget(title_);
    content_->addWidget(description_);
}

void Empty::setTitle(const QString& title) {
    title_->setText(title);
    title_->setVisible(!title.isEmpty());
    title_->setAccessibleName(title);
    updateGeometry();
}

void Empty::setDescription(const QString& description) {
    description_->setText(description);
    description_->setVisible(!description.isEmpty());
    setAccessibleDescription(description);
    updateGeometry();
}

QString Empty::title() const { return title_->text(); }
QString Empty::description() const { return description_->text(); }
void Empty::addWidget(QWidget& widget) { content_->addWidget(&widget, 0, Qt::AlignCenter); }
QVBoxLayout& Empty::content() { return *content_; }

void Empty::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(borderFor(*this), 1, Qt::DashLine));
    painter.drawRoundedRect(QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius(*this), radius(*this));
}

Item::Item(QWidget* parent) : QFrame(parent), row_(new QHBoxLayout(this)),
    content_(new QVBoxLayout), title_(new QLabel(this)), description_(new QLabel(this)) {
    setFrameStyle(QFrame::NoFrame);
    row_->setContentsMargins(12, 10, 12, 10);
    row_->setSpacing(12);
    auto* body = new QWidget(this);
    body->setLayout(content_);
    content_->setContentsMargins(0, 0, 0, 0);
    content_->setSpacing(4);
    title_->setVisible(false);
    description_->setVisible(false);
    description_->setWordWrap(true);
    description_->setTextFormat(Qt::PlainText);
    content_->addWidget(title_);
    content_->addWidget(description_);
    row_->addWidget(body, 1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

void Item::setVariant(ItemVariant variant) {
    if (variant_ == variant) return;
    variant_ = variant;
    update();
}

void Item::setItemSize(ItemSize size) {
    if (size_ == size) return;
    size_ = size;
    const auto margin = size == ItemSize::Xs ? 8 : 10;
    row_->setContentsMargins(margin, margin, margin, margin);
    updateGeometry();
    update();
}

void Item::setTitle(const QString& title) {
    title_->setText(title);
    title_->setVisible(!title.isEmpty());
    updateGeometry();
}

void Item::setDescription(const QString& description) {
    description_->setText(description);
    description_->setVisible(!description.isEmpty());
    updateGeometry();
}

void Item::addLeading(QWidget& widget) {
    if (leading_) row_->removeWidget(leading_);
    leading_ = &widget;
    row_->insertWidget(0, leading_);
}

void Item::addTrailing(QWidget& widget) {
    if (trailing_) row_->removeWidget(trailing_);
    trailing_ = &widget;
    row_->addWidget(trailing_);
}

QVBoxLayout& Item::content() { return *content_; }
QSize Item::sizeHint() const { return row_->sizeHint(); }

void Item::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    QColor fill = Qt::transparent;
    QColor border = Qt::transparent;
    if (variant_ == ItemVariant::Outline) border = borderFor(*this);
    if (variant_ == ItemVariant::Muted) fill = withAlpha(colour(*this, Role::Muted), .5);
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius(*this), fill, border);
}

ItemGroup::ItemGroup(QWidget* parent) : QFrame(parent), layout_(new QVBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(16);
}

void ItemGroup::addItem(Item& item) { layout_->addWidget(&item); }

void ItemGroup::addSeparator() {
    auto* separator = new Separator(Qt::Horizontal, this);
    layout_->addWidget(separator);
}

int ItemGroup::count() const { return layout_->count(); }

} // namespace shadcn
