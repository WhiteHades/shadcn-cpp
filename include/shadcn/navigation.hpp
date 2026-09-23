// SPDX-License-Identifier: MIT
// Design source: shadcn-ui/ui radix-nova wrappers and style-nova.css.
#pragma once

#include <shadcn/widgets.hpp>

#include <QActionGroup>
#include <QFrame>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QStringList>
#include <QToolButton>
#include <QWidget>

#include <functional>

class QBoxLayout;
class QButtonGroup;
class QGraphicsOpacityEffect;
class QHBoxLayout;
class QLabel;
class QLayout;
class QVBoxLayout;
class QVariantAnimation;

namespace shadcn {

/// Direction context translated to Qt's layout direction.
enum class Direction { LeftToRight, RightToLeft };

class DirectionProvider : public QWidget {
    Q_OBJECT
public:
    explicit DirectionProvider(Direction direction = Direction::LeftToRight,
                               QWidget* parent = nullptr);
    [[nodiscard]] Direction direction() const noexcept { return direction_; }
    void setDirection(Direction direction);
    [[nodiscard]] QVBoxLayout& content();
signals:
    void directionChanged(shadcn::Direction direction);
private:
    Direction direction_;
    QVBoxLayout* content_;
};

/// One accordion item with a keyboard accessible trigger and animated content.
class AccordionItem : public QWidget {
    Q_OBJECT
public:
    explicit AccordionItem(const QString& title = {}, QWidget* parent = nullptr);
    [[nodiscard]] QString title() const;
    void setTitle(const QString& title);
    [[nodiscard]] bool isExpanded() const noexcept { return expanded_; }
    void setExpanded(bool expanded);
    void toggle();
    [[nodiscard]] QPushButton& trigger() { return *trigger_; }
    [[nodiscard]] QVBoxLayout& content();
    void setContentWidget(QWidget& widget);
signals:
    void expandedChanged(bool expanded);
protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    void updateContent(bool animate);
    QPushButton* trigger_;
    QWidget* contentHost_;
    QVBoxLayout* content_;
    QVariantAnimation* animation_;
    bool expanded_ = false;
};

/// A grouped accordion. Multiple open items can be enabled explicitly.
class Accordion : public QWidget {
    Q_OBJECT
public:
    explicit Accordion(QWidget* parent = nullptr);
    [[nodiscard]] AccordionItem& addItem(const QString& title);
    void addItem(AccordionItem& item);
    void removeItem(AccordionItem& item);
    [[nodiscard]] QList<AccordionItem*> items() const;
    [[nodiscard]] bool allowsMultiple() const noexcept { return multiple_; }
    void setAllowsMultiple(bool multiple);
    [[nodiscard]] QStringList expandedTitles() const;
    void setExpandedTitles(const QStringList& titles);
signals:
    void expandedChanged(const QStringList& titles);
private:
    void onItemChanged(AccordionItem& changed, bool expanded);
    QVBoxLayout* layout_;
    QList<QPointer<AccordionItem>> items_;
    bool multiple_ = false;
};

/// A single trigger and content region with animated open and closed states.
class Collapsible : public QWidget {
    Q_OBJECT
public:
    explicit Collapsible(const QString& title = {}, QWidget* parent = nullptr);
    [[nodiscard]] QPushButton& trigger() { return *trigger_; }
    [[nodiscard]] QVBoxLayout& content();
    void setContentWidget(QWidget& widget);
    [[nodiscard]] bool isOpen() const noexcept { return open_; }
    void setOpen(bool open);
    void toggle();
signals:
    void openChanged(bool open);
private:
    void updateContent(bool animate);
    QPushButton* trigger_;
    QWidget* contentHost_;
    QVBoxLayout* content_;
    QVariantAnimation* animation_;
    bool open_ = false;
};

enum class TabsListVariant { Default, Line };
/// A value based tab set with native focus and arrow key navigation.
class Tabs : public QWidget {
    Q_OBJECT
public:
    explicit Tabs(Qt::Orientation orientation = Qt::Horizontal,
                  QWidget* parent = nullptr);
    [[nodiscard]] Qt::Orientation orientation() const noexcept { return orientation_; }
    void setOrientation(Qt::Orientation orientation);
    void setListVariant(TabsListVariant variant);
    [[nodiscard]] TabsListVariant listVariant() const noexcept { return variant_; }
    [[nodiscard]] QPushButton& addTab(const QString& value, const QString& label);
    /// The supplied content remains caller owned and is detached on removal.
    void addContent(const QString& value, QWidget& content);
    /// Creates content owned by Tabs. It is deleted when its tab is removed.
    [[nodiscard]] QWidget& addContent(const QString& value);
    void removeTab(const QString& value);
    [[nodiscard]] QString currentValue() const;
    void setCurrentValue(const QString& value);
    [[nodiscard]] QStringList values() const;
    [[nodiscard]] QHBoxLayout& list();
signals:
    void currentChanged(const QString& value);
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    struct Entry {
        QString value;
        QPointer<QPushButton> button;
        QPointer<QWidget> content;
        bool ownedContent = false;
    };
    void select(int index, bool focus);
    int indexFor(const QString& value) const;
    void restyle();
    Qt::Orientation orientation_;
    TabsListVariant variant_ = TabsListVariant::Default;
    QWidget* listHost_;
    QHBoxLayout* list_;
    QStackedWidget* stack_;
    QList<Entry> entries_;
    int current_ = -1;
};

/// A QScrollArea with Nova scrollbars and the usual focus ring.
class ScrollArea : public QScrollArea {
    Q_OBJECT
public:
    explicit ScrollArea(QWidget* parent = nullptr);
    void setHorizontalScrollBarVisible(bool visible);
    void setVerticalScrollBarVisible(bool visible);
protected:
    void paintEvent(QPaintEvent* event) override;
};

/// A splitter panel. The panel itself remains an ordinary QWidget for composition.
class ResizablePanel : public QWidget {
    Q_OBJECT
public:
    explicit ResizablePanel(QWidget* parent = nullptr) : QWidget(parent) {}
};

class ResizableHandle;
/// A keyboard and pointer resizable panel group.
class ResizablePanelGroup : public QSplitter {
    Q_OBJECT
public:
    explicit ResizablePanelGroup(Qt::Orientation orientation = Qt::Horizontal,
                                 QWidget* parent = nullptr);
    [[nodiscard]] Qt::Orientation orientation() const noexcept { return orientation_; }
    void setOrientation(Qt::Orientation orientation);
    void addPanel(QWidget& panel);
    void setPanelSizes(const QList<int>& sizes);
    [[nodiscard]] QList<int> panelSizes() const;
    void setHandleVisible(bool visible);
protected:
    QSplitterHandle* createHandle() override;
private:
    Qt::Orientation orientation_;
    bool handleVisible_ = true;
};

/// Styled splitter handle. QSplitter supplies correct keyboard and mouse semantics.
class ResizableHandle : public QSplitterHandle {
    Q_OBJECT
public:
    ResizableHandle(Qt::Orientation orientation, QSplitter* parent);
protected:
    void paintEvent(QPaintEvent*) override;
};

enum class SidebarSide { Left, Right };
enum class SidebarCollapsible { None, Offcanvas, Icon };
enum class SidebarVariant { Sidebar, Floating, Inset };
enum class SidebarMenuSize { Default, Sm, Lg };
enum class SidebarMenuVariant { Default, Outline };

/// A composable sidebar with expanded, icon and offcanvas states.
class Sidebar : public QFrame {
    Q_OBJECT
public:
    explicit Sidebar(QWidget* parent = nullptr);
    [[nodiscard]] SidebarSide side() const noexcept { return side_; }
    void setSide(SidebarSide side);
    [[nodiscard]] SidebarCollapsible collapsible() const noexcept { return collapsible_; }
    void setCollapsible(SidebarCollapsible mode);
    [[nodiscard]] SidebarVariant variant() const noexcept { return variant_; }
    void setVariant(SidebarVariant variant);
    [[nodiscard]] bool isOpen() const noexcept { return open_; }
    void setOpen(bool open);
    void toggle();
    [[nodiscard]] int expandedWidth() const noexcept { return expandedWidth_; }
    void setExpandedWidth(int width);
    [[nodiscard]] int iconWidth() const noexcept { return iconWidth_; }
    void setIconWidth(int width);
    [[nodiscard]] QVBoxLayout& header();
    [[nodiscard]] QVBoxLayout& content();
    [[nodiscard]] QVBoxLayout& footer();
    [[nodiscard]] QPushButton& addMenuButton(const QString& text, bool active = false,
                                             SidebarMenuVariant variant = SidebarMenuVariant::Default,
                                             SidebarMenuSize size = SidebarMenuSize::Default);
signals:
    void openChanged(bool open);
protected:
    void paintEvent(QPaintEvent*) override;
    bool event(QEvent* event) override;
private:
    void updateWidth(bool animate);
    SidebarSide side_ = SidebarSide::Left;
    SidebarCollapsible collapsible_ = SidebarCollapsible::Offcanvas;
    SidebarVariant variant_ = SidebarVariant::Sidebar;
    bool open_ = true;
    int expandedWidth_ = 256;
    int iconWidth_ = 48;
    QWidget* headerHost_;
    QWidget* contentHost_;
    QWidget* footerHost_;
    QVBoxLayout* header_;
    QVBoxLayout* content_;
    QVBoxLayout* footer_;
    QVariantAnimation* animation_;
    double widthAmount_ = 1;
};

/// Owns one or more sidebars and provides the Ctrl or Meta plus B shortcut.
class SidebarProvider : public QWidget {
    Q_OBJECT
public:
    explicit SidebarProvider(QWidget* parent = nullptr);
    void addSidebar(Sidebar& sidebar);
    [[nodiscard]] QList<Sidebar*> sidebars() const;
    void setShortcutEnabled(bool enabled);
    [[nodiscard]] bool shortcutEnabled() const noexcept { return shortcutEnabled_; }
    void toggleSidebar();
    [[nodiscard]] QVBoxLayout& content();
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    QHBoxLayout* layout_;
    QVBoxLayout* content_;
    QList<QPointer<Sidebar>> sidebars_;
    bool shortcutEnabled_ = true;
};

/// A trigger that finds and toggles its nearest SidebarProvider.
class SidebarTrigger : public Button {
    Q_OBJECT
public:
    explicit SidebarTrigger(QWidget* parent = nullptr);
protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
};

/// Main content surface used beside a sidebar.
class SidebarInset : public QFrame {
    Q_OBJECT
public:
    explicit SidebarInset(QWidget* parent = nullptr);
    [[nodiscard]] QVBoxLayout& content() { return *content_; }
private:
    QVBoxLayout* content_;
};

/// A horizontal navigation list with one open popup or page at a time.
class NavigationMenu : public QFrame {
    Q_OBJECT
public:
    explicit NavigationMenu(QWidget* parent = nullptr);
    [[nodiscard]] QPushButton& addLink(const QString& text, const QString& value = {});
    [[nodiscard]] QPushButton& addMenu(const QString& text, const QString& value = {});
    void setMenuContent(const QString& value, QWidget& content);
    void setCurrentValue(const QString& value);
    [[nodiscard]] QString currentValue() const;
    [[nodiscard]] QHBoxLayout& list();
signals:
    void currentChanged(const QString& value);
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    struct Entry {
        QString value;
        QPointer<QPushButton> button;
        QPointer<QWidget> content;
        bool menu = false;
    };
    void choose(int index);
    QHBoxLayout* list_;
    QFrame* popup_;
    QVBoxLayout* popupLayout_;
    QList<Entry> entries_;
    int current_ = -1;
};

/// Native QMenuBar with Nova spacing and theme colours.
class Menubar : public QMenuBar {
    Q_OBJECT
public:
    explicit Menubar(QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent* event) override;
};

/// A popup menu with composable item helpers.
class DropdownMenu : public QMenu {
    Q_OBJECT
public:
    explicit DropdownMenu(QWidget* parent = nullptr);
    QAction& addItem(const QString& text, const std::function<void()>& callback = {});
    QAction& addCheckboxItem(const QString& text, bool checked = false,
                             const std::function<void(bool)>& callback = {});
    QAction& addRadioItem(const QString& text, bool checked = false,
                          const std::function<void()>& callback = {});
    QAction& addLabel(const QString& text);
    void addSeparatorLine();
    [[nodiscard]] QMenu& addSubmenu(const QString& text);
private:
    QActionGroup* radioGroup_;
};

/// A right click menu that can be attached to any QWidget.
class ContextMenu : public DropdownMenu {
    Q_OBJECT
public:
    explicit ContextMenu(QWidget* parent = nullptr);
    void attach(QWidget& target);
    void detach();
    void popupAt(const QPoint& globalPosition);
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    QPointer<QWidget> target_;
};

/// A page carousel with keyboard arrows and previous or next controls.
class Carousel : public QFrame {
    Q_OBJECT
public:
    explicit Carousel(Qt::Orientation orientation = Qt::Horizontal,
                      QWidget* parent = nullptr);
    [[nodiscard]] Qt::Orientation orientation() const noexcept { return orientation_; }
    void setOrientation(Qt::Orientation orientation);
    /// The supplied slide remains caller owned and is detached on removal.
    void addSlide(QWidget& slide);
    /// Removes and detaches the slide without deleting it.
    void removeSlide(QWidget& slide);
    [[nodiscard]] int currentIndex() const noexcept { return current_; }
    void setCurrentIndex(int index);
    void scrollPrevious();
    void scrollNext();
    [[nodiscard]] bool canScrollPrevious() const noexcept;
    [[nodiscard]] bool canScrollNext() const noexcept;
    [[nodiscard]] QStackedWidget& content() { return *stack_; }
    [[nodiscard]] Button& previousButton() { return *previous_; }
    [[nodiscard]] Button& nextButton() { return *next_; }
signals:
    void currentChanged(int index);
protected:
    void keyPressEvent(QKeyEvent* event) override;
private:
    void updateButtons();
    Qt::Orientation orientation_;
    QStackedWidget* stack_;
    Button* previous_;
    Button* next_;
    int current_ = -1;
};

/// A compact page navigation control with disabled edge buttons.
class Pagination : public QFrame {
    Q_OBJECT
public:
    explicit Pagination(QWidget* parent = nullptr);
    void setPageCount(int count);
    [[nodiscard]] int pageCount() const noexcept { return pageCount_; }
    void setCurrentPage(int page);
    [[nodiscard]] int currentPage() const noexcept { return currentPage_; }
    void setShowEllipsis(bool show);
    [[nodiscard]] bool showEllipsis() const noexcept { return showEllipsis_; }
    void setPreviousText(const QString& text);
    void setNextText(const QString& text);
signals:
    void pageChanged(int page);
private:
    void rebuild();
    QHBoxLayout* layout_;
    int pageCount_ = 1;
    int currentPage_ = 1;
    bool showEllipsis_ = true;
    QString previousText_;
    QString nextText_;
};

} // namespace shadcn
