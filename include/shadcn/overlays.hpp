// SPDX-License-Identifier: MIT
#pragma once

#include <QDialog>
#include <QElapsedTimer>
#include <QTimer>
#include <shadcn/widgets.hpp>

class QGraphicsOpacityEffect;

namespace shadcn {

enum class Side { Top, Right, Bottom, Left };

/// Modal content with a backdrop, focus containment and focus restoration.
class Dialog : public QDialog {
    Q_OBJECT
  public:
    explicit Dialog(QWidget* parent = nullptr);
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    void setCloseButtonVisible(bool visible);
    void setDismissOnOutsideClick(bool enabled) { outsideDismiss_ = enabled; }
    void setContentWidth(int width);
    [[nodiscard]] QVBoxLayout& content() { return *content_; }
    [[nodiscard]] QHBoxLayout& footer() { return *footer_; }
    [[nodiscard]] QWidget& panel() { return *panel_; }
  public slots:
    void done(int result) override;

  protected:
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void changeEvent(QEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;
    virtual QRect panelGeometry() const;
    virtual QPoint animationOffset() const { return {}; }
    void arrangePanel();
    void refreshTheme();
    void transition(double target);
    QFrame* panel_;
    QVBoxLayout* panelLayout_;
    int contentWidth_ = 384;
    int duration_ = 100;
    double amount_ = 1;

  private:
    void fitOwner();
    QLabel* title_;
    QLabel* description_;
    Button* close_;
    QVBoxLayout* content_;
    QHBoxLayout* footer_;
    QFrame* footerHost_;
    QPointer<QWidget> previousFocus_;
    QPointer<QWidget> owner_;
    QVariantAnimation* animation_;
    QGraphicsOpacityEffect* opacity_;
    int result_ = Rejected;
    bool closing_ = false;
    bool outsideDismiss_ = true;
};

/// Confirmation dialog. Outside clicks do not dismiss it; Cancel receives initial focus.
class AlertDialog : public Dialog {
    Q_OBJECT
  public:
    explicit AlertDialog(QWidget* parent = nullptr);
    [[nodiscard]] Button& actionButton() { return *action_; }
    [[nodiscard]] Button& cancelButton() { return *cancel_; }

  protected:
    void showEvent(QShowEvent*) override;

  private:
    Button* action_;
    Button* cancel_;
};

/// Modal panel attached to an edge of its owner window.
class Sheet : public Dialog {
    Q_OBJECT
  public:
    explicit Sheet(Side side = Side::Right, QWidget* parent = nullptr);
    void setSide(Side side);
    [[nodiscard]] Side side() const { return side_; }

  protected:
    QRect panelGeometry() const override;
    QPoint animationOffset() const override;

  private:
    Side side_;
};

/// Bottom sheet with a drag handle and pointer dismissal.
class Drawer : public Sheet {
    Q_OBJECT
  public:
    explicit Drawer(QWidget* parent = nullptr);

  protected:
    bool eventFilter(QObject*, QEvent*) override;
    QPoint animationOffset() const override;

  private:
    QWidget* handle_;
    QPoint dragStart_;
    QRect dragGeometry_;
    QElapsedTimer dragTime_;
    bool dragging_ = false;
};

/// Anchored popup with screen edge collision handling and Qt popup dismissal.
class Popover : public QFrame {
    Q_OBJECT
  public:
    explicit Popover(QWidget* parent = nullptr);
    [[nodiscard]] QVBoxLayout& content() { return *content_; }
    void setContentWidth(int width);
    void showFor(QWidget& anchor, Side side = Side::Bottom, int offset = 4);
    [[nodiscard]] Side actualSide() const { return actualSide_; }

  protected:
    void paintEvent(QPaintEvent*) override;
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;
    virtual bool takesFocus() const { return true; }
    virtual Role backgroundRole() const { return Role::Popover; }
    virtual Role foregroundRole() const { return Role::PopoverForeground; }
    void positionPopup();
    QPointer<QWidget> anchor_;

  private:
    QVBoxLayout* content_;
    QPointer<QWidget> previousFocus_;
    QPointer<QWidget> owner_;
    Side side_ = Side::Bottom;
    Side actualSide_ = Side::Bottom;
    int offset_ = 4;
    int contentWidth_ = 288;
    QVariantAnimation* animation_;
};

/// Text hint shown on hover and keyboard focus without moving focus.
class Tooltip : public Popover {
    Q_OBJECT
  public:
    explicit Tooltip(const QString& text = {}, QWidget* parent = nullptr);
    void setText(const QString& text);
    void attach(QWidget& target, Side side = Side::Top);
    void setDelay(int milliseconds);

  protected:
    bool eventFilter(QObject*, QEvent*) override;
    bool takesFocus() const override { return false; }
    Role backgroundRole() const override { return Role::Foreground; }
    Role foregroundRole() const override { return Role::Background; }

  private:
    QLabel* label_;
    QPointer<QWidget> target_;
    QTimer delay_;
    Side side_ = Side::Top;
};

/// Hover content with delayed opening and closing. Content remains reachable by pointer.
class HoverCard : public Popover {
    Q_OBJECT
  public:
    explicit HoverCard(QWidget* parent = nullptr);
    void attach(QWidget& target);
    void setOpenDelay(int milliseconds);
    void setCloseDelay(int milliseconds);

  protected:
    bool eventFilter(QObject*, QEvent*) override;
    bool event(QEvent*) override;
    bool takesFocus() const override { return false; }

  private:
    QPointer<QWidget> target_;
    QTimer open_;
    QTimer close_;
};

} // namespace shadcn
