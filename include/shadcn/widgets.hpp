// SPDX-License-Identifier: MIT
#pragma once

#include <shadcn/core.hpp>
#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QProgressBar>
#include <QProxyStyle>
#include <QPushButton>
#include <concepts>
#include <utility>

class QApplication;
class QHBoxLayout;
class QVBoxLayout;
class QVariantAnimation;

namespace shadcn {

/// Owns the theme values. QApplication owns an installed Style.
class Style final : public QProxyStyle {
    Q_OBJECT
public:
    explicit Style(Theme theme = Theme::neutral(), MotionPolicy motion = MotionPolicy::Full);
    [[nodiscard]] const Theme& theme() const noexcept { return theme_; }
    [[nodiscard]] MotionPolicy motion() const noexcept { return motion_; }
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                    const QWidget* widget = nullptr) const override;
    int styleHint(StyleHint hint, const QStyleOption* option = nullptr,
                  const QWidget* widget = nullptr, QStyleHintReturn* data = nullptr) const override;
private:
    Theme theme_;
    MotionPolicy motion_;
};

/// Install on the application's GUI thread. Uses the application's font family.
/// Passing zero or a negative fontPixels preserves the application's current font size.
void install(QApplication& app, Theme theme = Theme::neutral(),
             MotionPolicy motion = MotionPolicy::Full, int fontPixels = 14);

/// Construct a widget owned by its parent and return a borrowed reference.
/// Do not delete the reference or also place it in an owning smart pointer.
// moc does not need to parse this constrained free-function template.
#ifndef Q_MOC_RUN
template<class T, class... Args>
    requires std::derived_from<T, QWidget> && std::constructible_from<T, Args..., QWidget*>
T& make_child(QWidget& parent, Args&&... args) {
    return *new T(std::forward<Args>(args)..., &parent);
}
#endif

/// Native button activation with upstream variants, sizes and hover transitions.
class Button : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid)
public:
    explicit Button(const QString& text = {}, QWidget* parent = nullptr);
    [[nodiscard]] Variant variant() const noexcept { return variant_; }
    void setVariant(Variant variant);
    [[nodiscard]] ButtonSize buttonSize() const noexcept { return size_; }
    void setButtonSize(ButtonSize size);
    [[nodiscard]] bool isInvalid() const noexcept { return invalid_; }
    void setInvalid(bool invalid);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
    bool event(QEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
private:
    void updateHover();
    Variant variant_ = Variant::Default;
    ButtonSize size_ = ButtonSize::Default;
    bool invalid_ = false;
    double hoverAmount_ = 0;
    QVariantAnimation* hover_;
};

/// QLineEdit retains native selection, clipboard, undo, password and input method support.
/// File, date and number HTML input types need separate native components.
class Input : public QLineEdit {
    Q_OBJECT
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid)
public:
    explicit Input(QWidget* parent = nullptr);
    [[nodiscard]] bool isInvalid() const noexcept { return invalid_; }
    void setInvalid(bool invalid);
    /// Sets the error description exposed through Qt accessibility.
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

/// A static text badge. Link composition is tracked separately in the parity ledger.
class Badge : public QLabel {
    Q_OBJECT
public:
    explicit Badge(const QString& text = {}, QWidget* parent = nullptr);
    void setVariant(Variant variant);
    [[nodiscard]] Variant variant() const noexcept { return variant_; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    Variant variant_ = Variant::Default;
};

/// Label that focuses its Qt buddy or toggles a button when clicked.
class Label : public QLabel {
    Q_OBJECT
public:
    explicit Label(const QString& text = {}, QWidget* parent = nullptr);
    void setBuddy(QWidget* buddy);
protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;
private:
    QPointer<QWidget> buddy_;
    bool pressed_ = false;
};

/// Native checkbox semantics. The indeterminate state uses a dash.
class Checkbox : public QCheckBox {
    Q_OBJECT
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid)
public:
    explicit Checkbox(const QString& text = {}, QWidget* parent = nullptr);
    void setInvalid(bool invalid);
    [[nodiscard]] bool isInvalid() const noexcept { return invalid_; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
    bool hitButton(const QPoint& position) const override;
private:
    bool invalid_ = false;
};

enum class SwitchSize { Default, Sm };
/// Toggle switch. Set an accessible name or attach a Label before showing it.
class Switch : public QCheckBox {
    Q_OBJECT
public:
    explicit Switch(QWidget* parent = nullptr);
    void setSwitchSize(SwitchSize size);
    [[nodiscard]] SwitchSize switchSize() const noexcept { return size_; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    bool hitButton(const QPoint& position) const override;
    bool event(QEvent*) override;
private:
    void updatePosition();
    SwitchSize size_ = SwitchSize::Default;
    double position_ = 0;
    QVariantAnimation* transition_;
};

/// A decorative line one logical pixel thick.
class Separator : public QFrame {
    Q_OBJECT
public:
    explicit Separator(Qt::Orientation orientation = Qt::Horizontal, QWidget* parent = nullptr);
    void setOrientation(Qt::Orientation orientation);
    [[nodiscard]] Qt::Orientation orientation() const noexcept { return orientation_; }
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    Qt::Orientation orientation_;
};

/// Native progress state with a rounded track and animated determinate updates.
/// setRange(0, 0) exposes an indeterminate state and draws an empty track, as upstream does.
class Progress : public QProgressBar {
    Q_OBJECT
public:
    explicit Progress(QWidget* parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
public slots:
    void setRange(int minimum, int maximum);
    void setMinimum(int minimum);
    void setMaximum(int maximum);
protected:
    void paintEvent(QPaintEvent*) override;
    bool event(QEvent*) override;
private:
    void updateFraction();
    double fraction_ = 0;
    double target_ = 0;
    QVariantAnimation* transition_;
};

/// Pulse animation runs only while this widget is visible and motion is enabled.
class Skeleton : public QWidget {
    Q_OBJECT
public:
    explicit Skeleton(QWidget* parent = nullptr);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
    bool event(QEvent*) override;
private:
    void updateAnimation();
    double opacity_ = 1;
    QVariantAnimation* pulse_;
};

/// Card composition with owned header, content, footer and action layouts.
class Card : public QFrame {
    Q_OBJECT
public:
    explicit Card(QWidget* parent = nullptr);
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    /// Returned layouts are borrowed and remain owned by the card.
    [[nodiscard]] QVBoxLayout& content();
    [[nodiscard]] QHBoxLayout& footer();
    [[nodiscard]] QVBoxLayout& action();
protected:
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent*) override;
private:
    void updateHeader();
    void updatePalette();
    QWidget* header_;
    QLabel* title_;
    QLabel* description_;
    QWidget* contentHost_;
    QWidget* footerHost_;
    QWidget* actionHost_;
    QVBoxLayout* content_;
    QHBoxLayout* footer_;
    QVBoxLayout* action_;
};

} // namespace shadcn
