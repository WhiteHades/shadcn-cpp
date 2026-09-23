// SPDX-License-Identifier: MIT
#pragma once

#include <shadcn/widgets.hpp>

#include <QButtonGroup>
#include <QFrame>
#include <QHash>
#include <QLabel>
#include <QList>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVector>
#include <QStringList>

class QBoxLayout;
class QHBoxLayout;
class QVBoxLayout;

namespace shadcn {

enum class ToggleVariant { Default, Outline };
enum class ToggleSize { Default, Sm, Lg };
enum class ToggleGroupMode { Single, Multiple };
enum class AlertVariant { Default, Destructive };
enum class AvatarSize { Default, Sm, Lg };
enum class InputGroupAlign { InlineStart, InlineEnd, BlockStart, BlockEnd };
enum class InputGroupButtonSize { Xs, Sm, IconXs, IconSm };
enum class ItemVariant { Default, Outline, Muted };
enum class ItemSize { Default, Sm, Xs };
enum class ItemMediaVariant { Default, Icon, Image };

/// A plain multiline editor with the shadcn textarea dimensions and focus state.
class Textarea : public QPlainTextEdit {
    Q_OBJECT
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid)
public:
    explicit Textarea(QWidget* parent = nullptr);
    [[nodiscard]] bool isInvalid() const noexcept { return invalid_; }
    void setInvalid(bool invalid);
    void setError(const QString& message);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent*) override;
private:
    void updatePalette();
    bool invalid_ = false;
};

/// A checkable control with default and outline variants.
class Toggle : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid)
public:
    explicit Toggle(const QString& text = {}, QWidget* parent = nullptr);
    [[nodiscard]] ToggleVariant variant() const noexcept { return variant_; }
    void setVariant(ToggleVariant variant);
    [[nodiscard]] ToggleSize toggleSize() const noexcept { return size_; }
    void setToggleSize(ToggleSize size);
    [[nodiscard]] bool isInvalid() const noexcept { return invalid_; }
    void setInvalid(bool invalid);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
private:
    ToggleVariant variant_ = ToggleVariant::Default;
    ToggleSize size_ = ToggleSize::Default;
    bool invalid_ = false;
};

/// A group of toggles with single or multiple selection semantics.
class ToggleGroup : public QWidget {
    Q_OBJECT
    Q_PROPERTY(ToggleGroupMode mode READ mode WRITE setMode)
public:
    explicit ToggleGroup(Qt::Orientation orientation = Qt::Horizontal,
                         QWidget* parent = nullptr);
    void addToggle(Toggle& toggle, const QString& value = {});
    void removeToggle(Toggle& toggle);
    [[nodiscard]] QList<Toggle*> toggles() const;
    [[nodiscard]] ToggleGroupMode mode() const noexcept { return mode_; }
    void setMode(ToggleGroupMode mode);
    [[nodiscard]] Qt::Orientation orientation() const noexcept { return orientation_; }
    void setOrientation(Qt::Orientation orientation);
    void setSpacing(int spacing);
    [[nodiscard]] QStringList checkedValues() const;
    void setCheckedValues(const QStringList& values);
signals:
    void valuesChanged(const QStringList& values);
private:
    void onToggleChanged(Toggle* changed);
    QString valueFor(const Toggle& toggle) const;
    QBoxLayout* layout_;
    QList<QPointer<Toggle>> toggles_;
    QHash<Toggle*, QString> values_;
    Qt::Orientation orientation_;
    ToggleGroupMode mode_ = ToggleGroupMode::Single;
};

/// A multiline radio group item. Add it to RadioGroup with addItem.
class RadioGroupItem : public QRadioButton {
    Q_OBJECT
public:
    explicit RadioGroupItem(const QString& text = {}, QWidget* parent = nullptr);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
};

/// A vertical, keyboard native radio group.
class RadioGroup : public QWidget {
    Q_OBJECT
public:
    explicit RadioGroup(QWidget* parent = nullptr);
    void addItem(RadioGroupItem& item, const QString& value = {});
    void removeItem(RadioGroupItem& item);
    [[nodiscard]] QList<RadioGroupItem*> items() const;
    [[nodiscard]] QString checkedValue() const;
    void setCheckedValue(const QString& value);
signals:
    void valueChanged(const QString& value);
private:
    QVBoxLayout* layout_;
    QButtonGroup* buttons_;
    QList<QPointer<RadioGroupItem>> items_;
    QHash<RadioGroupItem*, QString> values_;
};

/// A keyboard and pointer controlled slider. One value makes a single thumb.
/// Two values make a range with two thumbs, matching the upstream default shape.
class Slider : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(QVector<double> values READ values WRITE setValues NOTIFY valuesChanged)
public:
    explicit Slider(QWidget* parent = nullptr);
    Slider(double minimum, double maximum, QWidget* parent = nullptr);
    [[nodiscard]] double minimum() const noexcept { return minimum_; }
    [[nodiscard]] double maximum() const noexcept { return maximum_; }
    void setMinimum(double minimum);
    void setMaximum(double maximum);
    void setRange(double minimum, double maximum);
    [[nodiscard]] QVector<double> values() const { return values_; }
    void setValues(const QVector<double>& values);
    [[nodiscard]] Qt::Orientation orientation() const noexcept { return orientation_; }
    void setOrientation(Qt::Orientation orientation);
    [[nodiscard]] double singleStep() const noexcept { return singleStep_; }
    void setSingleStep(double step);
    [[nodiscard]] double pageStep() const noexcept { return pageStep_; }
    void setPageStep(double step);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
signals:
    void valuesChanged(const QVector<double>& values);
protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void wheelEvent(QWheelEvent*) override;
private:
    friend class AccessibleSlider;
    friend class AccessibleSliderThumb;
    void updateAccessibleValue();
    QRectF trackRect() const;
    double valueAt(const QPointF& point) const;
    double positionFor(double value) const;
    int thumbAt(const QPointF& point) const;
    void setValueAt(int index, double value);
    double minimum_ = 0;
    double maximum_ = 100;
    double singleStep_ = 1;
    double pageStep_ = 10;
    QVector<double> values_{0, 100};
    Qt::Orientation orientation_ = Qt::Horizontal;
    int activeThumb_ = -1;
};

/// A non dismissible alert with default and destructive colour variants.
class Alert : public QFrame {
    Q_OBJECT
public:
    explicit Alert(QWidget* parent = nullptr);
    [[nodiscard]] AlertVariant variant() const noexcept { return variant_; }
    void setVariant(AlertVariant variant);
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    [[nodiscard]] QString title() const;
    [[nodiscard]] QString description() const;
    [[nodiscard]] QVBoxLayout& content();
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent*) override;
private:
    void updatePalette();
    AlertVariant variant_ = AlertVariant::Default;
    QLabel* title_;
    QLabel* description_;
    QVBoxLayout* content_;
};

/// A circular image or fallback avatar.
class Avatar : public QFrame {
    Q_OBJECT
public:
    explicit Avatar(const QString& fallback = {}, QWidget* parent = nullptr);
    [[nodiscard]] AvatarSize avatarSize() const noexcept { return size_; }
    void setAvatarSize(AvatarSize size);
    void setImage(const QPixmap& image);
    void clearImage();
    void setFallback(const QString& fallback);
    [[nodiscard]] QString fallback() const { return fallback_; }
    void setBadge(const QString& text);
    [[nodiscard]] QString badge() const { return badge_; }
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    AvatarSize size_ = AvatarSize::Default;
    QPixmap image_;
    QString fallback_;
    QString badge_;
};

/// An overlapping row of avatars with an optional overflow count.
class AvatarGroup : public QFrame {
    Q_OBJECT
public:
    explicit AvatarGroup(QWidget* parent = nullptr);
    void addAvatar(Avatar& avatar);
    void removeAvatar(Avatar& avatar);
    void setOverflowCount(int count);
    [[nodiscard]] int overflowCount() const noexcept { return overflow_; }
    [[nodiscard]] QList<Avatar*> avatars() const;
private:
    QHBoxLayout* layout_;
    QList<QPointer<Avatar>> avatars_;
    QLabel* overflowLabel_;
    int overflow_ = 0;
};

/// A widget that keeps its child content at a fixed width to height ratio.
class AspectRatio : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double ratio READ ratio WRITE setRatio)
public:
    explicit AspectRatio(double ratio = 1.0, QWidget* parent = nullptr);
    [[nodiscard]] double ratio() const noexcept { return ratio_; }
    void setRatio(double ratio);
    void setWidget(QWidget& widget);
    [[nodiscard]] QWidget* widget() const noexcept { return child_; }
    QSize sizeHint() const override;
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
protected:
    void resizeEvent(QResizeEvent*) override;
private:
    double ratio_ = 1;
    QPointer<QWidget> child_;
};

/// A compact navigation trail. Links are buttons, the last page is a label.
class Breadcrumb : public QFrame {
    Q_OBJECT
public:
    explicit Breadcrumb(QWidget* parent = nullptr);
    QPushButton& addLink(const QString& text);
    QLabel& addPage(const QString& text);
    QLabel& addSeparator(const QString& text = QString::fromUtf8("›"));
    QLabel& addEllipsis();
    void clear();
    [[nodiscard]] int count() const;
signals:
    void linkActivated(const QString& text);
private:
    QHBoxLayout* layout_;
};

/// A contiguous horizontal or vertical group of native controls.
class ButtonGroup : public QFrame {
    Q_OBJECT
public:
    explicit ButtonGroup(Qt::Orientation orientation = Qt::Horizontal,
                         QWidget* parent = nullptr);
    void addWidget(QWidget& widget);
    void addButton(Button& button);
    Separator& addSeparator(Qt::Orientation orientation = Qt::Vertical);
    QLabel& addText(const QString& text);
    void setOrientation(Qt::Orientation orientation);
    [[nodiscard]] Qt::Orientation orientation() const noexcept { return orientation_; }
private:
    QBoxLayout* layout_;
    Qt::Orientation orientation_;
};

/// A bordered composition for an input, addons, text and small buttons.
class InputGroup : public QFrame {
    Q_OBJECT
public:
    explicit InputGroup(QWidget* parent = nullptr);
    void addWidget(QWidget& widget, InputGroupAlign align = InputGroupAlign::InlineStart);
    void addInput(Input& input);
    void addTextarea(Textarea& textarea);
    Button& addButton(const QString& text, InputGroupButtonSize size = InputGroupButtonSize::Xs);
    QLabel& addText(const QString& text);
    void setInvalid(bool invalid);
    [[nodiscard]] bool isInvalid() const noexcept { return invalid_; }
    void setError(const QString& message);
protected:
    void paintEvent(QPaintEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;
private:
    QBoxLayout* layout_;
    bool invalid_ = false;
};

/// A small keyboard key label and an inline group for shortcuts.
class Kbd : public QLabel {
    Q_OBJECT
public:
    explicit Kbd(const QString& text = {}, QWidget* parent = nullptr);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
};

class KbdGroup : public QFrame {
    Q_OBJECT
public:
    explicit KbdGroup(QWidget* parent = nullptr);
    void addKey(Kbd& key);
    Kbd& addKey(const QString& text);
    [[nodiscard]] int count() const;
private:
    QHBoxLayout* layout_;
};

/// A loading indicator that pauses while hidden and respects reduced motion.
class Spinner : public QWidget {
    Q_OBJECT
public:
    explicit Spinner(QWidget* parent = nullptr);
    [[nodiscard]] bool isSpinning() const noexcept { return spinning_; }
    void setSpinning(bool spinning);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
    void changeEvent(QEvent*) override;
private:
    void updateTimer();
    QTimer timer_;
    int angle_ = 0;
    bool spinning_ = true;
};

/// A centred empty state with optional title, description and content widgets.
class Empty : public QFrame {
    Q_OBJECT
public:
    explicit Empty(QWidget* parent = nullptr);
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    [[nodiscard]] QString title() const;
    [[nodiscard]] QString description() const;
    void addWidget(QWidget& widget);
    [[nodiscard]] QVBoxLayout& content();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QLabel* title_;
    QLabel* description_;
    QVBoxLayout* content_;
};

/// A list row with default, outline and muted variants.
class Item : public QFrame {
    Q_OBJECT
public:
    explicit Item(QWidget* parent = nullptr);
    void setVariant(ItemVariant variant);
    [[nodiscard]] ItemVariant variant() const noexcept { return variant_; }
    void setItemSize(ItemSize size);
    [[nodiscard]] ItemSize itemSize() const noexcept { return size_; }
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    void addLeading(QWidget& widget);
    void addTrailing(QWidget& widget);
    [[nodiscard]] QVBoxLayout& content();
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    ItemVariant variant_ = ItemVariant::Default;
    ItemSize size_ = ItemSize::Default;
    QHBoxLayout* row_;
    QVBoxLayout* content_;
    QLabel* title_;
    QLabel* description_;
    QWidget* leading_ = nullptr;
    QWidget* trailing_ = nullptr;
};

/// A vertical owner for Item rows.
class ItemGroup : public QFrame {
    Q_OBJECT
public:
    explicit ItemGroup(QWidget* parent = nullptr);
    void addItem(Item& item);
    void addSeparator();
    [[nodiscard]] int count() const;
private:
    QVBoxLayout* layout_;
};

} // namespace shadcn
