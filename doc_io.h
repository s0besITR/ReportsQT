#pragma once

#include "xml_io.h"
#include <xlnt/xlnt.hpp>
#include <QString>
#include <QMap>

class xlnt_style
{
 public:
    xlnt_style();
    xlnt_style(const Template::style& xml_style);

    xlnt::alignment align;										// Выравнивание текста (горизонтальное и вертикальное)
    xlnt::border border;										// Границы
    xlnt::font font;											// Шрифт (размер, имя, цвет)
    xlnt::color cellColorBkg;									// Цвет ячейки
};


class doc_io
{
public:
    doc_io(const QString & name, const QString& path, const QString& time_stamp, const QMap<QString, Template::style> &xml_styles);

    void drawRow(const Template::row & row);
    QString save();

private:
    QString name;
    QString path;
    QString time_stamp;
    unsigned int position;
    QMap<QString, xlnt_style> styles;
    xlnt::workbook wb;

    void apply_style_to_cell(xlnt::cell & cell, const xlnt_style& style);
    void apply_style_to_range(xlnt::range & range, const xlnt_style& style);
};
