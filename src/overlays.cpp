// SPDX-License-Identifier: MIT
// Adapted from shadcn/ui Radix components and Nova styles.
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QVBoxLayout>
#include <QVariantAnimation>
#include <algorithm>
#include <shadcn/overlays.hpp>

namespace shadcn {
namespace {
const Theme& theme(const QWidget& widget) {
    if (const auto* style = qobject_cast<const Style*>(widget.style()))
        return style->theme();
    if (const auto* style = qobject_cast<const Style*>(QApplication::style()))
        return style->theme();
    static const Theme fallback = Theme::neutral();
    return fallback;
}
QColor colour(const QWidget& widget, Role role) {
    const auto c = theme(widget).color(role);
    return QColor::fromRgbF(float(c.r), float(c.g), float(c.b), float(c.a));
}
bool reduced(const QWidget& widget) {
    const auto* style = qobject_cast<const Style*>(widget.style());
    if (!style) style = qobject_cast<const Style*>(QApplication::style());
    return style && style->motion() == MotionPolicy::Reduced;
}
QString css(const QWidget& widget, Role role) {
    const auto c = colour(widget, role);
    return QString("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}
} // namespace

Dialog::Dialog(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint), panel_(new QFrame(this)),
      panelLayout_(new QVBoxLayout(panel_)), title_(new QLabel(panel_)),
      description_(new QLabel(panel_)), close_(new Button({}, panel_)),
      content_(new QVBoxLayout), footer_(new QHBoxLayout), footerHost_(new QFrame(panel_)), animation_(new QVariantAnimation(this)),
      opacity_(new QGraphicsOpacityEffect(panel_)) {
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowModality(Qt::WindowModal);
    setModal(true);
    setSizeGripEnabled(false);
    panel_->setObjectName("shadcnDialogPanel");
    panel_->setGraphicsEffect(opacity_);
    panelLayout_->setContentsMargins(0, 0, 0, 0);
    panelLayout_->setSpacing(16);
    auto* header = new QVBoxLayout;
    header->setContentsMargins(16, 16, 16, 0);
    header->setSpacing(8);
    auto f = title_->font();
    f.setPixelSize(16);
    f.setWeight(QFont::Medium);
    title_->setFont(f);
    title_->setTextFormat(Qt::PlainText);
    description_->setTextFormat(Qt::PlainText);
    title_->setWordWrap(true);
    description_->setWordWrap(true);
    title_->setContentsMargins(0, 0, 28, 0);
    title_->hide();
    description_->hide();
    header->addWidget(title_);
    header->addWidget(description_);
    panelLayout_->addLayout(header);
    panelLayout_->addLayout(content_, 1);
    footerHost_->setObjectName("shadcnDialogFooter");
    footerHost_->setLayout(footer_);
    panelLayout_->addWidget(footerHost_);
    content_->setContentsMargins(16, 0, 16, 0);
    content_->setSpacing(16);
    footer_->setSpacing(8);
    footer_->setContentsMargins(16, 16, 16, 16);
    footer_->addStretch();
    close_->setVariant(Variant::Ghost);
    close_->setButtonSize(ButtonSize::IconSm);
    close_->setAccessibleName(tr("Close"));
    connect(close_, &Button::clicked, this, &Dialog::reject);
    connect(animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        amount_ = value.toDouble();
        opacity_->setOpacity(amount_);
        arrangePanel();
        update();
    });
    connect(animation_, &QVariantAnimation::finished, this, [this] {
        if (closing_) {
            closing_ = false;
            QDialog::done(result_);
        }
    });
    refreshTheme();
}
void Dialog::setTitle(const QString& title) {
    title_->setText(title);
    title_->setVisible(!title.isEmpty());
    setWindowTitle(title);
    setAccessibleName(title);
    arrangePanel();
}
void Dialog::setDescription(const QString& text) {
    description_->setText(text);
    description_->setVisible(!text.isEmpty());
    setAccessibleDescription(text);
    arrangePanel();
}
void Dialog::setCloseButtonVisible(bool visible) {
    close_->setVisible(visible);
}
void Dialog::setContentWidth(int width) {
    contentWidth_ = std::max(1, width);
    arrangePanel();
}
void Dialog::refreshTheme() {
    const bool sheet = qobject_cast<Sheet*>(this);
    const bool drawer = qobject_cast<Drawer*>(this);
    const auto radius = theme(*this).radius() * 1.4;
    const auto corners = sheet ? (drawer ? QString("border-top-left-radius: %1px; border-top-right-radius: %1px;").arg(radius)
                                         : QString{})
                               : QString("border-radius: %1px;").arg(radius);
    panel_->setStyleSheet(QString("QFrame#shadcnDialogPanel { background: %1; color: %2; border: "
                                  "1px solid %3; %4 }")
                              .arg(css(*this, Role::Popover), css(*this, Role::PopoverForeground),
                                   css(*this, Role::Border), corners));
    auto muted = colour(*this, Role::Muted);
    muted.setAlphaF(.5f);
    const auto fill = QString("rgba(%1,%2,%3,%4)").arg(muted.red()).arg(muted.green()).arg(muted.blue()).arg(muted.alpha());
    footerHost_->setStyleSheet(sheet ? QString{} :
        QString("QFrame#shadcnDialogFooter { background: %1; border-top: 1px solid %2; border-bottom-left-radius: %3px; border-bottom-right-radius: %3px; }")
            .arg(fill, css(*this, Role::Border)).arg(radius));
    QPixmap image(32, 32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(colour(*this, Role::Foreground), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(9, 9, 23, 23);
    painter.drawLine(23, 9, 9, 23);
    painter.end();
    close_->setIcon(QIcon(image));
    auto p = description_->palette();
    p.setColor(QPalette::WindowText, colour(*this, Role::MutedForeground));
    description_->setPalette(p);
}
void Dialog::fitOwner() {
    if (owner_) {
        resize(owner_->size());
        move(owner_->mapToGlobal(QPoint{}));
    } else {
        const auto area = screen()->availableGeometry();
        resize(area.size());
        move(area.topLeft());
    }
    arrangePanel();
}
QRect Dialog::panelGeometry() const {
    const int w = std::min(contentWidth_, std::max(1, width() - 32));
    const int h = std::min(std::max(120, panel_->sizeHint().height()), std::max(1, height() - 32));
    return {(width() - w) / 2, (height() - h) / 2, w, h};
}
void Dialog::arrangePanel() {
    if (!footerHost_) return;
    footerHost_->setVisible(footer_->count() > 1);
    content_->setContentsMargins(16, 0, 16, footerHost_->isHidden() ? 16 : 0);
    auto target = panelGeometry();
    const auto offset = animationOffset();
    target.translate(
        QPoint(qRound(offset.x() * (1 - amount_)), qRound(offset.y() * (1 - amount_))));
    panel_->setGeometry(target);
    close_->resize(close_->sizeHint());
    close_->move(panel_->width() - close_->width() - 8, 8);
    close_->raise();
}
void Dialog::transition(double target) {
    animation_->stop();
    animation_->setDuration(reduced(*this) ? 0 : duration_);
    animation_->setStartValue(amount_);
    animation_->setEndValue(target);
    animation_->setEasingCurve(QEasingCurve::InOutCubic);
    animation_->start();
}
void Dialog::showEvent(QShowEvent* event) {
    previousFocus_ = QApplication::focusWidget();
    if (parentWidget()) {
        owner_ = parentWidget()->window();
        owner_->installEventFilter(this);
    }
    closing_ = false;
    amount_ = reduced(*this) ? 1 : 0;
    opacity_->setOpacity(amount_);
    refreshTheme();
    fitOwner();
    QDialog::showEvent(event);
    transition(1);
}
void Dialog::hideEvent(QHideEvent* event) {
    animation_->stop();
    if (owner_)
        owner_->removeEventFilter(this);
    QDialog::hideEvent(event);
    if (previousFocus_ && previousFocus_->isVisible() && previousFocus_->isEnabled())
        previousFocus_->setFocus(Qt::PopupFocusReason);
}
void Dialog::done(int result) {
    if (!isVisible() || reduced(*this)) {
        animation_->stop();
        closing_ = false;
        QDialog::done(result);
        return;
    }
    result_ = result;
    closing_ = true;
    transition(0);
}
void Dialog::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    arrangePanel();
}
void Dialog::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, qRound(26 * amount_)));
}
void Dialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && outsideDismiss_ &&
        !panel_->geometry().contains(event->position().toPoint()))
        reject();
    else
        QDialog::mousePressEvent(event);
}
void Dialog::changeEvent(QEvent* event) {
    QDialog::changeEvent(event);
    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange)
        refreshTheme();
}
bool Dialog::eventFilter(QObject* object, QEvent* event) {
    if (object == owner_ && (event->type() == QEvent::Resize || event->type() == QEvent::Move))
        fitOwner();
    return QDialog::eventFilter(object, event);
}

AlertDialog::AlertDialog(QWidget* parent)
    : Dialog(parent), action_(new Button(tr("Continue"), panel_)),
      cancel_(new Button(tr("Cancel"), panel_)) {
    setContentWidth(384);
    setDismissOnOutsideClick(false);
    setCloseButtonVisible(false);
    cancel_->setVariant(Variant::Outline);
    footer().addWidget(cancel_);
    footer().addWidget(action_);
    connect(action_, &Button::clicked, this, &AlertDialog::accept);
    connect(cancel_, &Button::clicked, this, &AlertDialog::reject);
}
void AlertDialog::showEvent(QShowEvent* event) {
    Dialog::showEvent(event);
    cancel_->setFocus(Qt::PopupFocusReason);
}

Sheet::Sheet(Side side, QWidget* parent) : Dialog(parent), side_(side) {
    contentWidth_ = 384;
    duration_ = 200;
    panelLayout_->setSpacing(16);
    refreshTheme();
}
void Sheet::setSide(Side side) {
    side_ = side;
    arrangePanel();
}
QRect Sheet::panelGeometry() const {
    const int w = std::min(contentWidth_, std::max(1, width() * 3 / 4));
    const int h =
        std::min(std::max(200, panel_->sizeHint().height()), std::max(1, height() * 4 / 5));
    switch (side_) {
    case Side::Left:
        return {0, 0, w, height()};
    case Side::Right:
        return {width() - w, 0, w, height()};
    case Side::Top:
        return {0, 0, width(), h};
    case Side::Bottom:
        return {0, height() - h, width(), h};
    }
    return {};
}
QPoint Sheet::animationOffset() const {
    switch (side_) {
    case Side::Top:
        return {0, -40};
    case Side::Right:
        return {40, 0};
    case Side::Bottom:
        return {0, 40};
    case Side::Left:
        return {-40, 0};
    }
    return {};
}
Drawer::Drawer(QWidget* parent) : Sheet(Side::Bottom, parent), handle_(new QFrame(panel_)) {
    duration_ = 450;
    handle_->setFixedSize(100, 4);
    handle_->setObjectName("shadcnDrawerHandle");
    handle_->setStyleSheet(
        "QFrame#shadcnDrawerHandle { background: palette(mid); border-radius: 3px; }");
    handle_->setCursor(Qt::SizeVerCursor);
    handle_->installEventFilter(this);
    panelLayout_->insertWidget(0, handle_, 0, Qt::AlignHCenter);
    setCloseButtonVisible(false);
    refreshTheme();
}
bool Drawer::eventFilter(QObject* object, QEvent* event) {
    if (object == handle_) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton) {
                dragging_ = true;
                dragStart_ = mouse->globalPosition().toPoint();
                dragGeometry_ = panel_->geometry();
                dragTime_.start();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove && dragging_) {
            const int delta =
                std::max(0, static_cast<QMouseEvent*>(event)->globalPosition().toPoint().y() -
                                dragStart_.y());
            panel_->move(dragGeometry_.topLeft() + QPoint(0, delta));
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease && dragging_) {
            dragging_ = false;
            const int delta = panel_->y() - dragGeometry_.y();
            if (delta > dragGeometry_.height() / 3 || (delta > 30 && dragTime_.elapsed() < 200))
                reject();
            else
                arrangePanel();
            return true;
        }
    }
    return Sheet::eventFilter(object, event);
}

Popover::Popover(QWidget* parent)
    : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint), content_(new QVBoxLayout(this)),
      animation_(new QVariantAnimation(this)) {
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);
    content_->setContentsMargins(10, 10, 10, 10);
    content_->setSpacing(10);
    connect(animation_, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& value) { setWindowOpacity(value.toDouble()); });
}
void Popover::setContentWidth(int width) {
    contentWidth_ = std::max(1, width);
    if (isVisible())
        positionPopup();
}
void Popover::showFor(QWidget& anchor, Side side, int offset) {
    if (anchor_)
        anchor_->removeEventFilter(this);
    if (owner_)
        owner_->removeEventFilter(this);
    anchor_ = &anchor;
    owner_ = anchor.window();
    side_ = side;
    offset_ = std::max(0, offset);
    anchor.installEventFilter(this);
    if (owner_ != anchor_)
        owner_->installEventFilter(this);
    previousFocus_ = QApplication::focusWidget();
    positionPopup();
    show();
    raise();
    if (takesFocus()) {
        activateWindow();
        setFocus(Qt::PopupFocusReason);
        focusNextChild();
    }
}
void Popover::positionPopup() {
    if (!anchor_)
        return;
    const auto available = anchor_->screen()->availableGeometry().adjusted(4, 4, -4, -4);
    setFixedWidth(std::min(contentWidth_, available.width()));
    adjustSize();
    resize(width(), std::min(sizeHint().height(), available.height()));
    const QRect anchor(anchor_->mapToGlobal(QPoint{}), anchor_->size());
    auto location = [&](Side side) {
        switch (side) {
        case Side::Top:
            return QPoint(anchor.center().x() - width() / 2, anchor.top() - height() - offset_);
        case Side::Bottom:
            return QPoint(anchor.center().x() - width() / 2, anchor.bottom() + 1 + offset_);
        case Side::Left:
            return QPoint(anchor.left() - width() - offset_, anchor.center().y() - height() / 2);
        case Side::Right:
            return QPoint(anchor.right() + 1 + offset_, anchor.center().y() - height() / 2);
        }
        return QPoint{};
    };
    actualSide_ = side_;
    auto point = location(side_);
    if (!available.contains(QRect(point, size()))) {
        const auto opposite = side_ == Side::Top      ? Side::Bottom
                              : side_ == Side::Bottom ? Side::Top
                              : side_ == Side::Left   ? Side::Right
                                                      : Side::Left;
        const auto flipped = location(opposite);
        if (available.contains(QRect(flipped, size()))) {
            point = flipped;
            actualSide_ = opposite;
        }
    }
    point.setX(std::clamp(point.x(), available.left(),
                          std::max(available.left(), available.right() - width() + 1)));
    point.setY(std::clamp(point.y(), available.top(),
                          std::max(available.top(), available.bottom() - height() + 1)));
    move(point);
}
void Popover::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(colour(*this, Role::Border), 1));
    p.setBrush(colour(*this, backgroundRole()));
    p.drawRoundedRect(QRectF(rect()).adjusted(.5, .5, -.5, -.5), 8, 8);
}
void Popover::showEvent(QShowEvent* event) {
    auto p = palette();
    p.setColor(QPalette::WindowText, colour(*this, foregroundRole()));
    setPalette(p);
    QFrame::showEvent(event);
    animation_->stop();
    animation_->setDuration(reduced(*this) ? 0 : 100);
    animation_->setStartValue(0.0);
    animation_->setEndValue(1.0);
    animation_->start();
}
void Popover::hideEvent(QHideEvent* event) {
    animation_->stop();
    setWindowOpacity(1);
    QFrame::hideEvent(event);
    if (takesFocus() && previousFocus_ && previousFocus_->isVisible() &&
        previousFocus_->isEnabled())
        previousFocus_->setFocus(Qt::PopupFocusReason);
}
void Popover::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        hide();
        event->accept();
    } else
        QFrame::keyPressEvent(event);
}
bool Popover::eventFilter(QObject* object, QEvent* event) {
    if (object == anchor_ || object == owner_) {
        if (event->type() == QEvent::Hide || event->type() == QEvent::Close ||
            event->type() == QEvent::Destroy)
            hide();
        if (isVisible() && (event->type() == QEvent::Move || event->type() == QEvent::Resize))
            positionPopup();
    }
    return QFrame::eventFilter(object, event);
}

Tooltip::Tooltip(const QString& text, QWidget* parent) : Popover(parent), label_(new QLabel(this)) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    content().setContentsMargins(12, 6, 12, 6);
    label_->setWordWrap(true);
    label_->setTextFormat(Qt::PlainText);
    auto f = label_->font();
    f.setPixelSize(12);
    label_->setFont(f);
    content().addWidget(label_);
    delay_.setSingleShot(true);
    delay_.setInterval(0);
    connect(&delay_, &QTimer::timeout, this, [this] {
        if (target_ && target_->isVisible() && target_->isEnabled())
            showFor(*target_, side_, 4);
    });
    setText(text);
}
void Tooltip::setText(const QString& text) {
    label_->setText(text);
    setAccessibleName(text);
    setContentWidth(
        std::min(320, std::max(32, label_->fontMetrics().horizontalAdvance(text) + 24)));
    if (target_)
        target_->setAccessibleDescription(text);
}
void Tooltip::attach(QWidget& target, Side side) {
    delay_.stop();
    hide();
    if (target_)
        target_->removeEventFilter(this);
    target_ = &target;
    side_ = side;
    target.installEventFilter(this);
    target.setAccessibleDescription(label_->text());
}
void Tooltip::setDelay(int milliseconds) {
    delay_.setInterval(std::max(0, milliseconds));
}
bool Tooltip::eventFilter(QObject* object, QEvent* event) {
    if (object == target_) {
        if (event->type() == QEvent::Enter || event->type() == QEvent::FocusIn)
            delay_.start();
        else if (event->type() == QEvent::Leave || event->type() == QEvent::FocusOut ||
                 event->type() == QEvent::Hide || event->type() == QEvent::Close ||
                 event->type() == QEvent::Destroy ||
                 event->type() == QEvent::MouseButtonPress) {
            delay_.stop();
            hide();
        } else if (event->type() == QEvent::KeyPress &&
                   static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
            delay_.stop();
            hide();
        }
    }
    return Popover::eventFilter(object, event);
}
HoverCard::HoverCard(QWidget* parent) : Popover(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setContentWidth(256);
    open_.setSingleShot(true);
    close_.setSingleShot(true);
    open_.setInterval(700);
    close_.setInterval(300);
    connect(&open_, &QTimer::timeout, this, [this] {
        if (target_ && target_->isVisible() && target_->isEnabled())
            showFor(*target_);
    });
    connect(&close_, &QTimer::timeout, this, &QWidget::hide);
}
void HoverCard::attach(QWidget& target) {
    open_.stop();
    close_.stop();
    hide();
    if (target_)
        target_->removeEventFilter(this);
    target_ = &target;
    target.installEventFilter(this);
}
void HoverCard::setOpenDelay(int milliseconds) {
    open_.setInterval(std::max(0, milliseconds));
}
void HoverCard::setCloseDelay(int milliseconds) {
    close_.setInterval(std::max(0, milliseconds));
}
bool HoverCard::eventFilter(QObject* object, QEvent* event) {
    if (object == target_) {
        if (event->type() == QEvent::Hide || event->type() == QEvent::Close ||
            event->type() == QEvent::Destroy) {
            open_.stop();
            close_.stop();
            hide();
        } else if (event->type() == QEvent::Enter || event->type() == QEvent::FocusIn) {
            close_.stop();
            open_.start();
        } else if (event->type() == QEvent::Leave || event->type() == QEvent::FocusOut) {
            open_.stop();
            close_.start();
        }
    }
    return Popover::eventFilter(object, event);
}
bool HoverCard::event(QEvent* event) {
    if (event->type() == QEvent::Enter)
        close_.stop();
    else if (event->type() == QEvent::Leave)
        close_.start();
    return Popover::event(event);
}
} // namespace shadcn
