// SPDX-License-Identifier: MIT
#pragma once

#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>

namespace shadcn::detail {

/// Monochrome vector glyphs for the controls that own their icon.
enum class Glyph { Close, ChevronLeft, ChevronRight, PanelLeft };

/// Paints with the caller's pen colour, so an icon follows its button's text colour.
class GlyphIcon final : public QIconEngine {
  public:
    explicit GlyphIcon(Glyph glyph) : glyph_(glyph) {}
    QIconEngine* clone() const override { return new GlyphIcon(glyph_); }
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap image(size);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setPen(QApplication::palette().color(QPalette::ButtonText));
        paint(&painter, QRect(QPoint{}, size), mode, state);
        return image;
    }
    void paint(QPainter* painter, const QRect& rect, QIcon::Mode, QIcon::State) override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(rect.topLeft());
        painter->scale(rect.width() / 24., rect.height() / 24.);
        const auto colour = painter->pen().color();
        painter->setPen(QPen(colour, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        QPainterPath path;
        switch (glyph_) {
        case Glyph::Close:
            path.moveTo(6, 6);
            path.lineTo(18, 18);
            path.moveTo(18, 6);
            path.lineTo(6, 18);
            break;
        case Glyph::ChevronLeft:
            path.moveTo(15, 18);
            path.lineTo(9, 12);
            path.lineTo(15, 6);
            break;
        case Glyph::ChevronRight:
            path.moveTo(9, 18);
            path.lineTo(15, 12);
            path.lineTo(9, 6);
            break;
        case Glyph::PanelLeft:
            path.addRoundedRect(QRectF(3, 3, 18, 18), 2, 2);
            path.moveTo(9, 3);
            path.lineTo(9, 21);
            break;
        }
        painter->drawPath(path);
        painter->restore();
    }

  private:
    Glyph glyph_;
};

[[nodiscard]] inline QIcon glyph(Glyph which) { return QIcon(new GlyphIcon(which)); }

} // namespace shadcn::detail
