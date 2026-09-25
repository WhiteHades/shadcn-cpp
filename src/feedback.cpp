// SPDX-License-Identifier: MIT
// Design source: shadcn-ui/ui base wrappers and Nova style rules.
#include <shadcn/feedback.hpp>

#include <QAccessible>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QRadioButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace shadcn {
namespace {

const Style* installedStyle(const QWidget& widget) {
    if (const auto* style = qobject_cast<const Style*>(widget.style())) return style;
    if (qApp) return qobject_cast<const Style*>(qApp->style());
    return nullptr;
}

const Theme& themeFor(const QWidget& widget) {
    if (const auto* style = installedStyle(widget)) return style->theme();
    static const auto fallback = Theme::neutral();
    return fallback;
}

QColor colour(const QWidget& widget, Role role) {
    const auto value = themeFor(widget).color(role);
    return QColor::fromRgbF(static_cast<float>(value.r), static_cast<float>(value.g),
                            static_cast<float>(value.b), static_cast<float>(value.a));
}

QColor alpha(QColor value, double amount) {
    value.setAlphaF(static_cast<float>(std::clamp(amount, 0.0, 1.0)));
    return value;
}

double radius(const QWidget& widget) { return std::max(0.0, themeFor(widget).radius() - 2.0); }

void rounded(QPainter& painter, QRectF rect, double cornerRadius, const QColor& fill,
             const QColor& border = QColor(Qt::transparent), double width = 1) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(fill);
    painter.setPen(border.alpha() == 0 ? Qt::NoPen : QPen(border, width));
    cornerRadius = std::min(cornerRadius, std::min(rect.width(), rect.height()) / 2.0);
    painter.drawRoundedRect(rect, cornerRadius, cornerRadius);
}

QColor foreground(const QWidget& widget) { return colour(widget, Role::Foreground); }
QColor muted(const QWidget& widget) { return colour(widget, Role::MutedForeground); }

QColor bubbleFill(const QWidget& widget, BubbleVariant variant) {
    switch (variant) {
    case BubbleVariant::Default: return colour(widget, Role::Primary);
    case BubbleVariant::Secondary: return colour(widget, Role::Secondary);
    case BubbleVariant::Muted: return colour(widget, Role::Muted);
    case BubbleVariant::Tinted:
        return themeFor(widget).mode() == ColorMode::Dark
            ? alpha(colour(widget, Role::Primary), .3) : alpha(colour(widget, Role::Primary), .12);
    case BubbleVariant::Outline: return colour(widget, Role::Background);
    case BubbleVariant::Ghost: return Qt::transparent;
    case BubbleVariant::Destructive: return alpha(colour(widget, Role::Destructive), .12);
    }
    return colour(widget, Role::Primary);
}

QColor bubbleText(const QWidget& widget, BubbleVariant variant) {
    switch (variant) {
    case BubbleVariant::Default: return colour(widget, Role::PrimaryForeground);
    case BubbleVariant::Secondary: return colour(widget, Role::SecondaryForeground);
    case BubbleVariant::Muted: case BubbleVariant::Tinted: case BubbleVariant::Outline:
    case BubbleVariant::Ghost: return foreground(widget);
    case BubbleVariant::Destructive: return colour(widget, Role::Destructive);
    }
    return foreground(widget);
}

void announce(QObject* object, const QString& message) {
    if (!object || message.isEmpty()) return;
    QAccessibleAnnouncementEvent event(object, message);
    event.setPoliteness(QAccessible::AnnouncementPoliteness::Polite);
    QAccessible::updateAccessibility(&event);
}

} // namespace

Toast::Toast(int id, const QString& title, const QString& description, QWidget* parent)
    : QFrame(parent), id_(id), remaining_(duration_), timer_(new QTimer(this)),
      icon_(new QLabel(this)), title_(new QLabel(title, this)),
      description_(new QLabel(description, this)), action_(new Button(QString{}, this)),
      close_(new Button(QStringLiteral("×"), this)) {
    setObjectName(QStringLiteral("toast"));
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(title);
    setAccessibleDescription(description);
    setFrameStyle(QFrame::NoFrame);

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(12, 10, 8, 10);
    row->setSpacing(8);
    icon_->setFixedWidth(18);
    icon_->setAlignment(Qt::AlignCenter);
    auto* body = new QWidget(this);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(2);
    title_->setVisible(!title.isEmpty());
    description_->setVisible(!description.isEmpty());
    title_->setWordWrap(true);
    description_->setWordWrap(true);
    title_->setTextFormat(Qt::PlainText);
    description_->setTextFormat(Qt::PlainText);
    bodyLayout->addWidget(title_);
    bodyLayout->addWidget(description_);
    row->addWidget(icon_);
    row->addWidget(body, 1);

    action_->setVisible(false);
    action_->setButtonSize(ButtonSize::Sm);
    action_->setVariant(Variant::Outline);
    row->addWidget(action_);
    close_->setAccessibleName(QStringLiteral("Dismiss notification"));
    close_->setButtonSize(ButtonSize::Xs);
    close_->setVariant(Variant::Ghost);
    close_->setFocusPolicy(Qt::StrongFocus);
    row->addWidget(close_);

    timer_->setSingleShot(true);
    connect(timer_, &QTimer::timeout, this, &Toast::expire);
    connect(action_, &QPushButton::clicked, this, [this] {
        emit actionTriggered(id_);
        dismiss();
    });
    connect(close_, &QPushButton::clicked, this, &Toast::dismiss);
    action_->installEventFilter(this);
    close_->installEventFilter(this);
    updateIcon();
}

void Toast::setTitle(const QString& title) {
    title_->setText(title);
    title_->setVisible(!title.isEmpty());
    setAccessibleName(title);
    updateGeometry();
}

void Toast::setDescription(const QString& description) {
    description_->setText(description);
    description_->setVisible(!description.isEmpty());
    setAccessibleDescription(description);
    updateGeometry();
}

void Toast::setType(ToastType type) {
    if (type_ == type) return;
    type_ = type;
    updateIcon();
    update();
}

void Toast::setDuration(int milliseconds) {
    duration_ = std::max(0, milliseconds);
    remaining_ = duration_;
    if (!paused_) resume();
}

void Toast::setActionText(const QString& text) {
    action_->setText(text);
    action_->setVisible(!text.isEmpty());
    updateGeometry();
}

QString Toast::actionText() const { return action_->text(); }

void Toast::pause() {
    if (paused_ || duration_ <= 0 || dismissed_) return;
    if (elapsed_.isValid()) remaining_ = std::max(0, remaining_ - static_cast<int>(elapsed_.elapsed()));
    timer_->stop();
    paused_ = true;
}

void Toast::resume() {
    if (!paused_ && timer_->isActive()) return;
    if (duration_ <= 0 || dismissed_ || remaining_ <= 0) return;
    paused_ = false;
    elapsed_.restart();
    timer_->start(remaining_);
}

void Toast::dismiss() {
    if (dismissed_) return;
    dismissed_ = true;
    timer_->stop();
    emit dismissed(id_);
}

void Toast::enterEvent(QEnterEvent* event) {
    QFrame::enterEvent(event);
    pause();
}

void Toast::leaveEvent(QEvent* event) {
    QFrame::leaveEvent(event);
    resume();
}

void Toast::focusInEvent(QFocusEvent* event) {
    QFrame::focusInEvent(event);
    pause();
}

void Toast::focusOutEvent(QFocusEvent* event) {
    QFrame::focusOutEvent(event);
    resume();
}

bool Toast::event(QEvent* event) {
    if (event->type() == QEvent::Enter || event->type() == QEvent::FocusIn) pause();
    if (event->type() == QEvent::Leave || event->type() == QEvent::FocusOut) resume();
    return QFrame::event(event);
}

bool Toast::eventFilter(QObject*, QEvent* event) {
    if (event->type() == QEvent::FocusIn || event->type() == QEvent::Enter) pause();
    if (event->type() == QEvent::FocusOut || event->type() == QEvent::Leave) resume();
    return false;
}

void Toast::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        dismiss();
        event->accept();
        return;
    }
    QFrame::keyPressEvent(event);
}

void Toast::expire() { dismiss(); }

void Toast::updateIcon() {
    QString glyph;
    switch (type_) {
    case ToastType::Default: glyph = {}; break;
    case ToastType::Success: glyph = QStringLiteral("✓"); break;
    case ToastType::Info: glyph = QStringLiteral("ⓘ"); break;
    case ToastType::Warning: glyph = QStringLiteral("!"); break;
    case ToastType::Error: glyph = QStringLiteral("×"); break;
    case ToastType::Loading: glyph = QStringLiteral("…"); break;
    }
    icon_->setText(glyph);
    icon_->setVisible(!glyph.isEmpty());
    auto palette = icon_->palette();
    palette.setColor(QPalette::WindowText,
                     type_ == ToastType::Error ? colour(*this, Role::Destructive) : foreground(*this));
    icon_->setPalette(palette);
}

void Toast::announce(const QString& text) const { shadcn::announce(const_cast<Toast*>(this), text); }

void Toast::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const auto border = type_ == ToastType::Error ? colour(*this, Role::Destructive)
                                                  : colour(*this, Role::Border);
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius(*this),
            colour(*this, Role::Popover), border);
}

Sonner::Sonner(QWidget* parent) : QFrame(parent), layout_(new QVBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
    setFixedWidth(360);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(8);
    layout_->setAlignment(Qt::AlignBottom);
    setAccessibleName(QStringLiteral("Notifications"));
}

int Sonner::showToast(const QString& title, const QString& description, ToastType type,
                      int duration, const QString& actionText) {
    return addToast(title, description, type, duration, actionText).id();
}

Toast& Sonner::addToast(const QString& title, const QString& description, ToastType type,
                        int duration, const QString& actionText) {
    auto* toast = new Toast(nextId_++, title, description, this);
    toast->setType(type);
    toast->setDuration(duration);
    toast->setActionText(actionText);
    layout_->insertWidget(0, toast);
    toasts_.append(QPointer<Toast>(toast));
    connect(toast, &Toast::dismissed, this, [this](int id) { removeToast(id); });
    toast->show();
    toast->resume();
    if (!title.isEmpty()) announce(toast, description.isEmpty() ? title : title + QStringLiteral(". ") + description);
    while (toasts_.size() > maxVisible_) {
        if (toasts_.first()) toasts_.first()->dismiss();
        else toasts_.removeFirst();
    }
    show();
    if (attachedWindow_) raise();
    positionHost();
    emit toastAdded(toast->id());
    return *toast;
}

void Sonner::dismiss(int id) {
    for (const auto& toast : toasts_)
        if (toast && toast->id() == id) { toast->dismiss(); return; }
}

void Sonner::dismissAll() {
    const auto current = toasts();
    for (auto* toast : current) if (toast) toast->dismiss();
}

int Sonner::toastCount() const {
    int count = 0;
    for (const auto& toast : toasts_) if (toast) ++count;
    return count;
}

QList<Toast*> Sonner::toasts() const {
    QList<Toast*> result;
    for (const auto& toast : toasts_) if (toast) result.append(toast.data());
    return result;
}

void Sonner::setMaxVisible(int count) {
    maxVisible_ = std::max(1, count);
    while (toasts_.size() > maxVisible_ && toasts_.first()) toasts_.first()->dismiss();
}

void Sonner::setPosition(ToastPosition position) {
    position_ = position;
    positionHost();
}

void Sonner::attachTo(QWidget& window, ToastPosition position) {
    if (attachedWindow_) attachedWindow_->removeEventFilter(this);
    attachedWindow_ = &window;
    position_ = position;
    setParent(&window);
    window.installEventFilter(this);
    show();
    raise();
    positionHost();
}

bool Sonner::eventFilter(QObject* watched, QEvent* event) {
    if (watched == attachedWindow_ && (event->type() == QEvent::Resize ||
                                       event->type() == QEvent::Move || event->type() == QEvent::Show))
        QTimer::singleShot(0, this, &Sonner::positionHost);
    return QFrame::eventFilter(watched, event);
}

void Sonner::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    if (attachedWindow_) positionHost();
}

void Sonner::showEvent(QShowEvent* event) {
    QFrame::showEvent(event);
    positionHost();
}

void Sonner::removeToast(int id) {
    for (int index = 0; index < toasts_.size(); ++index) {
        auto* toast = toasts_[index].data();
        if (!toast || toast->id() != id) continue;
        layout_->removeWidget(toast);
        toasts_.removeAt(index);
        toast->deleteLater();
        emit toastRemoved(id);
        break;
    }
    if (toasts_.isEmpty() && !attachedWindow_) hide();
    positionHost();
}

void Sonner::positionHost() {
    if (!attachedWindow_) return;
    adjustSize();
    const auto margin = 16;
    const auto parentSize = attachedWindow_->size();
    const auto xRight = std::max(margin, parentSize.width() - width() - margin);
    const auto xCenter = std::max(margin, (parentSize.width() - width()) / 2);
    const auto x = position_ == ToastPosition::Top || position_ == ToastPosition::Bottom
        ? xCenter
        : position_ == ToastPosition::TopLeft || position_ == ToastPosition::BottomLeft
            ? margin : xRight;
    const auto yBottom = std::max(margin, parentSize.height() - height() - margin);
    const auto y = position_ == ToastPosition::Top || position_ == ToastPosition::TopLeft ||
                           position_ == ToastPosition::TopRight ? margin : yBottom;
    move(x, y);
}

Attachment::Attachment(QWidget* parent) : QFrame(parent), media_(new QLabel(this)),
    title_(new QLabel(this)), description_(new QLabel(this)), contentHost_(new QWidget(this)),
    outer_(new QHBoxLayout(this)), content_(new QVBoxLayout(contentHost_)),
    actions_(new QHBoxLayout) {
    setFrameStyle(QFrame::NoFrame);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    media_->setAlignment(Qt::AlignCenter);
    media_->setFixedSize(40, 40);
    media_->setText(QStringLiteral("FILE"));
    content_->setContentsMargins(0, 0, 0, 0);
    content_->setSpacing(2);
    title_->setVisible(false);
    description_->setVisible(false);
    title_->setWordWrap(false);
    description_->setWordWrap(false);
    title_->setTextFormat(Qt::PlainText);
    description_->setTextFormat(Qt::PlainText);
    content_->addWidget(title_);
    content_->addWidget(description_);
    actions_->setContentsMargins(0, 0, 0, 0);
    actions_->setSpacing(2);
    outer_->setContentsMargins(10, 8, 10, 8);
    outer_->setSpacing(8);
    outer_->addWidget(media_);
    outer_->addWidget(contentHost_, 1);
    outer_->addLayout(actions_);
    updateLayout();
}

void Attachment::setTitle(const QString& title) {
    title_->setText(title);
    title_->setVisible(!title.isEmpty());
    setAccessibleName(title);
    updateGeometry();
}

void Attachment::setDescription(const QString& description) {
    description_->setText(description);
    description_->setVisible(!description.isEmpty());
    setAccessibleDescription(description);
    updateGeometry();
}

void Attachment::setSource(const QUrl& source) {
    source_ = source;
    if (title_->text().isEmpty()) setTitle(source.fileName());
    media_->setToolTip(source.toDisplayString());
}

void Attachment::setFilePath(const QString& path) { setSource(QUrl::fromLocalFile(path)); }

void Attachment::setPreview(const QPixmap& preview) {
    preview_ = preview;
    if (preview_.isNull()) media_->setText(QStringLiteral("FILE"));
    else media_->setPixmap(preview_.scaled(media_->size(), Qt::KeepAspectRatioByExpanding,
                                           Qt::SmoothTransformation));
    update();
}

void Attachment::setState(AttachmentState state) {
    if (state_ == state) return;
    state_ = state;
    update();
}

void Attachment::setSize(AttachmentSize size) {
    if (size_ == size) return;
    size_ = size;
    updateLayout();
    updateGeometry();
    update();
}

void Attachment::setOrientation(AttachmentOrientation orientation) {
    if (orientation_ == orientation) return;
    orientation_ = orientation;
    updateLayout();
    updateGeometry();
    update();
}

QString Attachment::title() const { return title_->text(); }
QString Attachment::description() const { return description_->text(); }

Button& Attachment::addAction(const QString& text) {
    auto* action = new AttachmentAction(text, this);
    actions_->addWidget(action);
    connect(action, &QPushButton::clicked, this, [this, action] { emit actionTriggered(action->text()); });
    return *action;
}

QSize Attachment::sizeHint() const {
    const auto base = outer_->sizeHint();
    return {std::max(160, base.width()), base.height()};
}

void Attachment::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && source_.isValid()) {
        setFocus(Qt::MouseFocusReason);
        emit openRequested(source_);
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void Attachment::keyPressEvent(QKeyEvent* event) {
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Space) && source_.isValid()) {
        emit openRequested(source_);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete) {
        emit dismissed();
        event->accept();
        return;
    }
    QFrame::keyPressEvent(event);
}

QColor Attachment::stateColour() const {
    return state_ == AttachmentState::Error ? colour(*this, Role::Destructive)
                                            : colour(*this, Role::Border);
}

void Attachment::updateLayout() {
    const auto mediaSize = size_ == AttachmentSize::Xs ? 28 : size_ == AttachmentSize::Sm ? 32 : 40;
    media_->setFixedSize(mediaSize, mediaSize);
    if (orientation_ == AttachmentOrientation::Horizontal) {
        outer_->setDirection(QBoxLayout::LeftToRight);
        setMinimumWidth(160);
        content_->setContentsMargins(0, 0, 0, 0);
    } else {
        outer_->setDirection(QBoxLayout::TopToBottom);
        setMinimumWidth(120);
        content_->setContentsMargins(4, 0, 4, 0);
    }
}

void Attachment::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const auto dashed = state_ == AttachmentState::Idle;
    const auto border = stateColour();
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius(*this),
            colour(*this, Role::Card), border);
    if (dashed) {
        painter.setPen(QPen(border, 1, Qt::DashLine));
        painter.drawRoundedRect(QRectF(rect()).adjusted(.5, .5, -.5, -.5), radius(*this), radius(*this));
    }
    if (hasFocus()) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(alpha(colour(*this, Role::Ring), .5), 2));
        painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), radius(*this), radius(*this));
    }
}

AttachmentAction::AttachmentAction(const QString& text, QWidget* parent) : Button(text, parent) {
    setVariant(Variant::Ghost);
    setButtonSize(ButtonSize::Xs);
}

AttachmentGroup::AttachmentGroup(QWidget* parent) : QFrame(parent), layout_(new QHBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(4, 4, 4, 4);
    layout_->setSpacing(12);
}

void AttachmentGroup::addAttachment(Attachment& attachment) { layout_->addWidget(&attachment); }

void AttachmentGroup::removeAttachment(Attachment& attachment) {
    layout_->removeWidget(&attachment);
    attachment.setParent(nullptr);
    attachment.hide();
}

int AttachmentGroup::count() const { return layout_->count(); }

Bubble::Bubble(const QString& text, QWidget* parent) : QFrame(parent), text_(new QLabel(text, this)),
    content_(new QVBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setAccessibleName(text);
    content_->setContentsMargins(12, 8, 12, 8);
    content_->setSpacing(4);
    text_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    text_->setWordWrap(true);
    text_->setTextFormat(Qt::PlainText);
    content_->addWidget(text_);
}

void Bubble::setText(const QString& text) {
    text_->setText(text);
    setAccessibleName(text);
    updateGeometry();
    update();
}

QString Bubble::text() const { return text_->text(); }

void Bubble::setVariant(BubbleVariant variant) {
    if (variant_ == variant) return;
    variant_ = variant;
    update();
}

void Bubble::setAlign(BubbleAlign align) {
    if (align_ == align) return;
    align_ = align;
    updateGeometry();
    update();
}

void Bubble::addWidget(QWidget& widget) { content_->addWidget(&widget); }
QVBoxLayout& Bubble::content() { return *content_; }
QSize Bubble::sizeHint() const { return content_->sizeHint(); }
bool Bubble::hasHeightForWidth() const { return content_->hasHeightForWidth(); }
int Bubble::heightForWidth(int width) const {
    if (width <= 0) return sizeHint().height();
    return content_->totalHeightForWidth(width);
}

void Bubble::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const auto fill = bubbleFill(*this, variant_);
    const auto border = variant_ == BubbleVariant::Outline ? colour(*this, Role::Border)
        : variant_ == BubbleVariant::Ghost ? QColor(Qt::transparent) : QColor(Qt::transparent);
    rounded(painter, QRectF(rect()).adjusted(.5, .5, -.5, -.5),
            variant_ == BubbleVariant::Ghost ? 0 : 12, fill, border);
    auto palette = text_->palette();
    palette.setColor(QPalette::WindowText, bubbleText(*this, variant_));
    text_->setPalette(palette);
}

BubbleGroup::BubbleGroup(QWidget* parent) : QFrame(parent), layout_(new QVBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(8);
}

void BubbleGroup::addBubble(Bubble& bubble) {
    layout_->addWidget(&bubble, 0,
                       bubble.align() == BubbleAlign::End ? Qt::AlignRight : Qt::AlignLeft);
}
int BubbleGroup::count() const { return layout_->count(); }

BubbleReactions::BubbleReactions(QWidget* parent) : QFrame(parent), layout_(new QHBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(6, 2, 6, 2);
    layout_->setSpacing(4);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
}

void BubbleReactions::addWidget(QWidget& widget) { layout_->addWidget(&widget); }
void BubbleReactions::setSide(Qt::Edge side) {
    side_ = side;
    setProperty("shadcnBubbleReactionSide", side_ == Qt::TopEdge ? QStringLiteral("top")
                                                                   : QStringLiteral("bottom"));
    update();
}

void BubbleReactions::setAlign(BubbleAlign align) {
    align_ = align;
    layout_->setAlignment(align_ == BubbleAlign::End ? Qt::AlignRight : Qt::AlignLeft);
    setProperty("shadcnBubbleReactionAlign", align_ == BubbleAlign::End ? QStringLiteral("end")
                                                                           : QStringLiteral("start"));
    update();
}

MessageAvatar::MessageAvatar(QWidget* parent) : QFrame(parent) {
    setFrameStyle(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    setLayout(new QHBoxLayout);
    layout()->setContentsMargins(0, 0, 0, 0);
}

void MessageAvatar::setAvatar(Avatar& avatar) { layout()->addWidget(&avatar); }

MessageHeader::MessageHeader(QWidget* parent) : QFrame(parent) {
    setFrameStyle(QFrame::NoFrame);
    auto* box = new QHBoxLayout(this);
    box->setContentsMargins(12, 0, 12, 0);
    box->setSpacing(4);
}

void MessageHeader::addWidget(QWidget& widget) { static_cast<QBoxLayout*>(layout())->addWidget(&widget); }

MessageFooter::MessageFooter(QWidget* parent) : QFrame(parent) {
    setFrameStyle(QFrame::NoFrame);
    auto* box = new QHBoxLayout(this);
    box->setContentsMargins(12, 0, 12, 0);
    box->setSpacing(4);
}

void MessageFooter::addWidget(QWidget& widget) { static_cast<QBoxLayout*>(layout())->addWidget(&widget); }

Message::Message(QWidget* parent) : QFrame(parent), row_(new QHBoxLayout(this)),
    avatarHost_(new QWidget(this)), bodyHost_(new QWidget(this)), body_(new QVBoxLayout(bodyHost_)),
    content_(new QVBoxLayout), text_(new QLabel(bodyHost_)), headerHost_(new QWidget(bodyHost_)),
    footerHost_(new QWidget(bodyHost_)) {
    setFrameStyle(QFrame::NoFrame);
    row_->setContentsMargins(0, 0, 0, 0);
    row_->setSpacing(8);
    avatarHost_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    auto* avatarLayout = new QHBoxLayout(avatarHost_);
    avatarLayout->setContentsMargins(0, 0, 0, 0);
    body_->setContentsMargins(0, 0, 0, 0);
    body_->setSpacing(10);
    text_->setWordWrap(true);
    text_->setTextFormat(Qt::PlainText);
    content_->setContentsMargins(0, 0, 0, 0);
    content_->setSpacing(4);
    content_->addWidget(text_);
    headerHost_->setVisible(false);
    footerHost_->setVisible(false);
    body_->addWidget(headerHost_);
    body_->addLayout(content_);
    body_->addWidget(footerHost_);
    row_->addWidget(avatarHost_);
    row_->addWidget(bodyHost_, 1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

void Message::setAlign(MessageAlign align) {
    if (align_ == align) return;
    align_ = align;
    row_->setDirection(align_ == MessageAlign::Start ? QBoxLayout::LeftToRight
                                                      : QBoxLayout::RightToLeft);
    update();
}

void Message::setText(const QString& text) {
    text_->setText(text);
    setAccessibleName(text);
    updateGeometry();
}

QString Message::text() const { return text_->text(); }

void Message::setAvatar(Avatar& avatar) { avatarHost_->layout()->addWidget(&avatar); }
void Message::addContent(QWidget& widget) { content_->addWidget(&widget); }

void Message::addHeader(QWidget& widget) {
    auto* layout = headerHost_->layout();
    if (!layout) layout = new QHBoxLayout(headerHost_);
    layout->setContentsMargins(0, 0, 0, 0);
    static_cast<QBoxLayout*>(layout)->addWidget(&widget);
    headerHost_->show();
}

void Message::addFooter(QWidget& widget) {
    auto* layout = footerHost_->layout();
    if (!layout) layout = new QHBoxLayout(footerHost_);
    layout->setContentsMargins(0, 0, 0, 0);
    static_cast<QBoxLayout*>(layout)->addWidget(&widget);
    footerHost_->show();
}

QVBoxLayout& Message::content() { return *content_; }
void Message::paintEvent(QPaintEvent*) {}

MessageGroup::MessageGroup(QWidget* parent) : QFrame(parent), layout_(new QVBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(8);
}

void MessageGroup::addMessage(Message& message) { layout_->addWidget(&message); }
int MessageGroup::count() const { return layout_->count(); }

MessageScrollerItem::MessageScrollerItem(QWidget* parent) : QFrame(parent), layout_(new QVBoxLayout(this)) {
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(0, 0, 0, 0);
}

void MessageScrollerItem::addWidget(QWidget& widget) { layout_->addWidget(&widget); }

MessageScrollerButton::MessageScrollerButton(Direction direction, QWidget* parent)
    : Button(direction == Direction::End ? QStringLiteral("↓") : QStringLiteral("↑"), parent),
      direction_(direction) {
    setVariant(Variant::Secondary);
    setButtonSize(ButtonSize::Sm);
    setAccessibleName(direction == Direction::End ? QStringLiteral("Scroll to end")
                                                 : QStringLiteral("Scroll to start"));
}

MessageScroller::MessageScroller(QWidget* parent) : QFrame(parent), area_(new QScrollArea(this)),
    contentHost_(new QWidget), content_(new QVBoxLayout(contentHost_)),
    startButton_(new MessageScrollerButton(MessageScrollerButton::Direction::Start, this)),
    endButton_(new MessageScrollerButton(MessageScrollerButton::Direction::End, this)) {
    setFrameStyle(QFrame::NoFrame);
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(4);
    area_->setWidgetResizable(true);
    area_->setFrameShape(QFrame::NoFrame);
    area_->setWidget(contentHost_);
    content_->setContentsMargins(0, 0, 0, 0);
    content_->setSpacing(24);
    content_->addStretch(1);
    outer->addWidget(startButton_, 0, Qt::AlignHCenter);
    outer->addWidget(area_, 1);
    outer->addWidget(endButton_, 0, Qt::AlignHCenter);
    startButton_->setEnabled(false);
    endButton_->setEnabled(false);
    connect(startButton_, &QPushButton::clicked, this, &MessageScroller::scrollToStart);
    connect(endButton_, &QPushButton::clicked, this, &MessageScroller::scrollToEnd);
    connect(area_->verticalScrollBar(), &QScrollBar::valueChanged, this, [this] { updateButtons(); });
    area_->viewport()->installEventFilter(this);
    setAccessibleName(QStringLiteral("Message scroller"));
}

void MessageScroller::addWidget(QWidget& widget) {
    content_->insertWidget(std::max(0, content_->count() - 1), &widget);
    if (autoScroll_) QTimer::singleShot(0, this, &MessageScroller::scrollToEnd);
    updateButtons();
}

void MessageScroller::addItem(MessageScrollerItem& item) { addWidget(item); }

void MessageScroller::scrollToStart() {
    area_->verticalScrollBar()->setValue(area_->verticalScrollBar()->minimum());
    updateButtons();
}

void MessageScroller::scrollToEnd() {
    area_->verticalScrollBar()->setValue(area_->verticalScrollBar()->maximum());
    updateButtons();
}

bool MessageScroller::isAtStart() const {
    return area_->verticalScrollBar()->value() <= area_->verticalScrollBar()->minimum();
}

bool MessageScroller::isAtEnd() const {
    const auto* bar = area_->verticalScrollBar();
    return bar->value() >= bar->maximum();
}

void MessageScroller::setAutoScroll(bool enabled) { autoScroll_ = enabled; }
QVBoxLayout& MessageScroller::content() { return *content_; }

bool MessageScroller::eventFilter(QObject* watched, QEvent* event) {
    if (watched == area_->viewport() && (event->type() == QEvent::Resize || event->type() == QEvent::Show))
        QTimer::singleShot(0, this, &MessageScroller::updateButtons);
    return QFrame::eventFilter(watched, event);
}

void MessageScroller::updateButtons() {
    const auto start = !isAtStart();
    const auto end = !isAtEnd();
    startButton_->setEnabled(start);
    endButton_->setEnabled(end);
    if (start != startVisible_) { startVisible_ = start; emit startVisibilityChanged(start); }
    if (end != endVisible_) { endVisible_ = end; emit endVisibilityChanged(end); }
}

Questionnaire::Questionnaire(QWidget* parent) : QFrame(parent), layout_(new QVBoxLayout(this)),
    progress_(new QLabel(this)) {
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(16);
    progress_->setAlignment(Qt::AlignRight);
    progress_->setPalette([this] {
        auto palette = this->palette();
        palette.setColor(QPalette::WindowText, muted(*this));
        return palette;
    }());
    layout_->addWidget(progress_);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

int Questionnaire::addQuestion(const QString& title, const QString& description, QuestionnaireType type) {
    questions_.append({title, description, type, {}, {}});
    if (current_ < 0) current_ = 0;
    render();
    return static_cast<int>(questions_.size() - 1);
}

void Questionnaire::addChoice(int question, const QString& id, const QString& label,
                              const QString& description, const QString& shortcut) {
    if (question < 0 || question >= questions_.size() || id.isEmpty()) return;
    questions_[question].choices.append({id, label, description, shortcut});
    if (question == current_) render();
}

int Questionnaire::questionCount() const { return static_cast<int>(questions_.size()); }

void Questionnaire::setCurrentQuestion(int index) {
    if (index < 0 || index >= questions_.size() || index == current_) return;
    current_ = index;
    render();
    emit questionChanged(current_);
}

QStringList Questionnaire::answer(int question) const {
    if (question < 0 || question >= questions_.size()) return {};
    return questions_[question].answers;
}

void Questionnaire::setAnswer(int question, const QStringList& ids) {
    if (question < 0 || question >= questions_.size()) return;
    QStringList accepted;
    for (const auto& choice : questions_[question].choices)
        if (ids.contains(choice.id)) accepted.append(choice.id);
    if (questions_[question].type == QuestionnaireType::Single && accepted.size() > 1)
        accepted = {accepted.front()};
    if (questions_[question].answers == accepted) return;
    questions_[question].answers = accepted;
    if (question == current_) render();
    emit answerChanged(question, accepted);
}

QHash<int, QStringList> Questionnaire::answers() const {
    QHash<int, QStringList> result;
    for (int index = 0; index < questions_.size(); ++index) result.insert(index, questions_[index].answers);
    return result;
}

void Questionnaire::next() {
    if (current_ < 0) return;
    if (current_ + 1 >= questions_.size()) { submit(); return; }
    ++current_;
    render();
    emit questionChanged(current_);
}

void Questionnaire::previous() {
    if (current_ <= 0) return;
    --current_;
    render();
    emit questionChanged(current_);
}

void Questionnaire::skip() {
    if (current_ < 0) return;
    const auto skippedQuestion = current_;
    emit skipped(skippedQuestion);
    next();
}

void Questionnaire::submit() { emit submitted(); }

void Questionnaire::updateChoice(int question, const QString& id, bool checked) {
    if (question < 0 || question >= questions_.size()) return;
    auto& selected = questions_[question].answers;
    if (questions_[question].type == QuestionnaireType::Single) selected.clear();
    if (checked) {
        if (!selected.contains(id)) selected.append(id);
    } else {
        selected.removeAll(id);
    }
    emit answerChanged(question, selected);
}

void Questionnaire::render() {
    while (layout_->count() > 1) {
        auto* item = layout_->takeAt(1);
        if (auto* widget = item->widget()) { widget->hide(); widget->deleteLater(); }
        delete item;
    }
    if (choiceGroup_) {
        choiceGroup_->deleteLater();
        choiceGroup_.clear();
    }
    if (current_ < 0 || current_ >= questions_.size()) {
        progress_->clear();
        return;
    }
    auto& question = questions_[current_];
    progress_->setText(QStringLiteral("%1 of %2").arg(current_ + 1).arg(questions_.size()));
    auto* title = new QLabel(question.title, this);
    title->setTextFormat(Qt::PlainText);
    title->setWordWrap(true);
    title->setStyleSheet(QStringLiteral("font-weight:500;"));
    layout_->addWidget(title);
    if (!question.description.isEmpty()) {
        auto* description = new QLabel(question.description, this);
        description->setTextFormat(Qt::PlainText);
        description->setWordWrap(true);
        auto palette = description->palette();
        palette.setColor(QPalette::WindowText, muted(*this));
        description->setPalette(palette);
        layout_->addWidget(description);
    }
    auto* choices = new QVBoxLayout;
    choices->setContentsMargins(0, 0, 0, 0);
    choices->setSpacing(8);
    choiceGroup_ = new QButtonGroup(this);
    choiceGroup_->setExclusive(question.type == QuestionnaireType::Single);
    for (const auto& choice : question.choices) {
        QAbstractButton* button = question.type == QuestionnaireType::Single
            ? static_cast<QAbstractButton*>(new QRadioButton(choice.label, this))
            : static_cast<QAbstractButton*>(new QCheckBox(choice.label, this));
        button->setMinimumHeight(36);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setAccessibleDescription(choice.description);
        button->setToolTip(choice.shortcut);
        button->setChecked(question.answers.contains(choice.id));
        choiceGroup_->addButton(button);
        choices->addWidget(button);
        const auto questionIndex = current_;
        const auto choiceId = choice.id;
        connect(button, &QAbstractButton::toggled, this,
                [this, questionIndex, choiceId](bool checked) {
                    if (questionIndex == current_) updateChoice(questionIndex, choiceId, checked);
                });
    }
    auto* choicesHost = new QWidget(this);
    choicesHost->setLayout(choices);
    layout_->addWidget(choicesHost);
    auto* actions = new QHBoxLayout;
    actions->setContentsMargins(0, 0, 0, 0);
    auto* previous = new Button(QStringLiteral("Previous"), this);
    previous->setVariant(Variant::Outline);
    previous->setEnabled(current_ > 0);
    connect(previous, &QPushButton::clicked, this, &Questionnaire::previous);
    actions->addWidget(previous);
    actions->addStretch();
    auto* skipButton = new Button(QStringLiteral("Skip"), this);
    skipButton->setVariant(Variant::Outline);
    connect(skipButton, &QPushButton::clicked, this, &Questionnaire::skip);
    actions->addWidget(skipButton);
    auto* nextButton = new Button(current_ + 1 == questions_.size() ? QStringLiteral("Submit")
                                                                      : QStringLiteral("Next"), this);
    nextButton->setVariant(Variant::Default);
    connect(nextButton, &QPushButton::clicked, this, &Questionnaire::next);
    actions->addWidget(nextButton);
    auto* actionsHost = new QWidget(this);
    actionsHost->setLayout(actions);
    layout_->addWidget(actionsHost);
}

Marker::Marker(const QString& text, QWidget* parent) : QFrame(parent), icon_(new QLabel(this)),
    text_(new QLabel(text, this)), before_(new QFrame(this)), after_(new QFrame(this)),
    layout_(new QHBoxLayout(this)) {
    text_->setTextFormat(Qt::PlainText);
    setFrameStyle(QFrame::NoFrame);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(8);
    before_->setFrameShape(QFrame::HLine);
    after_->setFrameShape(QFrame::HLine);
    before_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    after_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    icon_->setFixedSize(16, 16);
    icon_->setVisible(false);
    layout_->addWidget(before_);
    layout_->addWidget(icon_);
    layout_->addWidget(text_);
    layout_->addWidget(after_);
    setAccessibleName(text);
    setVariant(MarkerVariant::Default);
}

void Marker::setText(const QString& text) {
    text_->setText(text);
    setAccessibleName(text);
    updateGeometry();
}

QString Marker::text() const { return text_->text(); }

void Marker::setVariant(MarkerVariant variant) {
    if (variant_ == variant && before_->isVisible() == (variant == MarkerVariant::Separator)) return;
    variant_ = variant;
    const auto separator = variant == MarkerVariant::Separator;
    before_->setVisible(separator);
    after_->setVisible(separator);
    update();
}

void Marker::setIcon(const QPixmap& icon) {
    icon_->setPixmap(icon);
    icon_->setVisible(!icon.isNull());
    updateGeometry();
}

QSize Marker::sizeHint() const { return layout_->sizeHint(); }

void Marker::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);
    if (variant_ == MarkerVariant::Border) {
        QPainter painter(this);
        painter.setPen(QPen(colour(*this, Role::Border), 1));
        painter.drawLine(rect().bottomLeft(), rect().bottomRight());
    }
}

} // namespace shadcn
