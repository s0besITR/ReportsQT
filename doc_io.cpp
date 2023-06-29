#include "doc_io.h"
#include "ui/ui_mainWindow.h"

extern ui_mainWindow* pMainW;

QMap<QString, xlnt::horizontal_alignment> horizontal_alignment_map{
    {"general", xlnt::horizontal_alignment::general},
    {"left", xlnt::horizontal_alignment::left},
    {"center", xlnt::horizontal_alignment::center},
    {"right", xlnt::horizontal_alignment::right},
    {"fill", xlnt::horizontal_alignment::fill},
    {"justify", xlnt::horizontal_alignment::justify},
    {"center_continuous", xlnt::horizontal_alignment::center_continuous},
    {"distributed", xlnt::horizontal_alignment::distributed},
};

QMap<QString, xlnt::vertical_alignment> vertical_alignment_map{
    {"top", xlnt::vertical_alignment::top},
    {"center", xlnt::vertical_alignment::center},
    {"bottom", xlnt::vertical_alignment::bottom},
    {"justify", xlnt::vertical_alignment::justify},
    {"distributed", xlnt::vertical_alignment::distributed}
};

QMap<QString, xlnt::border_style> border_style_map{
    {"none", xlnt::border_style::none},
    {"dashdot", xlnt::border_style::dashdot},
    {"dashdotdot", xlnt::border_style::dashdotdot},
    {"dashed", xlnt::border_style::dashed},
    {"dotted", xlnt::border_style::dotted},
    {"double_", xlnt::border_style::double_},
    {"hair", xlnt::border_style::hair},
    {"medium", xlnt::border_style::medium},
    {"mediumdashdot", xlnt::border_style::mediumdashdot},
    {"mediumdashdotdot", xlnt::border_style::mediumdashdotdot},
    {"mediumdashed", xlnt::border_style::mediumdashed},
    {"slantdashdot", xlnt::border_style::slantdashdot},
    {"thick", xlnt::border_style::thick},
    {"thin", xlnt::border_style::thin}
};

QMap<QString, xlnt::color> color_names_map{
    {"@black", xlnt::color::black()},
    {"@white", xlnt::color::white()},
    {"@red", xlnt::color::red()},
    {"@darkred", xlnt::color::darkred()},
    {"@blue", xlnt::color::blue()},
    {"@darkblue", xlnt::color::darkblue()},
    {"@green", xlnt::color::green()},
    {"@darkgreen", xlnt::color::darkgreen()},
    {"@yellow", xlnt::color::yellow()},
    {"@darkyellow", xlnt::color::darkyellow()}
};

xlnt_style::xlnt_style()
{
    align.horizontal(xlnt::horizontal_alignment::left);
    align.vertical(xlnt::vertical_alignment::bottom);

    //Линии границ
    for (xlnt::border_side side : border.all_sides())
        border.side(side, xlnt::border::border_property().style(xlnt::border_style::none));
    //Шрифт
    font.bold(false);
    font.color(xlnt::color::black());
    font.size(12);
    font.name("Arial");
    //Заливка
    cellColorBkg = xlnt::color(xlnt::color::white());
}

xlnt_style::xlnt_style(const Template::style& xml_style)
{
    //Выравнивание
    align.horizontal(horizontal_alignment_map.value(xml_style.textAlign_h, xlnt::horizontal_alignment::left));
    align.vertical(vertical_alignment_map.value(xml_style.textAlign_w, xlnt::vertical_alignment::bottom));

    //Линии границ
    for (xlnt::border_side side : border.all_sides())
        border.side(side, xlnt::border::border_property().style(border_style_map.value(xml_style.cellBorder, xlnt::border_style::none)));

    //Шрифт
    font.bold(xml_style.textBold.compare("true", Qt::CaseInsensitive)==0 ? true : false);
    if(xml_style.textColor.startsWith("@"))
        font.color(color_names_map.value(xml_style.textColor, xlnt::color::black()));
    else
        font.color(xlnt::rgb_color(xml_style.textColor.toStdString()));
    font.size(xml_style.textSize.toUInt());
    font.name(xml_style.textFontName.toStdString());

    //Заливка
    if(xml_style.cellColor.startsWith("@"))
        cellColorBkg = xlnt::color(color_names_map.value(xml_style.cellColor, xlnt::color::white()));
    else
        cellColorBkg = xlnt::color(xlnt::rgb_color(xml_style.cellColor.toStdString()));
}

doc_io::doc_io(const QString & name, const QString& path, const QString& time_stamp, const QMap<QString, Template::style> &xml_styles)
    : name(name), path(path), time_stamp(time_stamp), position(0)
{
    for(auto it = xml_styles.begin(); it != xml_styles.end(); ++it)
        styles.insert(it.key(),xlnt_style(it.value()));

    wb.active_sheet().title(u8"Лист1");
}

void doc_io::apply_style_to_cell(xlnt::cell & cell, const xlnt_style& style)
{
    cell.alignment(style.align);
    cell.border(style.border);
    cell.font(style.font);
    cell.fill(xlnt::fill::solid(style.cellColorBkg));

}
void doc_io::apply_style_to_range(xlnt::range & range, const xlnt_style& style)
{
    range.alignment(style.align);
    range.border(style.border);
    range.font(style.font);
    range.fill(xlnt::fill::solid(style.cellColorBkg));
}

QString doc_io::save()
{
    //Подпись
    xlnt::cell_reference signn_ref = wb.active_sheet().cell("A1").reference().make_offset(0, position + 1);
    xlnt::cell sign_cell = wb.active_sheet().cell(signn_ref);
    sign_cell.value(u8"ООО \"ИННОВАЦИОННЫЕ ТЕХНОЛОГИИ И РЕШЕНИЯ\", 2023");

    QString file_name = name + " (" + time_stamp + ")" + ".xlsx";

#ifndef unix
    file_name.replace(":", "_");
#endif

    QString absolute_path = path + file_name;
    QString return_msg;
    try {      
        wb.save(absolute_path.toLocal8Bit().toStdString());
        return_msg = QString("Отчет сохранен: %1").arg(absolute_path);
        pMainW->log_write(return_msg, Qt::darkGreen);
    }
    catch (const std::exception& e)
    {
        pMainW->log_write(QString("Ошибка. Не удалось сохранить отчет. Существует ли путь: %1 ?").arg(path), Qt::red);
        try
        {           
            wb.save(file_name.toLocal8Bit().toStdString());
            return_msg = QString("Отчет сохранен рядом с исполняемым exe: %1").arg(file_name);
            pMainW->log_write(return_msg, Qt::darkGreen);
        }
        catch (const std::exception& e)
        {
            return_msg = QString("Ошибка. Не удалось сохранить отчет %1. Причина: %2").arg(file_name, e.what());
            pMainW->log_write(return_msg, Qt::red);
        }
    }
    return return_msg;
}

void doc_io::drawRow(const Template::row & row)
{
    xlnt::worksheet ws = wb.active_sheet();
    xlnt::cell_reference start_cell = ws.cell("A1").reference().make_offset(0, position);
    unsigned int col_offset = 0;

    for(Template::cell xml_cell : row)
    {
        // Объект cel_ref является ссылкой на ячейку, в которую на этой итерации попадут данные
        xlnt::cell_reference cel_ref = start_cell.make_offset(col_offset, 0);       // с помощью make_offset(horiz, vert) двигаемся слева направо
        xlnt::cell cell = ws.cell(cel_ref);                                         // конкретная ячейка по нашей ссылке

        // Значение ячейки
        if(QString(xml_cell.value.toStdString().c_str()).startsWith('='))
            cell.formula(xml_cell.value.toStdString());
        else
            cell.value(xml_cell.value.toStdString());


        //Размеры
        if (xml_cell.size.use_custom_height)
        {
            ws.row_properties(cell.row()).custom_height = true;
            ws.row_properties(cell.row()).height = xml_cell.size.height;
        }
        if (xml_cell.size.use_custom_width)
        {
            ws.column_properties(cell.column()).custom_width = true;
            ws.column_properties(cell.column()).width = xml_cell.size.width;
        }

        // В зависимости от того, надо ли объединять ячейки между собой
        if (xml_cell.mergeCells > 0)
        {
            xlnt::range_reference range_ref = xlnt::range_reference(cel_ref, cel_ref.make_offset((int)xml_cell.mergeCells - 1, 0));
            xlnt::range range = ws.range(range_ref);

            // Объединяем диапазон ячеек
            ws.merge_cells(range_ref);
            //Применяем стиль к диапазону объединенных ячеек
            apply_style_to_range(range, styles.value(xml_cell.style, xlnt_style()));
            //Сдвигаемся вправо
            col_offset += (xml_cell.mergeCells - 1);
        }
        else
        {
            apply_style_to_cell(cell,styles.value(xml_cell.style, xlnt_style()));
            col_offset++;
        }
    }
    position++;
}




