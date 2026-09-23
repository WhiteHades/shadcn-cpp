// SPDX-License-Identifier: MIT
#pragma once
#include <QCalendarWidget>
#include <QComboBox>
#include <QDate>
#include <QListWidget>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <functional>
#include <shadcn/overlays.hpp>
#include <shadcn/widgets.hpp>

namespace shadcn {

/// Select using Qt's native option model and keyboard handling.
class NativeSelect : public QComboBox {
    Q_OBJECT
  public:
    explicit NativeSelect(QWidget* parent = nullptr);
    void setInvalid(bool invalid);
    [[nodiscard]] bool isInvalid() const { return invalid_; }
    QSize sizeHint() const override;

  protected:
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent*) override;
    void refreshTheme();

  private:
    bool invalid_ = false;
};

/// Styled option popup with item labels, disabled items and separators.
class Select : public NativeSelect {
    Q_OBJECT
  public:
    explicit Select(QWidget* parent = nullptr);
    void addGroup(const QString& label);
    void setItemEnabled(int index, bool enabled);
    void showPopup() override;
};

/// Searchable select with case insensitive substring completion.
class Combobox : public Select {
    Q_OBJECT
  public:
    explicit Combobox(QWidget* parent = nullptr);
    void setPlaceholderText(const QString& text);
};

/// Filterable command list. Item data is returned when a command is activated.
class Command : public QWidget {
    Q_OBJECT
  public:
    explicit Command(QWidget* parent = nullptr);
    [[nodiscard]] Input& search() { return *search_; }
    [[nodiscard]] QListWidget& list() { return *list_; }
    QListWidgetItem& addItem(const QString& label, const QVariant& data = {},
                             const QStringList& keywords = {});
    void addGroup(const QString& label);
    void setEmptyText(const QString& text);
  signals:
    void triggered(const QVariant& data);

  protected:
    bool eventFilter(QObject*, QEvent*) override;
    void changeEvent(QEvent*) override;

  private:
    void filter(const QString& text);
    void refreshTheme();
    Input* search_;
    QListWidget* list_;
    QLabel* empty_;
};

/// Calendar with native date navigation, optional range selection and locale support.
class Calendar : public QCalendarWidget {
    Q_OBJECT
  public:
    explicit Calendar(QWidget* parent = nullptr);
    void setRangeSelection(bool enabled);
    [[nodiscard]] bool rangeSelection() const { return rangeSelection_; }
    void setSelectedRange(QDate first, QDate last);
    [[nodiscard]] QDate rangeStart() const { return rangeStart_; }
    [[nodiscard]] QDate rangeEnd() const { return rangeEnd_; }
  signals:
    void rangeChanged(QDate first, QDate last);

  protected:
    void paintCell(QPainter*, const QRect&, QDate) const override;
    void changeEvent(QEvent*) override;

  private:
    void choose(QDate date);
    void refreshTheme();
    bool rangeSelection_ = false;
    bool selectingEnd_ = false;
    QDate rangeStart_;
    QDate rangeEnd_;
};

/// A calendar popup triggered by a button. An invalid QDate clears the selection.
class DatePicker : public Button {
    Q_OBJECT
  public:
    explicit DatePicker(QWidget* parent = nullptr);
    [[nodiscard]] QDate date() const { return date_; }
    void setDate(QDate date);
    [[nodiscard]] Calendar& calendar() { return *calendar_; }
  signals:
    void dateChanged(QDate date);

  private:
    QDate date_;
    Calendar* calendar_;
    Popover* popup_;
};

/// Label, description, control and validation feedback in one layout.
class Field : public QWidget {
    Q_OBJECT
  public:
    explicit Field(const QString& label = {}, QWidget* parent = nullptr);
    /// Reparents the control to this field. Replacing it detaches the old control without deleting it.
    void setControl(QWidget* control);
    [[nodiscard]] QWidget* control() const { return control_.data(); }
    void setLabel(const QString& label);
    void setDescription(const QString& description);
    void setError(const QString& error);
    [[nodiscard]] QString error() const;
    void setValidator(std::function<QString()> validator);
    bool validate();

  protected:
    void changeEvent(QEvent*) override;

  private:
    void refreshTheme();
    QVBoxLayout* layout_;
    Label* label_;
    QLabel* description_;
    QLabel* error_;
    QPointer<QWidget> control_;
    std::function<QString()> validator_;
};

/// Local form validation. Application code handles storage and submission.
class Form : public QWidget {
    Q_OBJECT
  public:
    explicit Form(QWidget* parent = nullptr);
    Field& addField(const QString& label, QWidget* control);
    [[nodiscard]] QVBoxLayout& content() { return *layout_; }
    bool validate();
  public slots:
    void submit();
  signals:
    void submitted();

  private:
    QVBoxLayout* layout_;
    QList<QPointer<Field>> fields_;
};

/// Segmented code input with paste, arrows, deletion and completion signals.
class InputOTP : public QLineEdit {
    Q_OBJECT
  public:
    explicit InputOTP(int length = 6, QWidget* parent = nullptr);
    [[nodiscard]] QString code() const;
    bool setCode(const QString& code);
    void setAlphanumeric(bool enabled);
    [[nodiscard]] int length() const { return length_; }
    QSize sizeHint() const override;
  signals:
    void codeChanged(const QString& code);
    void completed(const QString& code);

  protected:
    bool event(QEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

  private:
    bool allowed(QChar character) const;
    int length_;
    int selectionAnchor_ = 0;
    bool alphanumeric_ = false;
    bool caret_ = true;
    QTimer blink_;
};

/// Model based table with themed headers, row selection and horizontal separators.
class Table : public QTableView {
    Q_OBJECT
  public:
    explicit Table(QWidget* parent = nullptr);

  protected:
    void changeEvent(QEvent*) override;

  private:
    void refreshTheme();
};

/// Sortable and filterable table backed by a Qt proxy model.
class DataTable : public Table {
    Q_OBJECT
  public:
    explicit DataTable(QWidget* parent = nullptr);
    void setSourceModel(QAbstractItemModel* model);
    void setFilter(const QString& text, int column = -1);
    [[nodiscard]] QSortFilterProxyModel& proxy() { return *proxy_; }

  private:
    QSortFilterProxyModel* proxy_;
};

enum class ChartType { Line, Area, Bar, Pie };
struct ChartSeries {
    QString name;
    QList<double> values;
    Role colour = Role::Chart1;
};
/// Native chart with themed axes, legend, pointer values and reduced motion support.
class Chart : public QWidget {
    Q_OBJECT
  public:
    explicit Chart(QWidget* parent = nullptr);
    void setChartType(ChartType type);
    std::expected<void, ValueError> setSeries(QList<ChartSeries> series);
    void setLabels(QStringList labels);
    void setLegendVisible(bool visible);
    QSize sizeHint() const override;

  protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    bool event(QEvent*) override;

  private:
    void animate();
    QRectF plotRect() const;
    ChartType type_ = ChartType::Bar;
    QList<ChartSeries> series_;
    QStringList labels_;
    bool legend_ = true;
    double amount_ = 1;
    int hover_ = -1;
    QVariantAnimation* animation_;
};

} // namespace shadcn
