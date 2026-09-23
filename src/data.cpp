// SPDX-License-Identifier: MIT
// Adapted from shadcn/ui stock components and Nova styles.
#include <QAccessible>
#include <QApplication>
#include <QCompleter>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QToolTip>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariantAnimation>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <shadcn/data.hpp>
#include <stdexcept>

namespace shadcn {
namespace {
const Theme& theme(const QWidget& widget) {
    if (const auto* s = qobject_cast<const Style*>(widget.style()))
        return s->theme();
    if (const auto* s = qobject_cast<const Style*>(QApplication::style()))
        return s->theme();
    static const auto fallback = Theme::neutral();
    return fallback;
}
QColor colour(const QWidget& widget, Role role) {
    const auto c = theme(widget).color(role);
    return QColor::fromRgbF(float(c.r), float(c.g), float(c.b), float(c.a));
}
QString css(const QWidget& widget, Role role) {
    const auto c = colour(widget, role);
    return QString("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}
void rounded(QPainter& p, QRectF rect, const QColor& fill, const QColor& border,
             double radius = 10) {
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(fill);
    p.setPen(QPen(border, 1));
    p.drawRoundedRect(rect, radius, radius);
}
bool reduced(const QWidget& widget) {
    const auto* s = qobject_cast<const Style*>(widget.style());
    if (!s)
        s = qobject_cast<const Style*>(QApplication::style());
    return s && s->motion() == MotionPolicy::Reduced;
}

} // namespace

NativeSelect::NativeSelect(QWidget* parent) : QComboBox(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(32);
    setSizeAdjustPolicy(QComboBox::AdjustToContents);
    setMaxVisibleItems(12);
    refreshTheme();
}
void NativeSelect::setInvalid(bool invalid) {
    invalid_ = invalid;
    setProperty("shadcnInvalid", invalid);
    update();
}
QSize NativeSelect::sizeHint() const {
    auto size = QComboBox::sizeHint();
    return {std::max(100, size.width() + 12), std::max(32, fontMetrics().height() + 10)};
}
void NativeSelect::refreshTheme() {
    auto p = palette();
    p.setColor(QPalette::Base, colour(*this, Role::Popover));
    p.setColor(QPalette::Text, colour(*this, Role::Foreground));
    p.setColor(QPalette::Highlight, colour(*this, Role::Accent));
    p.setColor(QPalette::HighlightedText, colour(*this, Role::AccentForeground));
    setPalette(p);
    view()->setPalette(p);
}
void NativeSelect::changeEvent(QEvent* event) {
    QComboBox::changeEvent(event);
    if (event->type() == QEvent::StyleChange)
        refreshTheme();
}
void NativeSelect::paintEvent(QPaintEvent*) {
    QPainter p(this);
    if (!isEnabled())
        p.setOpacity(.5);
    const auto border = colour(*this, invalid_     ? Role::Destructive
                                      : hasFocus() ? Role::Ring
                                                   : Role::Input);
    auto fill = colour(*this, Role::Background);
    if (theme(*this).mode() == ColorMode::Dark) {
        fill = colour(*this, Role::Input);
        fill.setAlphaF(fill.alphaF() * .3f);
    }
    rounded(p, QRectF(rect()).adjusted(.5, .5, -.5, -.5), fill, border);
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    const int arrow = rtl ? 15 : width() - 15;
    p.setPen(QPen(colour(*this, Role::MutedForeground), 1.5, Qt::SolidLine, Qt::RoundCap,
                  Qt::RoundJoin));
    QPainterPath path;
    path.moveTo(arrow - 4, height() / 2 - 2);
    path.lineTo(arrow, height() / 2 + 2);
    path.lineTo(arrow + 4, height() / 2 - 2);
    p.drawPath(path);
    if (!isEditable()) {
        p.setFont(font());
        p.setPen(colour(*this, currentIndex() < 0 ? Role::MutedForeground : Role::Foreground));
        const QString text = currentIndex() < 0 ? placeholderText() : currentText();
        const auto bounds = rect().adjusted(rtl ? 30 : 10, 0, rtl ? -10 : -30, 0);
        p.drawText(bounds, static_cast<int>(Qt::AlignVCenter | (rtl ? Qt::AlignRight : Qt::AlignLeft)),
                   fontMetrics().elidedText(text, Qt::ElideRight, bounds.width()));
    }
}
Select::Select(QWidget* parent) : NativeSelect(parent) {}
void Select::addGroup(const QString& label) {
    const auto previous = currentIndex();
    addItem(label);
    const int row = count() - 1;
    if (auto* model = qobject_cast<QStandardItemModel*>(this->model())) {
        auto* item = model->item(row);
        item->setFlags(Qt::NoItemFlags);
        auto f = font();
        f.setWeight(QFont::Medium);
        item->setFont(f);
        item->setForeground(colour(*this, Role::MutedForeground));
    }
    if (previous < 0)
        setCurrentIndex(-1);
}
void Select::setItemEnabled(int index, bool enabled) {
    if (auto* items = qobject_cast<QStandardItemModel*>(model());
        items && index >= 0 && index < items->rowCount()) {
        auto* item = items->item(index);
        item->setEnabled(enabled);
        item->setSelectable(enabled);
    }
}
void Select::showPopup() {
    view()->setStyleSheet(
        QString("QAbstractItemView { background: %1; color: %2; border: 1px solid %3; "
                "border-radius: 8px; padding: 4px; outline: 0; } QAbstractItemView::item { "
                "min-height: 24px; padding: 2px 8px; border-radius: 6px; } "
                "QAbstractItemView::item:selected { background: %4; color: %2; }")
            .arg(css(*this, Role::Popover), css(*this, Role::Foreground), css(*this, Role::Border),
                 css(*this, Role::Accent)));
    NativeSelect::showPopup();
}
Combobox::Combobox(QWidget* parent) : Select(parent) {
    setEditable(true);
    setInsertPolicy(QComboBox::NoInsert);
    lineEdit()->setClearButtonEnabled(true);
    lineEdit()->setTextMargins(6, 0, 0, 0);
    lineEdit()->setStyleSheet("QLineEdit { background: transparent; border: none; }");
    completer()->setCompletionMode(QCompleter::PopupCompletion);
    completer()->setCaseSensitivity(Qt::CaseInsensitive);
    completer()->setFilterMode(Qt::MatchContains);
}
void Combobox::setPlaceholderText(const QString& text) {
    QComboBox::setPlaceholderText(text);
    lineEdit()->setPlaceholderText(text);
}

Command::Command(QWidget* parent)
    : QWidget(parent), search_(new Input(this)), list_(new QListWidget(this)),
      empty_(new QLabel(tr("No results found."), this)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    search_->setPlaceholderText(tr("Type a command or search..."));
    search_->setAccessibleName(tr("Search commands"));
    search_->installEventFilter(this);
    list_->setFrameShape(QFrame::NoFrame);
    list_->setAccessibleName(tr("Commands"));
    list_->setMouseTracking(true);
    list_->setMinimumHeight(120);
    empty_->setAlignment(Qt::AlignCenter);
    empty_->setMinimumHeight(80);
    empty_->hide();
    layout->addWidget(search_);
    layout->addWidget(list_);
    layout->addWidget(empty_);
    setFocusProxy(search_);
    connect(search_, &Input::textChanged, this, &Command::filter);
    list_->installEventFilter(this);
    const auto activate = [this](QListWidgetItem* item) {
        if (item && (item->flags() & Qt::ItemIsEnabled))
            emit triggered(item->data(Qt::UserRole));
    };
    connect(list_, &QListWidget::itemClicked, this, activate);
    refreshTheme();
}
QListWidgetItem& Command::addItem(const QString& label, const QVariant& itemData,
                                  const QStringList& keywords) {
    auto* item = new QListWidgetItem(label, list_);
    item->setData(Qt::UserRole, itemData.isValid() ? itemData : QVariant(label));
    item->setData(Qt::UserRole + 1, keywords);
    filter(search_->text());
    return *item;
}
void Command::addGroup(const QString& label) {
    auto* item = new QListWidgetItem(label, list_);
    item->setFlags(Qt::NoItemFlags);
    item->setData(Qt::UserRole + 2, true);
    auto f = font();
    f.setPixelSize(12);
    item->setFont(f);
    item->setForeground(colour(*this, Role::MutedForeground));
}
void Command::setEmptyText(const QString& text) {
    empty_->setText(text);
}
void Command::filter(const QString& text) {
    QListWidgetItem* first = nullptr;
    QListWidgetItem* group = nullptr;
    bool groupVisible = false;
    for (int row = 0; row < list_->count(); ++row) {
        auto* item = list_->item(row);
        if (item->data(Qt::UserRole + 2).toBool()) {
            if (group)
                group->setHidden(!groupVisible);
            group = item;
            groupVisible = false;
            continue;
        }
        const auto haystack =
            item->text() + " " + item->data(Qt::UserRole + 1).toStringList().join(' ');
        const bool visible = haystack.contains(text, Qt::CaseInsensitive);
        item->setHidden(!visible);
        groupVisible |= visible;
        if (visible && (item->flags() & Qt::ItemIsEnabled) && !first)
            first = item;
    }
    if (group)
        group->setHidden(!groupVisible);
    list_->setCurrentItem(first);
    empty_->setVisible(!first);
    list_->setVisible(first != nullptr);
}
bool Command::eventFilter(QObject* object, QEvent* event) {
    if ((object == search_ || object == list_) && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (object == search_ && (key->key() == Qt::Key_Down || key->key() == Qt::Key_Up)) {
            QApplication::sendEvent(list_, event);
            return true;
        }
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            auto* item = list_->currentItem();
            if (item && !item->isHidden() && (item->flags() & Qt::ItemIsEnabled))
                emit triggered(item->data(Qt::UserRole));
            return true;
        }
    }
    return QWidget::eventFilter(object, event);
}
void Command::refreshTheme() {
    list_->setStyleSheet(QString("QListWidget { background: %1; color: %2; border: none; outline: "
                                 "0; } QListWidget::item { padding: 6px 8px; border-radius: 6px; } "
                                 "QListWidget::item:selected { background: %3; color: %2; }")
                             .arg(css(*this, Role::Popover), css(*this, Role::Foreground),
                                  css(*this, Role::Accent)));
}
void Command::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::StyleChange)
        refreshTheme();
}

Calendar::Calendar(QWidget* parent) : QCalendarWidget(parent) {
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
    setGridVisible(false);
    setFirstDayOfWeek(locale().firstDayOfWeek());
    setMinimumSize(252, 252);
    connect(this, &QCalendarWidget::clicked, this, &Calendar::choose);
    refreshTheme();
}
void Calendar::setRangeSelection(bool enabled) {
    rangeSelection_ = enabled;
    selectingEnd_ = false;
    updateCells();
}
void Calendar::setSelectedRange(QDate first, QDate last) {
    if (!first.isValid() || !last.isValid() || first > last || first < minimumDate() ||
        last > maximumDate())
        return;
    rangeStart_ = first;
    rangeEnd_ = last;
    setSelectedDate(first);
    selectingEnd_ = false;
    updateCells();
    emit rangeChanged(first, last);
}
void Calendar::choose(QDate date) {
    if (!rangeSelection_)
        return;
    if (!selectingEnd_) {
        rangeStart_ = date;
        rangeEnd_ = QDate{};
        selectingEnd_ = true;
    } else {
        rangeEnd_ = date;
        if (rangeStart_ > rangeEnd_)
            std::swap(rangeStart_, rangeEnd_);
        selectingEnd_ = false;
    }
    updateCells();
    emit rangeChanged(rangeStart_, rangeEnd_);
}
void Calendar::refreshTheme() {
    for (auto* child : findChildren<QWidget*>()) {
        if (child->objectName() == "qt_calendar_navigationbar")
            child->setStyleSheet(
                QString("QWidget#qt_calendar_navigationbar { background: %1; } QToolButton { "
                        "background: transparent; color: %2; border: none; border-radius: 6px; "
                        "padding: 4px; } QToolButton:hover { background: %3; }")
                    .arg(css(*this, Role::Background), css(*this, Role::Foreground),
                         css(*this, Role::Muted)));
    }
    QTextCharFormat heading;
    heading.setForeground(colour(*this, Role::MutedForeground));
    heading.setBackground(Qt::transparent);
    heading.setFontWeight(QFont::Normal);
    setHeaderTextFormat(heading);
    for (const auto day : {Qt::Saturday, Qt::Sunday})
        setWeekdayTextFormat(day, heading);
    for (const auto* name : {"qt_calendar_prevmonth", "qt_calendar_nextmonth"}) {
        if (auto* button = findChild<QToolButton*>(name)) {
            QPixmap image(32, 32);
            image.setDevicePixelRatio(2);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(QPen(colour(*this, Role::Foreground), 1.5, Qt::SolidLine,
                                Qt::RoundCap, Qt::RoundJoin));
            const bool previous = button->objectName().endsWith("prevmonth");
            QPolygonF points;
            points << QPointF(previous ? 10 : 6, 4) << QPointF(previous ? 6 : 10, 8)
                   << QPointF(previous ? 10 : 6, 12);
            painter.drawPolyline(points);
            painter.end();
            button->setArrowType(Qt::NoArrow);
            button->setIcon(QIcon(image));
            button->setIconSize(QSize(16, 16));
        }
    }
    updateCells();
}
void Calendar::changeEvent(QEvent* event) {
    QCalendarWidget::changeEvent(event);
    if (event->type() == QEvent::StyleChange)
        refreshTheme();
}
void Calendar::paintCell(QPainter* p, const QRect& rect, QDate date) const {
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setFont(font());
    p->setPen(Qt::NoPen);
    p->fillRect(rect, colour(*this, Role::Background));
    const bool inRange = rangeSelection_ && rangeStart_.isValid() && rangeEnd_.isValid() &&
                         date >= rangeStart_ && date <= rangeEnd_;
    const bool endpoint = selectionMode() != QCalendarWidget::NoSelection &&
                          (rangeSelection_ ? (date == rangeStart_ || date == rangeEnd_)
                                           : date == selectedDate());
    const bool disabled = date < minimumDate() || date > maximumDate();
    if (inRange) {
        p->setBrush(colour(*this, Role::Muted));
        p->drawRect(rect);
    }
    if (endpoint || date == QDate::currentDate()) {
        p->setBrush(colour(*this, endpoint ? Role::Primary : Role::Muted));
        p->drawRoundedRect(rect.adjusted(1, 1, -1, -1), 6, 6);
    }
    p->setPen(colour(*this, endpoint                       ? Role::PrimaryForeground
                            : date.month() != monthShown() ? Role::MutedForeground
                                                           : Role::Foreground));
    if (disabled)
        p->setOpacity(.5);
    p->drawText(rect, Qt::AlignCenter, QString::number(date.day()));
    p->restore();
}
DatePicker::DatePicker(QWidget* parent)
    : Button(tr("Pick a date"), parent), calendar_(nullptr), popup_(new Popover(this)) {
    setVariant(Variant::Outline);
    popup_->setContentWidth(280);
    calendar_ = new Calendar(popup_);
    popup_->content().addWidget(calendar_);
    connect(this, &Button::clicked, this, [this] { popup_->showFor(*this); });
    connect(calendar_, &QCalendarWidget::clicked, this, [this](QDate date) {
        setDate(date);
        popup_->hide();
    });
    connect(calendar_, &QCalendarWidget::activated, this, [this](QDate date) {
        setDate(date);
        popup_->hide();
    });
}
void DatePicker::setDate(QDate date) {
    if (date.isValid() && (date < calendar_->minimumDate() || date > calendar_->maximumDate()))
        return;
    const auto selectionMode = date.isValid() ? QCalendarWidget::SingleSelection
                                              : QCalendarWidget::NoSelection;
    if (date_ == date && calendar_->selectionMode() == selectionMode)
        return;
    const bool changed = date_ != date;
    date_ = date;
    calendar_->setSelectionMode(selectionMode);
    if (date.isValid())
        calendar_->setSelectedDate(date);
    setText(date.isValid() ? locale().toString(date, QLocale::LongFormat) : tr("Pick a date"));
    if (changed)
        emit dateChanged(date);
}

Field::Field(const QString& text, QWidget* parent)
    : QWidget(parent), layout_(new QVBoxLayout(this)), label_(new Label(text, this)),
      description_(new QLabel(this)), error_(new QLabel(this)) {
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(6);
    layout_->addWidget(label_);
    layout_->addWidget(description_);
    layout_->addWidget(error_);
    description_->setWordWrap(true);
    error_->setWordWrap(true);
    description_->setTextFormat(Qt::PlainText);
    error_->setTextFormat(Qt::PlainText);
    description_->hide();
    error_->hide();
    refreshTheme();
}
void Field::setControl(QWidget* control) {
    if (control_ == control)
        return;
    if (control_) {
        auto* old = control_.data();
        layout_->removeWidget(old);
        old->hide();
        old->setParent(nullptr);
        label_->setBuddy(nullptr);
        control_.clear();
    }
    if (control) {
        control->setParent(this);
        layout_->insertWidget(1, control);
        control_ = control;
        label_->setBuddy(control);
        if (control->accessibleName().isEmpty())
            control->setAccessibleName(label_->text());
    }
}
void Field::setLabel(const QString& text) {
    label_->setText(text);
    if (control_)
        control_->setAccessibleName(text);
}
void Field::setDescription(const QString& text) {
    description_->setText(text);
    description_->setVisible(!text.isEmpty());
    if (control_ && error_->text().isEmpty())
        control_->setAccessibleDescription(text);
}
void Field::setError(const QString& text) {
    error_->setText(text);
    error_->setVisible(!text.isEmpty());
    if (control_) {
        control_->setProperty("invalid", !text.isEmpty());
        control_->setAccessibleDescription(text.isEmpty() ? description_->text() : text);
        QAccessibleEvent event(control_, QAccessible::DescriptionChanged);
        QAccessible::updateAccessibility(&event);
    }
}
QString Field::error() const {
    return error_->text();
}
void Field::setValidator(std::function<QString()> validator) {
    validator_ = std::move(validator);
}
bool Field::validate() {
    setError(validator_ && control_ && control_->isEnabled() ? validator_() : QString{});
    return error_->text().isEmpty();
}
void Field::refreshTheme() {
    auto p = description_->palette();
    p.setColor(QPalette::WindowText, colour(*this, Role::MutedForeground));
    description_->setPalette(p);
    p.setColor(QPalette::WindowText, colour(*this, Role::Destructive));
    error_->setPalette(p);
}
void Field::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::StyleChange)
        refreshTheme();
}
Form::Form(QWidget* parent) : QWidget(parent), layout_(new QVBoxLayout(this)) {
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(20);
}
Field& Form::addField(const QString& label, QWidget* control) {
    auto* field = new Field(label, this);
    field->setControl(control);
    fields_.append(field);
    layout_->addWidget(field);
    return *field;
}
bool Form::validate() {
    bool valid = true;
    QWidget* first = nullptr;
    for (const auto& field : fields_)
        if (field && !field->validate()) {
            valid = false;
            if (!first)
                first = field->control();
        }
    if (first)
        first->setFocus();
    return valid;
}
void Form::submit() {
    if (validate())
        emit submitted();
}

InputOTP::InputOTP(int length, QWidget* parent) : QLineEdit(parent), length_(length) {
    if (length < 1 || length > 32)
        throw std::invalid_argument("InputOTP length must be between 1 and 32");
    setMaxLength(length);
    setFrame(false);
    setAlphanumeric(false);
    setAccessibleName(tr("Verification code"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    blink_.setInterval(std::max(100, QApplication::cursorFlashTime() / 2));
    connect(&blink_, &QTimer::timeout, this, [this] {
        caret_ = !caret_;
        if (hasFocus())
            update();
    });
    connect(this, &QLineEdit::textChanged, this, [this](const QString& text) {
        emit codeChanged(text);
        if (text.size() == length_)
            emit completed(text);
        update();
    });
    connect(this, &QLineEdit::cursorPositionChanged, this, [this] {
        caret_ = true;
        update();
    });
}
bool InputOTP::allowed(QChar c) const {
    return (c >= '0' && c <= '9') ||
           (alphanumeric_ && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')));
}
QString InputOTP::code() const {
    return text();
}
bool InputOTP::setCode(const QString& code) {
    if (code.size() > length_ ||
        !std::all_of(code.begin(), code.end(), [this](QChar c) { return allowed(c); }))
        return false;
    setText(code);
    return true;
}
void InputOTP::setAlphanumeric(bool enabled) {
    alphanumeric_ = enabled;
    const auto pattern = enabled ? QString("[A-Za-z0-9]{0,%1}") : QString("[0-9]{0,%1}");
    auto* old = validator();
    setValidator(new QRegularExpressionValidator(QRegularExpression(pattern.arg(length_)), this));
    if (old && old->parent() == this)
        delete old;
    const auto value = text();
    if (!std::all_of(value.begin(), value.end(), [this](QChar c) { return allowed(c); }))
        clear();
}
bool InputOTP::event(QEvent* event) {
    const bool result = QLineEdit::event(event);
    if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut ||
        event->type() == QEvent::Show || event->type() == QEvent::Hide ||
        event->type() == QEvent::StyleChange || event->type() == QEvent::EnabledChange) {
        if (isVisible() && hasFocus() && isEnabled() && !reduced(*this))
            blink_.start();
        else {
            blink_.stop();
            caret_ = true;
        }
        update();
    }
    return result;
}
QSize InputOTP::sizeHint() const {
    return {length_ * 32 + 2, 34};
}
void InputOTP::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(font());
    if (!isEnabled())
        p.setOpacity(.5);
    const int cursor = std::min(cursorPosition(), length_ - 1);
    const auto text = this->text();
    for (int index = 0; index < length_; ++index) {
        const int visual = layoutDirection() == Qt::RightToLeft ? length_ - index - 1 : index;
        QRectF cell(visual * 32 + .5, .5, 32, height() - 1);
        rounded(p, cell, colour(*this, Role::Background),
                colour(*this, hasFocus() && index == cursor ? Role::Ring : Role::Input),
                index == 0 || index == length_ - 1 ? 8 : 0);
        if (selectionStart() >= 0 && index >= selectionStart() &&
            index < selectionStart() + selectedText().size()) {
            p.fillRect(cell.adjusted(2, 2, -2, -2), colour(*this, Role::Accent));
        }
        p.setPen(colour(*this, Role::Foreground));
        if (index < text.size())
            p.drawText(cell, Qt::AlignCenter, text.mid(index, 1));
        else if (hasFocus() && index == cursor && caret_)
            p.drawLine(QPointF(cell.center().x(), 9), QPointF(cell.center().x(), height() - 9));
    }
}
void InputOTP::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        QLineEdit::mousePressEvent(event);
        return;
    }
    setFocus(Qt::MouseFocusReason);
    int index = static_cast<int>(event->position().x()) / 32;
    if (layoutDirection() == Qt::RightToLeft)
        index = length_ - 1 - index;
    selectionAnchor_ = std::clamp(index, 0, static_cast<int>(text().size()));
    setCursorPosition(selectionAnchor_);
    event->accept();
}
void InputOTP::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        int index = static_cast<int>(event->position().x()) / 32;
        if (layoutDirection() == Qt::RightToLeft)
            index = length_ - 1 - index;
        index = std::clamp(index, 0, static_cast<int>(text().size()));
        setSelection(selectionAnchor_, index - selectionAnchor_);
        event->accept();
    } else
        QLineEdit::mouseMoveEvent(event);
}

Table::Table(QWidget* parent) : QTableView(parent) {
    setShowGrid(false);
    setFrameShape(QFrame::NoFrame);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setAlternatingRowColors(false);
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize(36);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    refreshTheme();
}
void Table::refreshTheme() {
    auto p = palette();
    p.setColor(QPalette::Base, colour(*this, Role::Background));
    p.setColor(QPalette::Text, colour(*this, Role::Foreground));
    p.setColor(QPalette::Highlight, colour(*this, Role::Muted));
    p.setColor(QPalette::HighlightedText, colour(*this, Role::Foreground));
    setPalette(p);
    horizontalHeader()->setStyleSheet(
        QString("QHeaderView::section { background: %1; color: %2; border: none; border-bottom: "
                "1px solid %3; padding: 8px; font-weight: 500; }")
            .arg(css(*this, Role::Background), css(*this, Role::Foreground),
                 css(*this, Role::Border)));
}
void Table::changeEvent(QEvent* event) {
    QTableView::changeEvent(event);
    if (event->type() == QEvent::StyleChange)
        refreshTheme();
}
DataTable::DataTable(QWidget* parent) : Table(parent), proxy_(new QSortFilterProxyModel(this)) {
    proxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_->setSortCaseSensitivity(Qt::CaseInsensitive);
    QTableView::setModel(proxy_);
    setSortingEnabled(true);
}
void DataTable::setSourceModel(QAbstractItemModel* model) {
    proxy_->setSourceModel(model);
}
void DataTable::setFilter(const QString& text, int column) {
    proxy_->setFilterKeyColumn(column);
    proxy_->setFilterFixedString(text);
}

Chart::Chart(QWidget* parent) : QWidget(parent), animation_(new QVariantAnimation(this)) {
    setMouseTracking(true);
    setMinimumSize(160, 120);
    setAccessibleName(tr("Chart"));
    animation_->setStartValue(0.0);
    animation_->setEndValue(1.0);
    animation_->setDuration(500);
    animation_->setEasingCurve(QEasingCurve::OutCubic);
    connect(animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        amount_ = value.toDouble();
        update();
    });
}
void Chart::setChartType(ChartType type) {
    type_ = type;
    animate();
}
std::expected<void, ValueError> Chart::setSeries(QList<ChartSeries> series) {
    for (const auto& row : series)
        for (double value : row.values)
            if (!std::isfinite(value))
                return std::unexpected(ValueError::NonFinite);
    series_ = std::move(series);
    hover_ = -1;
    QStringList descriptions;
    for (const auto& row : series_) {
        QStringList values;
        for (double value : row.values)
            values << QString::number(value);
        descriptions << row.name + ": " + values.join(", ");
    }
    setAccessibleDescription(descriptions.join(". "));
    animate();
    return {};
}
void Chart::setLabels(QStringList labels) {
    labels_ = std::move(labels);
    update();
}
void Chart::setLegendVisible(bool visible) {
    legend_ = visible;
    update();
}
QSize Chart::sizeHint() const {
    return {480, 280};
}
QRectF Chart::plotRect() const {
    return QRectF(rect()).adjusted(44, 12, -12, legend_ ? -54 : -30);
}
void Chart::animate() {
    animation_->stop();
    if (isVisible() && !reduced(*this)) {
        animation_->start();
    } else {
        amount_ = 1;
        update();
    }
}
bool Chart::event(QEvent* event) {
    const bool result = QWidget::event(event);
    if (event->type() == QEvent::Hide) {
        animation_->stop();
        amount_ = 1;
    } else if (event->type() == QEvent::StyleChange) {
        if (reduced(*this)) {
            animation_->stop();
            amount_ = 1;
        }
        update();
    }
    return result;
}
void Chart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    auto font = this->font();
    font.setPixelSize(12);
    p.setFont(font);
    const auto area = plotRect();
    if (area.width() <= 0 || area.height() <= 0)
        return;
    if (series_.isEmpty()) {
        p.setPen(colour(*this, Role::MutedForeground));
        p.drawText(rect(), Qt::AlignCenter, tr("No data"));
        return;
    }
    qsizetype count = 0;
    double low = 0, high = 0;
    for (const auto& row : series_) {
        count = std::max(count, row.values.size());
        for (double v : row.values) {
            low = std::min(low, v);
            high = std::max(high, v);
        }
    }
    if (count == 0) {
        p.setPen(colour(*this, Role::MutedForeground));
        p.drawText(rect(), Qt::AlignCenter, tr("No data"));
        return;
    }
    const double scale = std::max({std::abs(low), std::abs(high), 1.0});
    low /= scale;
    high /= scale;
    if (high == low) high = low + 1;
    const auto y = [&](double value) {
        return area.bottom() -
               ((value / scale * amount_ - low) / (high - low)) * area.height();
    };
    const double step = area.width() / static_cast<double>(std::max<qsizetype>(1, count));
    if (type_ == ChartType::Pie) {
        double sum = 0;
        for (double v : series_.first().values)
            sum += std::max(0.0, v / scale);
        if (sum <= 0)
            return;
        const int diameter = static_cast<int>(std::min(area.width(), area.height()));
        const QRect pie(static_cast<int>(area.center().x()) - diameter / 2,
                        static_cast<int>(area.center().y()) - diameter / 2, diameter, diameter);
        double angle = 90;
        for (qsizetype i = 0; i < series_.first().values.size(); ++i) {
            const double span =
                (std::max(0.0, series_.first().values[i] / scale) / sum) * 360 * amount_;
            p.setBrush(colour(*this, static_cast<Role>(static_cast<int>(Role::Chart1) + i % 5)));
            p.setPen(QPen(colour(*this, Role::Background), 2));
            p.drawPie(pie, qRound(angle * 16), qRound(-span * 16));
            angle -= span;
        }
    } else {
        for (int i = 0; i <= 4; ++i) {
            const double yy = area.top() + area.height() * i / 4;
            p.setPen(QPen(colour(*this, Role::Border), 1, Qt::DashLine));
            p.drawLine(QPointF(area.left(), yy), QPointF(area.right(), yy));
            p.setPen(colour(*this, Role::MutedForeground));
            const double value = std::clamp(high - (high - low) * i / 4, low, high) * scale;
            p.drawText(QRectF(0, yy - 9, area.left() - 8, 18), Qt::AlignRight | Qt::AlignVCenter,
                       QString::number(value, 'g', 3));
        }
        for (qsizetype s = 0; s < series_.size(); ++s) {
            const auto& row = series_[s];
            const auto ink = colour(*this, row.colour);
            QPainterPath path;
            for (qsizetype i = 0; i < row.values.size(); ++i) {
                const double x = area.left() + step * (static_cast<double>(i) + .5);
                const double yy = y(row.values[i]);
                if (type_ == ChartType::Bar) {
                    const double barWidth = step * .75 / static_cast<double>(series_.size());
                    const double xx = x - step * .375 + barWidth * static_cast<double>(s);
                    p.setPen(Qt::NoPen);
                    p.setBrush(ink);
                    p.drawRoundedRect(
                        QRectF(xx, std::min(yy, y(0)), barWidth - 1, std::abs(y(0) - yy)), 3, 3);
                } else {
                    if (i == 0)
                        path.moveTo(x, yy);
                    else
                        path.lineTo(x, yy);
                }
            }
            if (type_ != ChartType::Bar && !row.values.isEmpty()) {
                if (type_ == ChartType::Area) {
                    auto filled = path;
                    filled.lineTo(
                        area.left() + step * (static_cast<double>(row.values.size()) - .5), y(0));
                    filled.lineTo(area.left() + step * .5, y(0));
                    filled.closeSubpath();
                    auto fill = ink;
                    fill.setAlphaF(.2f);
                    p.fillPath(filled, fill);
                }
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(ink, 2));
                p.drawPath(path);
            }
        }
        p.setPen(colour(*this, Role::MutedForeground));
        for (qsizetype i = 0; i < labels_.size() && i < count; ++i) {
            if (count > 12 && i % (count / 12 + 1) != 0)
                continue;
            p.drawText(
                QRectF(area.left() + step * static_cast<double>(i), area.bottom() + 8, step, 18),
                Qt::AlignCenter,
                fontMetrics().elidedText(labels_[i], Qt::ElideRight,
                                         std::max(1, static_cast<int>(step) - 4)));
        }
    }
    if (legend_) {
        int x = 44;
        for (const auto& row : series_) {
            p.setPen(Qt::NoPen);
            p.setBrush(colour(*this, row.colour));
            p.drawRoundedRect(QRectF(x, height() - 20, 8, 8), 2, 2);
            p.setPen(colour(*this, Role::Foreground));
            p.drawText(x + 14, height() - 12, row.name);
            x += fontMetrics().horizontalAdvance(row.name) + 36;
        }
    }
}
void Chart::mouseMoveEvent(QMouseEvent* event) {
    const auto area = plotRect();
    qsizetype count = 0;
    for (const auto& series : series_)
        count = std::max(count, series.values.size());
    if (count == 0 || !area.contains(event->position())) {
        QToolTip::hideText();
        hover_ = -1;
        return;
    }
    auto index = std::clamp(static_cast<int>((event->position().x() - area.left()) /
                                                   area.width() * static_cast<double>(count)),
                                  0, static_cast<int>(count) - 1);
    if (type_ == ChartType::Pie) {
        const auto delta = event->position() - area.center();
        const auto radius = std::min(area.width(), area.height()) / 2;
        if (std::hypot(delta.x(), delta.y()) > radius) {
            QToolTip::hideText();
            hover_ = -1;
            return;
        }
        const auto& values = series_.first().values;
        double scale = 1;
        for (double value : values) scale = std::max(scale, value);
        double sum = 0;
        for (double value : values) sum += std::max(0.0, value / scale);
        const double angle = std::fmod(std::atan2(delta.y(), delta.x()) + 2.5 * std::numbers::pi,
                                      2 * std::numbers::pi);
        const double target = angle / (2 * std::numbers::pi) * sum;
        double cumulative = 0;
        index = -1;
        for (qsizetype i = 0; i < values.size(); ++i) {
            cumulative += std::max(0.0, values[i] / scale);
            if (target < cumulative) { index = static_cast<int>(i); break; }
        }
        if (index < 0) { QToolTip::hideText(); hover_ = -1; return; }
    }
    if (index == hover_)
        return;
    hover_ = index;
    QStringList lines;
    if (index < labels_.size())
        lines << labels_[index];
    for (const auto& series : series_) {
        if (index < series.values.size()) lines << series.name + ": " + QString::number(series.values[index]);
        if (type_ == ChartType::Pie) break;
    }
    QToolTip::showText(event->globalPosition().toPoint(), lines.join('\n'), this);
}
void Chart::leaveEvent(QEvent* event) {
    hover_ = -1;
    QToolTip::hideText();
    QWidget::leaveEvent(event);
}
} // namespace shadcn
