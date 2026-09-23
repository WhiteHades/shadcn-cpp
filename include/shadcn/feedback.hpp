// SPDX-License-Identifier: MIT
#pragma once

#include <shadcn/controls.hpp>

#include <QAbstractButton>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QFrame>
#include <QHash>
#include <QLabel>
#include <QList>
#include <QPointer>
#include <QPixmap>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QUrl>

class QBoxLayout;
class QButtonGroup;
class QHBoxLayout;
class QVBoxLayout;

namespace shadcn {

enum class ToastType { Default, Success, Info, Warning, Error, Loading };
enum class ToastPosition { Top, TopRight, TopLeft, Bottom, BottomRight, BottomLeft };

/// One timed notification. Hovering or focusing it pauses its lifetime.
class Toast : public QFrame {
    Q_OBJECT
public:
    Toast(int id, const QString& title = {}, const QString& description = {},
          QWidget* parent = nullptr);
    [[nodiscard]] int id() const noexcept { return id_; }
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    void setType(ToastType type);
    [[nodiscard]] ToastType type() const noexcept { return type_; }
    void setDuration(int milliseconds);
    [[nodiscard]] int duration() const noexcept { return duration_; }
    void setActionText(const QString& text);
    [[nodiscard]] QString actionText() const;
    void pause();
    void resume();
    [[nodiscard]] bool isPaused() const noexcept { return paused_; }
public slots:
    void dismiss();
signals:
    void actionTriggered(int id);
    void dismissed(int id);
protected:
    bool event(QEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void paintEvent(QPaintEvent*) override;
private:
    void expire();
    void updateIcon();
    void announce(const QString& text) const;
    int id_;
    int duration_ = 4000;
    int remaining_ = 4000;
    ToastType type_ = ToastType::Default;
    bool paused_ = false;
    bool dismissed_ = false;
    QElapsedTimer elapsed_;
    QTimer* timer_;
    QLabel* icon_;
    QLabel* title_;
    QLabel* description_;
    Button* action_;
    Button* close_;
};

/// A notification host with stacking, action and dismissal handling.
class Sonner : public QFrame {
    Q_OBJECT
public:
    explicit Sonner(QWidget* parent = nullptr);
    int showToast(const QString& title, const QString& description = {},
                  ToastType type = ToastType::Default, int duration = 4000,
                  const QString& actionText = {});
    Toast& addToast(const QString& title, const QString& description = {},
                    ToastType type = ToastType::Default, int duration = 4000,
                    const QString& actionText = {});
    void dismiss(int id);
    void dismissAll();
    [[nodiscard]] int toastCount() const;
    [[nodiscard]] QList<Toast*> toasts() const;
    void setMaxVisible(int count);
    [[nodiscard]] int maxVisible() const noexcept { return maxVisible_; }
    void setPosition(ToastPosition position);
    [[nodiscard]] ToastPosition position() const noexcept { return position_; }
    void attachTo(QWidget& window, ToastPosition position = ToastPosition::BottomRight);
signals:
    void toastAdded(int id);
    void toastRemoved(int id);
protected:
    bool eventFilter(QObject*, QEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void showEvent(QShowEvent*) override;
private:
    void removeToast(int id);
    void positionHost();
    QVBoxLayout* layout_;
    QList<QPointer<Toast>> toasts_;
    QPointer<QWidget> attachedWindow_;
    int nextId_ = 1;
    int maxVisible_ = 5;
    ToastPosition position_ = ToastPosition::BottomRight;
};

enum class AttachmentState { Idle, Uploading, Processing, Error, Done };
enum class AttachmentSize { Default, Sm, Xs };
enum class AttachmentOrientation { Horizontal, Vertical };

/// A file or URL card. It stores metadata and emits open requests without reading the source.
class Attachment : public QFrame {
    Q_OBJECT
public:
    explicit Attachment(QWidget* parent = nullptr);
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    void setSource(const QUrl& source);
    void setFilePath(const QString& path);
    void setPreview(const QPixmap& preview);
    void setState(AttachmentState state);
    void setSize(AttachmentSize size);
    void setOrientation(AttachmentOrientation orientation);
    [[nodiscard]] QString title() const;
    [[nodiscard]] QString description() const;
    [[nodiscard]] QUrl source() const { return source_; }
    [[nodiscard]] AttachmentState state() const noexcept { return state_; }
    [[nodiscard]] AttachmentSize size() const noexcept { return size_; }
    [[nodiscard]] AttachmentOrientation orientation() const noexcept { return orientation_; }
    [[nodiscard]] Button& addAction(const QString& text);
    QSize sizeHint() const override;
signals:
    void openRequested(const QUrl& source);
    void actionTriggered(const QString& text);
    void dismissed();
protected:
    void mousePressEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void paintEvent(QPaintEvent*) override;
private:
    void updateLayout();
    QColor stateColour() const;
    QUrl source_;
    AttachmentState state_ = AttachmentState::Done;
    AttachmentSize size_ = AttachmentSize::Default;
    AttachmentOrientation orientation_ = AttachmentOrientation::Horizontal;
    QLabel* media_;
    QLabel* title_;
    QLabel* description_;
    QWidget* contentHost_;
    QBoxLayout* outer_;
    QVBoxLayout* content_;
    QHBoxLayout* actions_;
    QPixmap preview_;
};

class AttachmentAction : public Button {
    Q_OBJECT
public:
    explicit AttachmentAction(const QString& text = {}, QWidget* parent = nullptr);
};

class AttachmentGroup : public QFrame {
    Q_OBJECT
public:
    explicit AttachmentGroup(QWidget* parent = nullptr);
    void addAttachment(Attachment& attachment);
    void removeAttachment(Attachment& attachment);
    [[nodiscard]] int count() const;
private:
    QHBoxLayout* layout_;
};

enum class BubbleVariant { Default, Secondary, Muted, Tinted, Outline, Ghost, Destructive };
enum class BubbleAlign { Start, End };

/// A chat bubble with Nova variants and start or end alignment.
class Bubble : public QFrame {
    Q_OBJECT
public:
    explicit Bubble(const QString& text = {}, QWidget* parent = nullptr);
    void setText(const QString& text);
    [[nodiscard]] QString text() const;
    void setVariant(BubbleVariant variant);
    [[nodiscard]] BubbleVariant variant() const noexcept { return variant_; }
    void setAlign(BubbleAlign align);
    [[nodiscard]] BubbleAlign align() const noexcept { return align_; }
    void addWidget(QWidget& widget);
    [[nodiscard]] QVBoxLayout& content();
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    BubbleVariant variant_ = BubbleVariant::Default;
    BubbleAlign align_ = BubbleAlign::Start;
    QLabel* text_;
    QVBoxLayout* content_;
};

class BubbleGroup : public QFrame {
    Q_OBJECT
public:
    explicit BubbleGroup(QWidget* parent = nullptr);
    void addBubble(Bubble& bubble);
    [[nodiscard]] int count() const;
private:
    QVBoxLayout* layout_;
};

class BubbleReactions : public QFrame {
    Q_OBJECT
public:
    explicit BubbleReactions(QWidget* parent = nullptr);
    void addWidget(QWidget& widget);
    void setSide(Qt::Edge side);
    void setAlign(BubbleAlign align);
private:
    QHBoxLayout* layout_;
    Qt::Edge side_ = Qt::BottomEdge;
    BubbleAlign align_ = BubbleAlign::End;
};

enum class MessageAlign { Start, End };

class MessageAvatar : public QFrame {
    Q_OBJECT
public:
    explicit MessageAvatar(QWidget* parent = nullptr);
    void setAvatar(Avatar& avatar);
};

class MessageHeader : public QFrame {
    Q_OBJECT
public:
    explicit MessageHeader(QWidget* parent = nullptr);
    void addWidget(QWidget& widget);
};

class MessageFooter : public QFrame {
    Q_OBJECT
public:
    explicit MessageFooter(QWidget* parent = nullptr);
    void addWidget(QWidget& widget);
};

/// A message row with optional avatar, content, header and footer slots.
class Message : public QFrame {
    Q_OBJECT
public:
    explicit Message(QWidget* parent = nullptr);
    void setAlign(MessageAlign align);
    [[nodiscard]] MessageAlign align() const noexcept { return align_; }
    void setText(const QString& text);
    [[nodiscard]] QString text() const;
    void setAvatar(Avatar& avatar);
    void addContent(QWidget& widget);
    void addHeader(QWidget& widget);
    void addFooter(QWidget& widget);
    [[nodiscard]] QVBoxLayout& content();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    MessageAlign align_ = MessageAlign::Start;
    QHBoxLayout* row_;
    QWidget* avatarHost_;
    QWidget* bodyHost_;
    QVBoxLayout* body_;
    QVBoxLayout* content_;
    QLabel* text_;
    QWidget* headerHost_;
    QWidget* footerHost_;
};

class MessageGroup : public QFrame {
    Q_OBJECT
public:
    explicit MessageGroup(QWidget* parent = nullptr);
    void addMessage(Message& message);
    [[nodiscard]] int count() const;
private:
    QVBoxLayout* layout_;
};

class MessageScrollerItem : public QFrame {
    Q_OBJECT
public:
    explicit MessageScrollerItem(QWidget* parent = nullptr);
    void addWidget(QWidget& widget);
private:
    QVBoxLayout* layout_;
};

class MessageScrollerButton : public Button {
    Q_OBJECT
public:
    enum class Direction { Start, End };
    explicit MessageScrollerButton(Direction direction = Direction::End,
                                   QWidget* parent = nullptr);
    [[nodiscard]] Direction direction() const noexcept { return direction_; }
private:
    Direction direction_;
};

/// A scrollable message surface with end tracking and accessible scroll controls.
class MessageScroller : public QFrame {
    Q_OBJECT
public:
    explicit MessageScroller(QWidget* parent = nullptr);
    void addWidget(QWidget& widget);
    void addItem(MessageScrollerItem& item);
    void scrollToStart();
    void scrollToEnd();
    [[nodiscard]] bool isAtStart() const;
    [[nodiscard]] bool isAtEnd() const;
    void setAutoScroll(bool enabled);
    [[nodiscard]] bool autoScroll() const noexcept { return autoScroll_; }
    [[nodiscard]] QVBoxLayout& content();
    [[nodiscard]] QScrollArea& viewport() { return *area_; }
signals:
    void startVisibilityChanged(bool visible);
    void endVisibilityChanged(bool visible);
protected:
    bool eventFilter(QObject*, QEvent*) override;
private:
    void updateButtons();
    QScrollArea* area_;
    QWidget* contentHost_;
    QVBoxLayout* content_;
    MessageScrollerButton* startButton_;
    MessageScrollerButton* endButton_;
    bool autoScroll_ = true;
    bool startVisible_ = false;
    bool endVisible_ = false;
};

enum class QuestionnaireType { Single, Multiple };

/// A local question flow with selectable choices and no network or model dependency.
class Questionnaire : public QFrame {
    Q_OBJECT
public:
    explicit Questionnaire(QWidget* parent = nullptr);
    int addQuestion(const QString& title, const QString& description = {},
                    QuestionnaireType type = QuestionnaireType::Single);
    void addChoice(int question, const QString& id, const QString& label,
                   const QString& description = {}, const QString& shortcut = {});
    [[nodiscard]] int questionCount() const;
    [[nodiscard]] int currentQuestion() const noexcept { return current_; }
    void setCurrentQuestion(int index);
    [[nodiscard]] QStringList answer(int question) const;
    void setAnswer(int question, const QStringList& ids);
    [[nodiscard]] QHash<int, QStringList> answers() const;
    void next();
    void previous();
    void skip();
    void submit();
signals:
    void answerChanged(int question, const QStringList& ids);
    void questionChanged(int question);
    void skipped(int question);
    void submitted();
private:
    struct Choice {
        QString id;
        QString label;
        QString description;
        QString shortcut;
    };
    struct Question {
        QString title;
        QString description;
        QuestionnaireType type = QuestionnaireType::Single;
        QList<Choice> choices;
        QStringList answers;
    };
    void render();
    void updateChoice(int question, const QString& id, bool checked);
    QList<Question> questions_;
    int current_ = -1;
    QVBoxLayout* layout_;
    QLabel* progress_;
    QPointer<QButtonGroup> choiceGroup_;
};

enum class MarkerVariant { Default, Separator, Border };

/// A small status marker with default, separator and border variants.
class Marker : public QFrame {
    Q_OBJECT
public:
    explicit Marker(const QString& text = {}, QWidget* parent = nullptr);
    void setText(const QString& text);
    [[nodiscard]] QString text() const;
    void setVariant(MarkerVariant variant);
    [[nodiscard]] MarkerVariant variant() const noexcept { return variant_; }
    void setIcon(const QPixmap& icon);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    MarkerVariant variant_ = MarkerVariant::Default;
    QLabel* icon_;
    QLabel* text_;
    QFrame* before_;
    QFrame* after_;
    QHBoxLayout* layout_;
};

} // namespace shadcn
