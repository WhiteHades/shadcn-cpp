#pragma once
#include <QStringList>
#include <QPixmap>
class QWidget;
namespace gallery {
QStringList components();
QWidget* demo(const QString& component, QWidget* parent = nullptr);
QPixmap capture(QWidget& canvas);
} // namespace gallery
