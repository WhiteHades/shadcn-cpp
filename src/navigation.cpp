// SPDX-License-Identifier: MIT
// Design source: shadcn-ui/ui radix-nova wrappers and style-nova.css.
#include <shadcn/navigation.hpp>

#include <QApplication>
#include <QContextMenuEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QScreen>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariantAnimation>
#include <algorithm>
#include <cmath>

namespace shadcn {
namespace {

const Theme& themeFor(const QWidget& widget) {
    if (const auto* style = qobject_cast<const Style*>(widget.style())) return style->theme();
    if (const auto* style = qobject_cast<const Style*>(QApplication::style())) return style->theme();
    static const Theme fallback = Theme::neutral();
    return fallback;
}

QColor colour(const QWidget& widget, Role role) {
    const auto value = themeFor(widget).color(role);
    return QColor::fromRgbF(static_cast<float>(value.r), static_cast<float>(value.g),
                            static_cast<float>(value.b), static_cast<float>(value.a));
}

bool reducedMotion(const QWidget& widget) {
    if (const auto* style = qobject_cast<const Style*>(widget.style()))
        return style->motion() == MotionPolicy::Reduced;
    if (const auto* style = qobject_cast<const Style*>(QApplication::style()))
        return style->motion() == MotionPolicy::Reduced;
    return false;
}

int motionDuration(const QWidget& widget, int full = 180) {
    return reducedMotion(widget) || !widget.isVisible() ? 0 : full;
}

QString rgb(const QColor& color) {
    return QStringLiteral("rgb(%1,%2,%3)").arg(color.red()).arg(color.green()).arg(color.blue());
}

QIcon accordionIcon(const QWidget& owner, bool expanded) {
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(colour(owner, Role::MutedForeground), 1.5, Qt::SolidLine,
                        Qt::RoundCap, Qt::RoundJoin));
    if (expanded) {
        painter.drawLine(QPointF(3.5, 10.0), QPointF(8.0, 5.5));
        painter.drawLine(QPointF(8.0, 5.5), QPointF(12.5, 10.0));
    } else {
        painter.drawLine(QPointF(3.5, 6.0), QPointF(8.0, 10.5));
        painter.drawLine(QPointF(8.0, 10.5), QPointF(12.5, 6.0));
    }
    return QIcon(pixmap);
}

void rounded(QPainter& painter, const QRectF& rect, double radius,
             const QColor& fill, const QColor& border = QColor(Qt::transparent)) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(fill);
    painter.setPen(border.alpha() == 0 ? Qt::NoPen : QPen(border, 1));
    const auto bounded = std::min(radius, std::min(rect.width(), rect.height()) / 2.0);
    painter.drawRoundedRect(rect, bounded, bounded);
}

QString menuStyle(const QWidget& widget, const QString& extra = {}) {
    const auto popover = rgb(colour(widget, Role::Popover));
    const auto foreground = rgb(colour(widget, Role::PopoverForeground));
    const auto accent = rgb(colour(widget, Role::Accent));
    const auto accentForeground = rgb(colour(widget, Role::AccentForeground));
    const auto border = rgb(colour(widget, Role::Border));
    const auto muted = rgb(colour(widget, Role::MutedForeground));
    return QStringLiteral(
               "QMenu { background:%1; color:%2; border:1px solid %3; border-radius:8px; padding:4px; }"
               "QMenu::item { padding:5px 10px; border-radius:6px; }"
               "QMenu::item:selected { background:%4; color:%5; }"
               "QMenu::item:disabled { color:%6; }"
               "QMenu::separator { height:1px; background:%3; margin:4px 0; }%7")
        .arg(popover, foreground, border, accent, accentForeground, muted, extra);
}

void styleTrigger(QPushButton& button, const QWidget& owner, bool active = false) {
    const auto foreground = rgb(colour(owner, Role::Foreground));
    const auto muted = rgb(colour(owner, Role::MutedForeground));
    const auto accent = rgb(colour(owner, Role::Accent));
    const auto accentForeground = rgb(colour(owner, Role::AccentForeground));
    const auto border = rgb(colour(owner, Role::Border));
    button.setStyleSheet(
        QStringLiteral("QPushButton { border:1px solid transparent; border-radius:8px; padding:6px 10px; "
                       "color:%1; background:transparent; text-align:left; }"
                       "QPushButton:hover { background:%2; color:%3; }"
                       "QPushButton:checked { background:%2; color:%3; }"
                       "QPushButton:focus { border-color:%4; }"
                       "QPushButton:disabled { color:%5; }")
            .arg(foreground, accent, accentForeground, border, muted));
    if (active) button.setChecked(true);
}

int nextEnabled(const QList<QPointer<QPushButton>>& buttons, int start, int step) {
    if (buttons.isEmpty()) return -1;
    const auto count = static_cast<int>(buttons.size());
    auto index = start;
    for (int i = 0; i < count; ++i) {
        index = (index + step + count) % count;
        if (buttons.at(index) && buttons.at(index)->isEnabled()) return index;
    }
    return -1;
}

} // namespace

DirectionProvider::DirectionProvider(Direction direction, QWidget* parent)
    : QWidget(parent), direction_(direction), content_(new QVBoxLayout(this)) {
    content_->setContentsMargins(0, 0, 0, 0);
    content_->setSpacing(0);
    setLayoutDirection(direction == Direction::RightToLeft ? Qt::RightToLeft : Qt::LeftToRight);
}

void DirectionProvider::setDirection(Direction direction) {
    if (direction_ == direction) return;
    direction_ = direction;
    setLayoutDirection(direction == Direction::RightToLeft ? Qt::RightToLeft : Qt::LeftToRight);
    emit directionChanged(direction_);
}

QVBoxLayout& DirectionProvider::content() { return *content_; }

AccordionItem::AccordionItem(const QString& title, QWidget* parent)
    : QWidget(parent), trigger_(new QPushButton(title, this)), contentHost_(new QWidget(this)),
      content_(new QVBoxLayout(contentHost_)), animation_(new QVariantAnimation(this)) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    trigger_->setProperty("shadcnAccordionTitle", title);
    trigger_->setCheckable(true);
    trigger_->setFocusPolicy(Qt::StrongFocus);
    trigger_->setIcon(accordionIcon(*this, false));
    trigger_->setIconSize(QSize(16, 16));
    trigger_->setLayoutDirection(Qt::RightToLeft);
    trigger_->setStyleSheet(QStringLiteral(
                                "QPushButton { color:%1; background:transparent; border:1px solid transparent; "
                                "border-radius:8px; padding:10px 0; text-align:left; font-weight:500; }"
                                "QPushButton:hover { text-decoration:underline; }"
                                "QPushButton:focus { border-color:%2; }"
                                "QPushButton:disabled { color:%3; }")
                                .arg(rgb(colour(*this, Role::Foreground)),
                                     rgb(colour(*this, Role::Ring)),
                                     rgb(colour(*this, Role::MutedForeground))));
    content_->setContentsMargins(0, 0, 0, 10);
    content_->setSpacing(8);
    contentHost_->setVisible(false);
    contentHost_->setMaximumHeight(0);
    outer->addWidget(trigger_);
    outer->addWidget(contentHost_);
    connect(trigger_, &QPushButton::clicked, this, [this] { setExpanded(!expanded_); });
    connect(animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        contentHost_->setMaximumHeight(value.toInt());
    });
    connect(animation_, &QVariantAnimation::finished, this, [this] {
        if (expanded_) contentHost_->setMaximumHeight(QWIDGETSIZE_MAX);
        else contentHost_->setVisible(false);
    });
    trigger_->installEventFilter(this);
}

QString AccordionItem::title() const {
    return trigger_->property("shadcnAccordionTitle").toString();
}

void AccordionItem::setTitle(const QString& title) {
    trigger_->setProperty("shadcnAccordionTitle", title);
    trigger_->setText(title);
}

void AccordionItem::setExpanded(bool expanded) {
    if (expanded_ == expanded) return;
    expanded_ = expanded;
    trigger_->setChecked(expanded_);
    trigger_->setIcon(accordionIcon(*this, expanded_));
    updateContent(true);
    emit expandedChanged(expanded_);
}

void AccordionItem::toggle() { setExpanded(!expanded_); }

QVBoxLayout& AccordionItem::content() { return *content_; }

void AccordionItem::paintEvent(QPaintEvent*) {
    const auto* accordion = qobject_cast<const Accordion*>(parentWidget());
    if (!accordion) return;
    const auto items = accordion->items();
    if (items.isEmpty() || items.last() == this) return;
    QPainter painter(this);
    painter.setPen(colour(*this, Role::Border));
    painter.drawLine(rect().bottomLeft(), rect().bottomRight());
}

void AccordionItem::setContentWidget(QWidget& widget) {
    widget.setParent(contentHost_);
    content_->addWidget(&widget);
    if (expanded_) updateContent(false);
}

void AccordionItem::updateContent(bool animate) {
    const auto target = expanded_ ? std::max(1, contentHost_->sizeHint().height()) : 0;
    contentHost_->setVisible(expanded_ || contentHost_->maximumHeight() > 0);
    animation_->stop();
    const auto current = contentHost_->maximumHeight() == QWIDGETSIZE_MAX
                              ? contentHost_->sizeHint().height()
                              : contentHost_->maximumHeight();
    if (!animate || motionDuration(*this) == 0) {
        contentHost_->setMaximumHeight(expanded_ ? QWIDGETSIZE_MAX : 0);
        contentHost_->setVisible(expanded_);
        return;
    }
    animation_->setDuration(motionDuration(*this));
    animation_->setStartValue(current);
    animation_->setEndValue(target);
    animation_->start();
}

bool AccordionItem::eventFilter(QObject* watched, QEvent* event) {
    if (watched == trigger_ && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        auto* accordion = qobject_cast<Accordion*>(parentWidget());
        if (accordion) {
            const auto all = accordion->items();
            const auto index = static_cast<int>(all.indexOf(this));
            if (index >= 0) {
                int target = -1;
                if (key->key() == Qt::Key_Home) {
                    for (int candidate = 0; candidate < static_cast<int>(all.size()); ++candidate) {
                        if (all.at(candidate) && all.at(candidate)->isEnabled()) {
                            target = candidate;
                            break;
                        }
                    }
                } else if (key->key() == Qt::Key_End) {
                    for (int candidate = static_cast<int>(all.size()) - 1; candidate >= 0; --candidate) {
                        if (all.at(candidate) && all.at(candidate)->isEnabled()) {
                            target = candidate;
                            break;
                        }
                    }
                }
                else if (key->key() == Qt::Key_Down || key->key() == Qt::Key_Right ||
                         key->key() == Qt::Key_Up || key->key() == Qt::Key_Left) {
                    const auto step = key->key() == Qt::Key_Down || key->key() == Qt::Key_Right ? 1 : -1;
                    const auto count = static_cast<int>(all.size());
                    for (int steps = 0, candidate = index; steps < count; ++steps) {
                        candidate = (candidate + step + count) % count;
                        if (all.at(candidate) && all.at(candidate)->isEnabled()) {
                            target = candidate;
                            break;
                        }
                    }
                }
                if (target >= 0 && all.at(target) && all.at(target)->isEnabled()) {
                    all.at(target)->trigger().setFocus(Qt::TabFocusReason);
                    event->accept();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

Accordion::Accordion(QWidget* parent) : QWidget(parent), layout_(new QVBoxLayout(this)) {
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);
    setFocusPolicy(Qt::StrongFocus);
}

AccordionItem& Accordion::addItem(const QString& title) {
    auto* item = new AccordionItem(title, this);
    addItem(*item);
    return *item;
}

void Accordion::addItem(AccordionItem& item) {
    if (items_.contains(&item)) return;
    item.setParent(this);
    layout_->addWidget(&item);
    items_.append(&item);
    connect(&item, &AccordionItem::expandedChanged, this,
            [this, pointer = QPointer<AccordionItem>(&item)](bool expanded) {
                if (pointer) onItemChanged(*pointer, expanded);
            });
}

void Accordion::removeItem(AccordionItem& item) {
    const auto index = items_.indexOf(&item);
    if (index < 0) return;
    items_.removeAt(index);
    layout_->removeWidget(&item);
    item.setParent(nullptr);
}

QList<AccordionItem*> Accordion::items() const {
    QList<AccordionItem*> result;
    for (const auto& item : items_) if (item) result.append(item.data());
    return result;
}

void Accordion::setAllowsMultiple(bool multiple) {
    if (multiple_ == multiple) return;
    multiple_ = multiple;
    if (!multiple_) {
        bool kept = false;
        for (const auto& item : items_) {
            if (!item || !item->isExpanded()) continue;
            if (kept) item->setExpanded(false);
            else kept = true;
        }
    }
}

QStringList Accordion::expandedTitles() const {
    QStringList result;
    for (const auto& item : items_) if (item && item->isExpanded()) result.append(item->title());
    return result;
}

void Accordion::setExpandedTitles(const QStringList& titles) {
    bool kept = false;
    for (const auto& item : items_) {
        if (!item) continue;
        const auto open = titles.contains(item->title()) && (multiple_ || !kept);
        item->setExpanded(open);
        kept = kept || open;
    }
}

void Accordion::onItemChanged(AccordionItem& changed, bool expanded) {
    if (expanded && !multiple_) {
        for (const auto& item : items_) if (item && item != &changed) item->setExpanded(false);
    }
    emit expandedChanged(expandedTitles());
}

Collapsible::Collapsible(const QString& title, QWidget* parent)
    : QWidget(parent), trigger_(new QPushButton(title, this)), contentHost_(new QWidget(this)),
      content_(new QVBoxLayout(contentHost_)), animation_(new QVariantAnimation(this)) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    trigger_->setCheckable(true);
    trigger_->setFocusPolicy(Qt::StrongFocus);
    styleTrigger(*trigger_, *this);
    content_->setContentsMargins(0, 8, 0, 8);
    contentHost_->setVisible(false);
    contentHost_->setMaximumHeight(0);
    outer->addWidget(trigger_);
    outer->addWidget(contentHost_);
    connect(trigger_, &QPushButton::clicked, this, [this] { setOpen(!open_); });
    connect(animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        contentHost_->setMaximumHeight(value.toInt());
    });
    connect(animation_, &QVariantAnimation::finished, this, [this] {
        contentHost_->setMaximumHeight(open_ ? QWIDGETSIZE_MAX : 0);
        contentHost_->setVisible(open_);
    });
}

QVBoxLayout& Collapsible::content() { return *content_; }

void Collapsible::setContentWidget(QWidget& widget) {
    widget.setParent(contentHost_);
    content_->addWidget(&widget);
    if (open_) updateContent(false);
}

void Collapsible::setOpen(bool open) {
    if (open_ == open) return;
    open_ = open;
    trigger_->setChecked(open_);
    updateContent(true);
    emit openChanged(open_);
}

void Collapsible::toggle() { setOpen(!open_); }

void Collapsible::updateContent(bool animate) {
    const auto target = open_ ? std::max(1, contentHost_->sizeHint().height()) : 0;
    contentHost_->setVisible(open_ || contentHost_->maximumHeight() > 0);
    animation_->stop();
    if (!animate || motionDuration(*this) == 0) {
        contentHost_->setMaximumHeight(open_ ? QWIDGETSIZE_MAX : 0);
        contentHost_->setVisible(open_);
        return;
    }
    animation_->setDuration(motionDuration(*this));
    animation_->setStartValue(contentHost_->maximumHeight() == QWIDGETSIZE_MAX
                                   ? contentHost_->sizeHint().height()
                                   : contentHost_->maximumHeight());
    animation_->setEndValue(target);
    animation_->start();
}

Tabs::Tabs(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent), orientation_(orientation), listHost_(new QWidget(this)),
      list_(new QHBoxLayout(listHost_)), stack_(new QStackedWidget(this)) {
    auto* outer = new QBoxLayout(orientation == Qt::Horizontal ? QBoxLayout::TopToBottom
                                                               : QBoxLayout::LeftToRight,
                                 this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(8);
    list_->setContentsMargins(3, 3, 3, 3);
    list_->setSpacing(3);
    listHost_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    stack_->setFrameStyle(QFrame::NoFrame);
    outer->addWidget(listHost_);
    outer->addWidget(stack_, 1);
    setFocusPolicy(Qt::StrongFocus);
    restyle();
}

void Tabs::setOrientation(Qt::Orientation orientation) {
    if (orientation_ == orientation) return;
    orientation_ = orientation;
    if (auto* outer = qobject_cast<QBoxLayout*>(layout())) {
        outer->setDirection(orientation_ == Qt::Horizontal ? QBoxLayout::TopToBottom
                                                           : QBoxLayout::LeftToRight);
    }
    listHost_->setSizePolicy(orientation_ == Qt::Horizontal ? QSizePolicy::Preferred
                                                            : QSizePolicy::Fixed,
                             orientation_ == Qt::Horizontal ? QSizePolicy::Fixed
                                                            : QSizePolicy::Preferred);
    updateGeometry();
}

void Tabs::setListVariant(TabsListVariant variant) {
    if (variant_ == variant) return;
    variant_ = variant;
    restyle();
}

QPushButton& Tabs::addTab(const QString& value, const QString& label) {
    auto* button = new QPushButton(label, listHost_);
    button->setCheckable(true);
    button->setFocusPolicy(Qt::StrongFocus);
    button->setProperty("shadcnTabValue", value);
    entries_.append({value, button, nullptr});
    list_->addWidget(button);
    const QPointer<QPushButton> observed(button);
    connect(button, &QPushButton::clicked, this, [this, observed] {
        for (int candidate = 0; candidate < static_cast<int>(entries_.size()); ++candidate) {
            if (entries_.at(candidate).button == observed) {
                select(candidate, false);
                return;
            }
        }
    });
    button->installEventFilter(this);
    restyle();
    if (current_ < 0) select(0, false);
    return *button;
}

void Tabs::addContent(const QString& value, QWidget& content) {
    const auto index = indexFor(value);
    if (index < 0) return;
    auto& entry = entries_[index];
    if (entry.content && entry.content != &content) {
        stack_->removeWidget(entry.content);
        if (entry.ownedContent) {
            entry.content->deleteLater();
        } else {
            entry.content->setParent(nullptr);
            entry.content->hide();
        }
    }
    content.setParent(stack_);
    stack_->addWidget(&content);
    entry.content = &content;
    entry.ownedContent = false;
    if (index == current_) stack_->setCurrentWidget(&content);
}

QWidget& Tabs::addContent(const QString& value) {
    auto* content = new QWidget(stack_);
    const auto index = indexFor(value);
    addContent(value, *content);
    if (index >= 0 && index < static_cast<int>(entries_.size())) entries_[index].ownedContent = true;
    return *content;
}

void Tabs::removeTab(const QString& value) {
    const auto index = indexFor(value);
    if (index < 0) return;
    auto entry = entries_.takeAt(index);
    if (entry.button) {
        list_->removeWidget(entry.button);
        entry.button->deleteLater();
    }
    if (entry.content) {
        stack_->removeWidget(entry.content);
        if (entry.ownedContent) {
            entry.content->deleteLater();
        } else {
            entry.content->setParent(nullptr);
            entry.content->hide();
        }
    }
    if (entries_.isEmpty()) current_ = -1;
    else {
        if (index < current_) --current_;
        const auto last = static_cast<int>(entries_.size()) - 1;
        current_ = std::clamp(current_, 0, last);
        select(std::min(current_, last), false);
    }
}

QString Tabs::currentValue() const {
    return current_ >= 0 && current_ < static_cast<int>(entries_.size())
               ? entries_.at(current_).value : QString{};
}

void Tabs::setCurrentValue(const QString& value) {
    const auto index = indexFor(value);
    if (index >= 0) select(index, true);
}

QStringList Tabs::values() const {
    QStringList result;
    for (const auto& entry : entries_) result.append(entry.value);
    return result;
}

QHBoxLayout& Tabs::list() { return *list_; }

void Tabs::select(int index, bool focus) {
    if (index < 0 || index >= static_cast<int>(entries_.size())) return;
    const auto previousValue = currentValue();
    current_ = index;
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
        if (entries_.at(i).button) entries_.at(i).button->setChecked(i == current_);
    }
    if (entries_.at(current_).content) stack_->setCurrentWidget(entries_.at(current_).content);
    if (focus && entries_.at(current_).button) entries_.at(current_).button->setFocus(Qt::TabFocusReason);
    if (previousValue != currentValue()) emit currentChanged(currentValue());
}

int Tabs::indexFor(const QString& value) const {
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i)
        if (entries_.at(i).value == value) return i;
    return -1;
}

void Tabs::restyle() {
    const auto listBackground = variant_ == TabsListVariant::Default
                                    ? rgb(colour(*this, Role::Muted))
                                    : QStringLiteral("transparent");
    const auto foreground = rgb(colour(*this, Role::Foreground));
    const auto border = rgb(colour(*this, Role::Border));
    const auto accent = rgb(colour(*this, Role::Background));
    const auto hover = rgb(colour(*this, Role::Accent));
    listHost_->setStyleSheet(QStringLiteral("QWidget { background:%1; border-radius:8px; }").arg(listBackground));
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
        auto* button = entries_.at(i).button.data();
        if (!button) continue;
        button->setStyleSheet(QStringLiteral(
                                  "QPushButton { color:%1; background:transparent; border:1px solid transparent; "
                                  "border-radius:6px; padding:4px 8px; font-weight:500; }"
                                  "QPushButton:hover { color:%1; background:%2; }"
                                  "QPushButton:checked { color:%1; background:%3; border-color:transparent; }"
                                  "QPushButton:focus { border-color:%4; }")
                                  .arg(foreground, hover, accent, border));
    }
}

bool Tabs::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* button = qobject_cast<QPushButton*>(watched);
        if (button) {
            auto* key = static_cast<QKeyEvent*>(event);
            const auto index = indexFor(button->property("shadcnTabValue").toString());
            if (index >= 0) {
                int target = -1;
                const auto rtl = layoutDirection() == Qt::RightToLeft;
                if (key->key() == Qt::Key_Home) {
                    for (int candidate = 0; candidate < static_cast<int>(entries_.size()); ++candidate) {
                        if (entries_.at(candidate).button && entries_.at(candidate).button->isEnabled()) {
                            target = candidate;
                            break;
                        }
                    }
                } else if (key->key() == Qt::Key_End) {
                    for (int candidate = static_cast<int>(entries_.size()) - 1; candidate >= 0; --candidate) {
                        if (entries_.at(candidate).button && entries_.at(candidate).button->isEnabled()) {
                            target = candidate;
                            break;
                        }
                    }
                }
                else if (orientation_ == Qt::Horizontal &&
                         (key->key() == Qt::Key_Right || key->key() == Qt::Key_Left)) {
                    const auto forward = (key->key() == Qt::Key_Right) != rtl;
                    target = nextEnabled([&] {
                        QList<QPointer<QPushButton>> result;
                        for (const auto& entry : entries_) result.append(entry.button);
                        return result;
                    }(), index, forward ? 1 : -1);
                } else if (orientation_ == Qt::Vertical &&
                           (key->key() == Qt::Key_Down || key->key() == Qt::Key_Up)) {
                    target = nextEnabled([&] {
                        QList<QPointer<QPushButton>> result;
                        for (const auto& entry : entries_) result.append(entry.button);
                        return result;
                    }(), index, key->key() == Qt::Key_Down ? 1 : -1);
                }
                if (target >= 0) {
                    select(target, true);
                    event->accept();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

ScrollArea::ScrollArea(QWidget* parent) : QScrollArea(parent) {
    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);
    setFocusPolicy(Qt::StrongFocus);
    const auto border = rgb(colour(*this, Role::Border));
    const auto muted = rgb(colour(*this, Role::Muted));
    setStyleSheet(QStringLiteral(
                      "QScrollBar:vertical { width:10px; background:transparent; margin:0; }"
                      "QScrollBar:horizontal { height:10px; background:transparent; margin:0; }"
                      "QScrollBar::handle { background:%1; border-radius:5px; min-height:24px; min-width:24px; }"
                      "QScrollBar::handle:hover { background:%2; }"
                      "QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0; }"
                      "QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }")
                      .arg(border, muted));
}

void ScrollArea::setHorizontalScrollBarVisible(bool visible) {
    horizontalScrollBar()->setVisible(visible);
}

void ScrollArea::setVerticalScrollBarVisible(bool visible) {
    verticalScrollBar()->setVisible(visible);
}

void ScrollArea::paintEvent(QPaintEvent* event) {
    QScrollArea::paintEvent(event);
    QPainter painter(this);
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5), themeFor(*this).radius(),
            Qt::transparent, colour(*this, Role::Border));
}

ResizablePanelGroup::ResizablePanelGroup(Qt::Orientation orientation, QWidget* parent)
    : QSplitter(orientation, parent), orientation_(orientation) {
    setChildrenCollapsible(false);
    setOpaqueResize(true);
    setHandleWidth(6);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(QStringLiteral("QSplitter::handle { background:%1; }").arg(rgb(colour(*this, Role::Border))));
}

void ResizablePanelGroup::setOrientation(Qt::Orientation orientation) {
    if (orientation_ == orientation) return;
    orientation_ = orientation;
    QSplitter::setOrientation(orientation_);
}

void ResizablePanelGroup::addPanel(QWidget& panel) {
    panel.setParent(this);
    addWidget(&panel);
}

void ResizablePanelGroup::setPanelSizes(const QList<int>& sizes) {
    QList<int> native;
    native.reserve(sizes.size());
    for (const auto size : sizes) native.append(std::max(1, size));
    QSplitter::setSizes(native);
}

QList<int> ResizablePanelGroup::panelSizes() const { return QSplitter::sizes(); }

void ResizablePanelGroup::setHandleVisible(bool visible) {
    handleVisible_ = visible;
    setHandleWidth(visible ? 6 : 0);
}

QSplitterHandle* ResizablePanelGroup::createHandle() {
    return new ResizableHandle(orientation_, this);
}

ResizableHandle::ResizableHandle(Qt::Orientation orientation, QSplitter* parent)
    : QSplitterHandle(orientation, parent) {
    setFocusPolicy(Qt::StrongFocus);
    setCursor(orientation == Qt::Horizontal ? Qt::SplitHCursor : Qt::SplitVCursor);
}

void ResizableHandle::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), colour(*this, Role::Border));
    painter.setBrush(colour(*this, Role::MutedForeground));
    painter.setPen(Qt::NoPen);
    if (orientation() == Qt::Horizontal) {
        painter.drawRoundedRect(QRectF(width() / 2.0 - 2, height() / 2.0 - 12, 4, 24), 2, 2);
    } else {
        painter.drawRoundedRect(QRectF(width() / 2.0 - 12, height() / 2.0 - 2, 24, 4), 2, 2);
    }
}

Sidebar::Sidebar(QWidget* parent)
    : QFrame(parent), headerHost_(new QWidget(this)), contentHost_(new QWidget(this)),
      footerHost_(new QWidget(this)), header_(new QVBoxLayout(headerHost_)),
      content_(new QVBoxLayout(contentHost_)), footer_(new QVBoxLayout(footerHost_)),
      animation_(new QVariantAnimation(this)) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->setSpacing(8);
    header_->setContentsMargins(0, 0, 0, 0);
    content_->setContentsMargins(0, 0, 0, 0);
    footer_->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(headerHost_);
    outer->addWidget(contentHost_, 1);
    outer->addWidget(footerHost_);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setMinimumWidth(0);
    setMaximumWidth(expandedWidth_);
    setProperty("shadcnRadius", 10.0);
    connect(animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        const auto width = qMax(0, value.toInt());
        setMinimumWidth(width);
        setMaximumWidth(width);
        update();
    });
    updateWidth(false);
}

void Sidebar::setSide(SidebarSide side) {
    if (side_ == side) return;
    side_ = side;
    update();
}

void Sidebar::setCollapsible(SidebarCollapsible mode) {
    if (collapsible_ == mode) return;
    collapsible_ = mode;
    if (mode == SidebarCollapsible::None) open_ = true;
    updateWidth(true);
}

void Sidebar::setVariant(SidebarVariant variant) {
    if (variant_ == variant) return;
    variant_ = variant;
    update();
}

void Sidebar::setOpen(bool open) {
    if (collapsible_ == SidebarCollapsible::None) open = true;
    if (open_ == open) return;
    open_ = open;
    updateWidth(true);
    emit openChanged(open_);
}

void Sidebar::toggle() { setOpen(!open_); }

void Sidebar::setExpandedWidth(int width) {
    expandedWidth_ = std::max(1, width);
    if (open_ && collapsible_ != SidebarCollapsible::Icon) updateWidth(false);
}

void Sidebar::setIconWidth(int width) {
    iconWidth_ = std::max(1, width);
    if (collapsible_ == SidebarCollapsible::Icon) updateWidth(false);
}

QVBoxLayout& Sidebar::header() { return *header_; }
QVBoxLayout& Sidebar::content() { return *content_; }
QVBoxLayout& Sidebar::footer() { return *footer_; }

QPushButton& Sidebar::addMenuButton(const QString& text, bool active,
                                    SidebarMenuVariant variant, SidebarMenuSize size) {
    auto* button = new QPushButton(text, contentHost_);
    button->setCheckable(true);
    button->setChecked(active);
    button->setFocusPolicy(Qt::StrongFocus);
    button->setProperty("shadcnSidebarText", text);
    const auto height = size == SidebarMenuSize::Sm ? 28 : size == SidebarMenuSize::Lg ? 48 : 32;
    button->setMinimumHeight(height);
    const auto background = variant == SidebarMenuVariant::Outline
                                ? rgb(colour(*this, Role::Background))
                                : QStringLiteral("transparent");
    button->setStyleSheet(QStringLiteral(
                              "QPushButton { color:%1; background:%2; border:1px solid %3; "
                              "border-radius:6px; padding:4px 8px; text-align:left; }"
                              "QPushButton:hover, QPushButton:checked { color:%4; background:%5; "
                              "border-color:%5; }"
                              "QPushButton:focus { border-color:%6; }")
                              .arg(rgb(colour(*this, Role::SidebarForeground)), background,
                                   rgb(colour(*this, Role::SidebarBorder)),
                                   rgb(colour(*this, Role::SidebarAccentForeground)),
                                   rgb(colour(*this, Role::SidebarAccent)),
                                   rgb(colour(*this, Role::SidebarRing))));
    content_->addWidget(button);
    return *button;
}

void Sidebar::updateWidth(bool animate) {
    const auto target = collapsible_ == SidebarCollapsible::None
                            ? expandedWidth_
                            : collapsible_ == SidebarCollapsible::Icon
                                  ? (open_ ? expandedWidth_ : iconWidth_)
                                  : (open_ ? expandedWidth_ : 0);
    animation_->stop();
    if (!animate || motionDuration(*this, 200) == 0) {
        setMinimumWidth(target);
        setMaximumWidth(target);
        widthAmount_ = target == 0 ? 0 : 1;
        return;
    }
    const auto current = width();
    animation_->setDuration(motionDuration(*this, 200));
    animation_->setStartValue(current);
    animation_->setEndValue(target);
    animation_->start();
}

void Sidebar::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const auto fill = variant_ == SidebarVariant::Inset ? colour(*this, Role::Background)
                                                        : colour(*this, Role::Sidebar);
    const auto border = colour(*this, Role::SidebarBorder);
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5), 10, fill, border);
}

bool Sidebar::event(QEvent* event) {
    const auto result = QFrame::event(event);
    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange) update();
    return result;
}

SidebarProvider::SidebarProvider(QWidget* parent)
    : QWidget(parent), layout_(new QHBoxLayout(this)), content_(nullptr) {
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);
    auto* contentHost = new QWidget(this);
    content_ = new QVBoxLayout(contentHost);
    content_->setContentsMargins(16, 16, 16, 16);
    content_->setSpacing(16);
    layout_->addWidget(contentHost, 1);
    setFocusPolicy(Qt::StrongFocus);
    installEventFilter(this);
    auto* shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_B), this);
    shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut, &QShortcut::activated, this, [this] {
        if (shortcutEnabled_) toggleSidebar();
    });
}

void SidebarProvider::addSidebar(Sidebar& sidebar) {
    if (sidebars_.contains(&sidebar)) return;
    sidebar.setParent(this);
    const auto insertAtStart = sidebar.side() == SidebarSide::Left;
    layout_->insertWidget(insertAtStart ? 0 : layout_->count() - 1, &sidebar);
    sidebars_.append(&sidebar);
}

QList<Sidebar*> SidebarProvider::sidebars() const {
    QList<Sidebar*> result;
    for (const auto& sidebar : sidebars_) if (sidebar) result.append(sidebar.data());
    return result;
}

void SidebarProvider::setShortcutEnabled(bool enabled) { shortcutEnabled_ = enabled; }

void SidebarProvider::toggleSidebar() {
    for (const auto& sidebar : sidebars_) if (sidebar) sidebar->toggle();
}

QVBoxLayout& SidebarProvider::content() { return *content_; }

bool SidebarProvider::eventFilter(QObject* watched, QEvent* event) {
    if (watched == this && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (shortcutEnabled_ && key->key() == Qt::Key_B &&
            (key->modifiers() & (Qt::ControlModifier | Qt::MetaModifier))) {
            toggleSidebar();
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

SidebarTrigger::SidebarTrigger(QWidget* parent) : Button(QString::fromUtf8("☰"), parent) {
    setVariant(Variant::Ghost);
    setButtonSize(ButtonSize::IconSm);
    setAccessibleName(tr("Toggle sidebar"));
    connect(this, &Button::clicked, this, [this] {
        auto* ancestor = parentWidget();
        while (ancestor) {
            if (auto* provider = qobject_cast<SidebarProvider*>(ancestor)) {
                provider->toggleSidebar();
                return;
            }
            ancestor = ancestor->parentWidget();
        }
    });
}

void SidebarTrigger::mouseReleaseEvent(QMouseEvent* event) {
    Button::mouseReleaseEvent(event);
}

SidebarInset::SidebarInset(QWidget* parent) : QFrame(parent), content_(new QVBoxLayout(this)) {
    content_->setContentsMargins(16, 16, 16, 16);
    content_->setSpacing(16);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

NavigationMenu::NavigationMenu(QWidget* parent)
    : QFrame(parent), list_(new QHBoxLayout(this)),
      popup_(new QFrame(this, Qt::Popup | Qt::FramelessWindowHint)),
      popupLayout_(new QVBoxLayout(popup_)) {
    list_->setContentsMargins(0, 0, 0, 0);
    list_->setSpacing(2);
    popupLayout_->setContentsMargins(8, 8, 8, 8);
    popupLayout_->setSpacing(4);
    popup_->setVisible(false);
    popup_->setFocusPolicy(Qt::StrongFocus);
    popup_->installEventFilter(this);
    popup_->setFrameStyle(QFrame::StyledPanel);
    popup_->setStyleSheet(QStringLiteral("QFrame { background:%1; border:1px solid %2; border-radius:8px; }")
                              .arg(rgb(colour(*this, Role::Popover)), rgb(colour(*this, Role::Border))));
    setMinimumHeight(36);
}

QPushButton& NavigationMenu::addLink(const QString& text, const QString& value) {
    auto* button = new QPushButton(text, this);
    button->setFocusPolicy(Qt::StrongFocus);
    button->setProperty("shadcnNavigationValue", value.isEmpty() ? text : value);
    styleTrigger(*button, *this);
    entries_.append({button->property("shadcnNavigationValue").toString(), button, nullptr, false});
    list_->addWidget(button);
    const QPointer<QPushButton> observed(button);
    connect(button, &QPushButton::clicked, this, [this, observed] {
        for (int candidate = 0; candidate < static_cast<int>(entries_.size()); ++candidate) {
            if (entries_.at(candidate).button == observed) {
                choose(candidate);
                return;
            }
        }
    });
    button->installEventFilter(this);
    return *button;
}

QPushButton& NavigationMenu::addMenu(const QString& text, const QString& value) {
    auto& button = addLink(text, value);
    entries_.last().menu = true;
    return button;
}

void NavigationMenu::setMenuContent(const QString& value, QWidget& content) {
    for (int index = 0; index < static_cast<int>(entries_.size()); ++index) {
        auto& entry = entries_[index];
        if (entry.value != value || !entry.menu) continue;
        content.setParent(popup_);
        popupLayout_->addWidget(&content);
        entry.content = &content;
        content.setVisible(current_ == index && popup_->isVisible());
        return;
    }
}

void NavigationMenu::setCurrentValue(const QString& value) {
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i)
        if (entries_.at(i).value == value) choose(i);
}

QString NavigationMenu::currentValue() const {
    return current_ >= 0 && current_ < static_cast<int>(entries_.size())
               ? entries_.at(current_).value : QString{};
}

QHBoxLayout& NavigationMenu::list() { return *list_; }

void NavigationMenu::choose(int index) {
    if (index < 0 || index >= static_cast<int>(entries_.size()) ||
        !entries_.at(index).button->isEnabled()) return;
    const auto& entry = entries_.at(index);
    if (entry.menu) {
        if (current_ == index && popup_->isVisible()) {
            popup_->hide();
            return;
        }
        current_ = index;
        for (int candidate = 0; candidate < static_cast<int>(entries_.size()); ++candidate) {
            if (entries_.at(candidate).content)
                entries_.at(candidate).content->setVisible(candidate == current_);
        }
        popup_->adjustSize();
        const auto popupSize = popup_->sizeHint().expandedTo(QSize(220, 1));
        const auto anchor = entry.button->mapToGlobal(QPoint(0, entry.button->height()));
        const auto top = entry.button->mapToGlobal(QPoint(0, 0));
        auto* screen = QApplication::screenAt(anchor);
        if (!screen) screen = QApplication::primaryScreen();
        const auto available = screen ? screen->availableGeometry()
                                      : QRect(anchor, QSize(popupSize.width(), popupSize.height()));
        const auto maxX = std::max(available.left(), available.right() - popupSize.width() + 1);
        const auto maxY = std::max(available.top(), available.bottom() - popupSize.height() + 1);
        auto x = std::clamp(anchor.x(), available.left(), maxX);
        auto y = anchor.y();
        if (y + popupSize.height() > available.bottom() + 1) y = top.y() - popupSize.height();
        y = std::clamp(y, available.top(), maxY);
        popup_->setGeometry(x, y, popupSize.width(), popupSize.height());
        popup_->show();
        popup_->raise();
        popup_->setFocus(Qt::PopupFocusReason);
    } else {
        popup_->hide();
        current_ = index;
        for (auto& item : entries_)
            if (item.content) item.content->hide();
    }
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i)
        entries_.at(i).button->setChecked(i == current_);
    emit currentChanged(currentValue());
}

bool NavigationMenu::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        if (watched == popup_) {
            auto* key = static_cast<QKeyEvent*>(event);
            if (key->key() == Qt::Key_Escape) {
                popup_->hide();
                if (current_ >= 0 && current_ < static_cast<int>(entries_.size()) &&
                    entries_.at(current_).button) {
                    entries_.at(current_).button->setFocus(Qt::PopupFocusReason);
                }
                event->accept();
                return true;
            }
        }
        auto* button = qobject_cast<QPushButton*>(watched);
        if (button) {
            auto* key = static_cast<QKeyEvent*>(event);
            int index = -1;
            for (int candidate = 0; candidate < static_cast<int>(entries_.size()); ++candidate) {
                if (entries_.at(candidate).button == button) {
                    index = candidate;
                    break;
                }
            }
            if (index >= 0) {
                int target = -1;
                if (key->key() == Qt::Key_Home) {
                    for (int candidate = 0; candidate < static_cast<int>(entries_.size()); ++candidate) {
                        if (entries_.at(candidate).button && entries_.at(candidate).button->isEnabled()) {
                            target = candidate;
                            break;
                        }
                    }
                } else if (key->key() == Qt::Key_End) {
                    for (int candidate = static_cast<int>(entries_.size()) - 1; candidate >= 0; --candidate) {
                        if (entries_.at(candidate).button && entries_.at(candidate).button->isEnabled()) {
                            target = candidate;
                            break;
                        }
                    }
                }
                else if (key->key() == Qt::Key_Right || key->key() == Qt::Key_Left) {
                    const auto rtl = layoutDirection() == Qt::RightToLeft;
                    const auto forward = (key->key() == Qt::Key_Right) != rtl;
                    int candidate = index;
                    const auto count = static_cast<int>(entries_.size());
                    for (int steps = 0; steps < count; ++steps) {
                        candidate = (candidate + (forward ? 1 : -1) + count) % count;
                        if (entries_.at(candidate).button && entries_.at(candidate).button->isEnabled()) {
                            target = candidate;
                            break;
                        }
                    }
                }
                if (target >= 0 && entries_.at(target).button && entries_.at(target).button->isEnabled()) {
                    entries_.at(target).button->setFocus(Qt::TabFocusReason);
                    choose(target);
                    event->accept();
                    return true;
                }
                if (key->key() == Qt::Key_Escape && popup_->isVisible()) {
                    popup_->hide();
                    event->accept();
                    return true;
                }
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

Menubar::Menubar(QWidget* parent) : QMenuBar(parent) {
    setNativeMenuBar(false);
    setMouseTracking(true);
    setStyleSheet(QStringLiteral(
                      "QMenuBar { background:%1; color:%2; border:1px solid %3; border-radius:8px; "
                      "padding:3px; spacing:2px; }"
                      "QMenuBar::item { padding:3px 8px; border-radius:5px; }"
                      "QMenuBar::item:selected, QMenuBar::item:pressed { background:%4; color:%5; }")
                      .arg(rgb(colour(*this, Role::Background)), rgb(colour(*this, Role::Foreground)),
                           rgb(colour(*this, Role::Border)), rgb(colour(*this, Role::Muted)),
                           rgb(colour(*this, Role::Foreground))));
}

void Menubar::paintEvent(QPaintEvent* event) {
    QMenuBar::paintEvent(event);
}

DropdownMenu::DropdownMenu(QWidget* parent) : QMenu(parent), radioGroup_(new QActionGroup(this)) {
    setStyleSheet(menuStyle(*this));
    setSeparatorsCollapsible(false);
    radioGroup_->setExclusive(true);
}

QAction& DropdownMenu::addItem(const QString& text, const std::function<void()>& callback) {
    auto* action = QMenu::addAction(text);
    if (callback) connect(action, &QAction::triggered, this, [callback] { callback(); });
    return *action;
}

QAction& DropdownMenu::addCheckboxItem(const QString& text, bool checked,
                                       const std::function<void(bool)>& callback) {
    auto* action = QMenu::addAction(text);
    action->setCheckable(true);
    action->setChecked(checked);
    if (callback) connect(action, &QAction::toggled, this, callback);
    return *action;
}

QAction& DropdownMenu::addRadioItem(const QString& text, bool checked,
                                    const std::function<void()>& callback) {
    auto* action = QMenu::addAction(text);
    action->setCheckable(true);
    action->setChecked(checked);
    radioGroup_->addAction(action);
    if (callback) connect(action, &QAction::triggered, this, [callback] { callback(); });
    return *action;
}

QAction& DropdownMenu::addLabel(const QString& text) {
    auto* action = QMenu::addAction(text);
    action->setEnabled(false);
    return *action;
}

void DropdownMenu::addSeparatorLine() { addSeparator(); }

QMenu& DropdownMenu::addSubmenu(const QString& text) { return *QMenu::addMenu(text); }

ContextMenu::ContextMenu(QWidget* parent) : DropdownMenu(parent) {}

void ContextMenu::attach(QWidget& target) {
    detach();
    target_ = &target;
    target.installEventFilter(this);
}

void ContextMenu::detach() {
    if (target_) target_->removeEventFilter(this);
    target_.clear();
}

void ContextMenu::popupAt(const QPoint& globalPosition) { popup(globalPosition); }

bool ContextMenu::eventFilter(QObject* watched, QEvent* event) {
    if (watched == target_ && event->type() == QEvent::ContextMenu) {
        auto* context = static_cast<QContextMenuEvent*>(event);
        popup(context->globalPos());
        context->accept();
        return true;
    }
    return DropdownMenu::eventFilter(watched, event);
}

Carousel::Carousel(Qt::Orientation orientation, QWidget* parent)
    : QFrame(parent), orientation_(orientation), stack_(new QStackedWidget(this)),
      previous_(new Button(QString::fromUtf8("‹"), this)), next_(new Button(QString::fromUtf8("›"), this)) {
    auto* outer = new QBoxLayout(orientation == Qt::Horizontal ? QBoxLayout::LeftToRight
                                                                : QBoxLayout::TopToBottom,
                                 this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(8);
    previous_->setVariant(Variant::Outline);
    previous_->setButtonSize(ButtonSize::IconSm);
    next_->setVariant(Variant::Outline);
    next_->setButtonSize(ButtonSize::IconSm);
    previous_->setAccessibleName(tr("Previous slide"));
    next_->setAccessibleName(tr("Next slide"));
    stack_->setFocusPolicy(Qt::StrongFocus);
    setFocusPolicy(Qt::StrongFocus);
    if (orientation_ == Qt::Horizontal) {
        outer->addWidget(previous_);
        outer->addWidget(stack_, 1);
        outer->addWidget(next_);
    } else {
        outer->addWidget(previous_, 0, Qt::AlignHCenter);
        outer->addWidget(stack_, 1);
        outer->addWidget(next_, 0, Qt::AlignHCenter);
    }
    connect(previous_, &Button::clicked, this, &Carousel::scrollPrevious);
    connect(next_, &Button::clicked, this, &Carousel::scrollNext);
    connect(stack_, &QStackedWidget::currentChanged, this, [this](int index) {
        current_ = index;
        updateButtons();
        emit currentChanged(index);
    });
    updateButtons();
}

void Carousel::setOrientation(Qt::Orientation orientation) {
    if (orientation_ == orientation) return;
    orientation_ = orientation;
    if (auto* outer = qobject_cast<QBoxLayout*>(layout())) {
        outer->setDirection(orientation_ == Qt::Horizontal ? QBoxLayout::LeftToRight
                                                           : QBoxLayout::TopToBottom);
    }
}

void Carousel::addSlide(QWidget& slide) {
    slide.setParent(stack_);
    stack_->addWidget(&slide);
    if (current_ < 0) setCurrentIndex(0);
    updateButtons();
}

void Carousel::removeSlide(QWidget& slide) {
    const auto index = stack_->indexOf(&slide);
    if (index < 0) return;
    stack_->removeWidget(&slide);
    slide.setParent(nullptr);
    slide.hide();
    if (stack_->count() == 0) current_ = -1;
    else setCurrentIndex(std::min(current_, stack_->count() - 1));
    updateButtons();
}

void Carousel::setCurrentIndex(int index) {
    if (index < 0 || index >= stack_->count()) return;
    stack_->setCurrentIndex(index);
}

void Carousel::scrollPrevious() {
    if (!canScrollPrevious()) return;
    setCurrentIndex(current_ - 1);
}

void Carousel::scrollNext() {
    if (!canScrollNext()) return;
    setCurrentIndex(current_ + 1);
}

bool Carousel::canScrollPrevious() const noexcept { return current_ > 0; }
bool Carousel::canScrollNext() const noexcept { return current_ >= 0 && current_ + 1 < stack_->count(); }

void Carousel::updateButtons() {
    previous_->setEnabled(canScrollPrevious());
    next_->setEnabled(canScrollNext());
}

void Carousel::keyPressEvent(QKeyEvent* event) {
    const auto rtl = layoutDirection() == Qt::RightToLeft;
    if (orientation_ == Qt::Horizontal &&
        (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)) {
        const auto next = (event->key() == Qt::Key_Right) != rtl;
        if (next) scrollNext(); else scrollPrevious();
        event->accept();
        return;
    }
    if (orientation_ == Qt::Vertical &&
        (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)) {
        if (event->key() == Qt::Key_Down) scrollNext(); else scrollPrevious();
        event->accept();
        return;
    }
    QFrame::keyPressEvent(event);
}

Pagination::Pagination(QWidget* parent) : QFrame(parent), layout_(new QHBoxLayout(this)) {
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(3);
    previousText_ = tr("Previous");
    nextText_ = tr("Next");
    rebuild();
}

void Pagination::setPageCount(int count) {
    const auto next = std::max(1, count);
    if (pageCount_ == next) return;
    pageCount_ = next;
    currentPage_ = std::min(currentPage_, pageCount_);
    rebuild();
}

void Pagination::setCurrentPage(int page) {
    const auto next = std::clamp(page, 1, pageCount_);
    if (currentPage_ == next) return;
    currentPage_ = next;
    rebuild();
    emit pageChanged(currentPage_);
}

void Pagination::setShowEllipsis(bool show) {
    if (showEllipsis_ == show) return;
    showEllipsis_ = show;
    rebuild();
}

void Pagination::setPreviousText(const QString& text) {
    if (previousText_ == text) return;
    previousText_ = text;
    rebuild();
}

void Pagination::setNextText(const QString& text) {
    if (nextText_ == text) return;
    nextText_ = text;
    rebuild();
}

void Pagination::rebuild() {
    while (layout_->count() > 0) {
        auto* item = layout_->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    const auto addPage = [this](int page) {
        auto* button = new Button(QString::number(page), this);
        button->setButtonSize(ButtonSize::IconSm);
        button->setVariant(page == currentPage_ ? Variant::Outline : Variant::Ghost);
        button->setAccessibleName(tr("Go to page %1").arg(page));
        connect(button, &Button::clicked, this, [this, page] { setCurrentPage(page); });
        layout_->addWidget(button);
    };
    auto* previous = new Button(previousText_, this);
    previous->setVariant(Variant::Ghost);
    previous->setButtonSize(ButtonSize::Sm);
    previous->setEnabled(currentPage_ > 1);
    previous->setAccessibleName(tr("Go to previous page"));
    connect(previous, &Button::clicked, this, [this] { setCurrentPage(currentPage_ - 1); });
    layout_->addWidget(previous);

    if (!showEllipsis_ || pageCount_ <= 7) {
        for (int page = 1; page <= pageCount_; ++page) addPage(page);
    } else {
        addPage(1);
        if (currentPage_ > 4) {
            auto* ellipsis = new QLabel(QString::fromUtf8("…"), this);
            ellipsis->setAccessibleName(tr("More pages"));
            layout_->addWidget(ellipsis);
        }
        const auto first = std::max(2, currentPage_ - 1);
        const auto last = std::min(pageCount_ - 1, currentPage_ + 1);
        for (int page = first; page <= last; ++page) addPage(page);
        if (currentPage_ < pageCount_ - 3) {
            auto* ellipsis = new QLabel(QString::fromUtf8("…"), this);
            ellipsis->setAccessibleName(tr("More pages"));
            layout_->addWidget(ellipsis);
        }
        addPage(pageCount_);
    }
    auto* next = new Button(nextText_, this);
    next->setVariant(Variant::Ghost);
    next->setButtonSize(ButtonSize::Sm);
    next->setEnabled(currentPage_ < pageCount_);
    next->setAccessibleName(tr("Go to next page"));
    connect(next, &Button::clicked, this, [this] { setCurrentPage(currentPage_ + 1); });
    layout_->addWidget(next);
}

} // namespace shadcn
