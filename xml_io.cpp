#include "xml_io.h"
using namespace pugi;

//Строка для соединения с БД
QString Station::DB_config::get_connectionString(const QString & host, const QString &template_name)
{
    return QString("host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user + " password=" + password + " connect_timeout=8" + " application_name=" + getAppName(template_name));
}

// Уникальное имя приложения зависит от uniq_name, template_name и имени хоста
QString Station::DB_config::getAppName(const QString & template_name)
{
    QString host_name = QHostInfo::localHostName();
    host_name.replace(' ','_');
    QString template_hash = QCryptographicHash::hash(template_name.toUtf8(),QCryptographicHash::Md5).toHex();
    return uniq_name + "_" + host_name + "_hash=" + template_hash;
}

// Открыть xml файл и загрузить его в doc
bool XmlParse::openFileXml(xml_document& doc, const QString& filename)
{
    xml_parse_result result = doc.load_file(filename.toStdWString().data(), parse_default, encoding_utf8);
    if (result.status != status_ok)
    {
        QString msg = QString("Возникла ошибка при загрузке %1:\n%2").arg(filename, result.description());
        throw ParseException(msg);
    }
    return result.status == status_ok;
}

// Возвращает прочитанную конфигурацию станции из файла station.xml
Station::xml_station XmlParse::readStationCfg()
{
    xml_document            station_doc;
    Station::xml_station    station_obj;

    XmlParse::openFileXml(station_doc, QCoreApplication::applicationDirPath() + TEMPLATE_DIR + "station.xml");

    xml_node station_node =             station_doc.child("station");
    xml_node station_settings_node =    station_doc.find_node([](pugi::xml_node& node) {return strcmp(node.name(), "settings") == 0; });
    xml_node station_objects_node =     station_doc.find_node([](pugi::xml_node& node) {return strcmp(node.name(), "objects") == 0; });

    if (!station_node || !station_settings_node || !station_objects_node)
    {
       QString msg = QString("Ошибка при чтении %1\nНе найден один из следующих узлов : <station/>, <settings/>, <objects/>").arg(XML_STATION_NAME);
       throw ParseException(msg);
    }

    //Заполнили ip адреса
    QVector<QString> ip_list;
    for (size_t i = 1; i <= 10; ++i)
    {
        QString ip = station_settings_node.attribute(QString("host_ip%1").arg(i).toStdString().data()).as_string("-1");
        if (ip != "-1")
            ip_list.push_back(ip);
    }
    // Инициализировали объект станции атрибутами
    station_obj.sql_settings = Station::DB_config(
            ip_list,
            station_settings_node.attribute("port").as_string(),
            station_settings_node.attribute("dbname").as_string(),
            station_settings_node.attribute("user").as_string(),
            station_settings_node.attribute("password").as_string(),
            station_settings_node.attribute("utc_time_offset").as_string(),
            station_settings_node.attribute("clear_old_connections").as_bool(false),
            station_settings_node.attribute("send_alarm_to_scada").as_bool(false),
            station_settings_node.attribute("checkTagExistance").as_bool(false),
            station_settings_node.attribute("checkServerState").as_bool(false),
            station_settings_node.attribute("ServerStateTag").as_string("Service.State.Server")

    );
    // Объекты на станции
    station_obj.name = station_node.attribute("name").as_string();
    station_obj.tag_prefix = station_node.attribute("tagPrefix").as_string();
    for (pugi::xml_node obj : station_objects_node.children())
        station_obj.objects.insert(obj.attribute("tag").as_string(),
                                   Station::object_fields(obj.attribute("name").as_string(), obj.attribute("location").as_string(), obj.attribute("common_type").as_string())
                                   );

    return station_obj;
}

// Заполнить row ячейками cells из узла row_node
void Template::getRow(const xml_node & row_node, row & cells)
{
    for(xml_node cell : row_node.children())
    {
        Template::cell tmp_cell;
        tmp_cell.value = cell.attribute("value").as_string();
        tmp_cell.style = cell.attribute("style").as_string();
        tmp_cell.mergeCells = cell.attribute("mergeCells").as_uint();
        double w = cell.attribute("width").as_double();
        double h = cell.attribute("height").as_double();
        if (w > 0) tmp_cell.size.set_custom_width(w);
        if (h > 0) tmp_cell.size.set_custom_height(h);
        cells.push_back(tmp_cell);
    }
}

Template::xml_template XmlParse::readTemplateCfg(const QString & name)
{
    xml_document            template_doc;
    Template::xml_template  template_obj;

    XmlParse::openFileXml(template_doc, QCoreApplication::applicationDirPath() + TEMPLATE_DIR + name);

    xml_node styles_node	= template_doc.find_node([](xml_node& node) {return strcmp(node.name(), "styles") == 0; });
    xml_node types_node		= template_doc.find_node([](xml_node& node) {return strcmp(node.name(), "types") == 0; });
    xml_node structure_node	= template_doc.find_node([](xml_node& node) {return strcmp(node.name(), "structure") == 0; });

    if (!styles_node || !types_node || !structure_node)
    {
        QString msg = QString("Ошибка при чтении %1\nНе найден один из следующих узлов в шаблоне: <styles/>, <types/>, <structure/>").arg(name);
        throw ParseException(msg);
    }

    // Считываем параметры отчета
    template_obj.name               = template_doc.child("template").attribute("name").as_string("unknown_tamplate");
    template_obj.save_path          = template_doc.child("template").attribute("save_path").as_string("");
    template_obj.includeNullResult  = template_doc.child("template").attribute("includeNullResult").as_bool(true);
    template_obj.oper_mode          = QString(template_doc.child("template").attribute("mode").as_string("oper")).compare("oper", Qt::CaseInsensitive) == 0;

    template_obj.save_path.replace("\\","/");

    // Заполняем стили
    for (xml_node style : styles_node.children())
    {
        template_obj.styles.insert(style.name(), Template::style(
            style.attribute("textBold").as_string(),
            style.attribute("textSize").as_string(),
            style.attribute("textFontName").as_string(),
            style.attribute("textColor").as_string(),
            style.attribute("textAlign_h").as_string(),
            style.attribute("textAlign_w").as_string(),
            style.attribute("cellColor").as_string(),
            style.attribute("cellBorder").as_string()
        ));
    }

    // Типы
    for (xml_node type : types_node.children())
    {
        Template::type tmp_type;
        // Атрибуты
        tmp_type.sql_querry = type.attribute("sql_querry").as_string();
        tmp_type.single_mode = type.attribute("single_mode").as_bool(true);

        // Теги
        for (xml_node tag : type.child("tags").children())
            tmp_type.tags.push_back(Template::tag(tag.attribute("name").as_string(), tag.attribute("value").as_string()));

        // Строка ячеек
        getRow(type.child("row"), tmp_type.one_row);
        template_obj.types.insert(type.name(), tmp_type);
    }

    // Структура отчета
    for (xml_node elem : structure_node.children())
    {
        if (strcmp(elem.name(), "row") == 0)
        {
            Template::row tmp_row;
            getRow(elem, tmp_row);
            template_obj.report_structure.head_rows.push_back(tmp_row);
        }
        if (strcmp(elem.name(), "report_data") == 0)
        {
            for (xml_node data : elem.children())
            {
                Template::report_data tmp_data;
                tmp_data.tag = data.attribute("tag").as_string();
                tmp_data.type = data.attribute("type").as_string();
                template_obj.report_structure.data.push_back(tmp_data);
            }
        }
    }
    return template_obj;
}

//Записать измененную конфигурацию
void XmlParse::writeStationCfg(Station::DB_config cfg)
{
    xml_document            station_doc;
    Station::xml_station    station_obj;
    QString station_path = QCoreApplication::applicationDirPath() + TEMPLATE_DIR + "station.xml";

    try{
        XmlParse::openFileXml(station_doc, station_path);
    }
    catch(ParseException&  exc)
    {
        QMessageBox::critical(qApp->activeWindow(),"Ошибка открытия файла station.xml",exc.what(), QMessageBox::Ok);
        return;
    }

    xml_node station_settings_node =    station_doc.find_node([](pugi::xml_node& node) {return strcmp(node.name(), "settings") == 0; });

    if (!station_settings_node)
    {
        QMessageBox::critical(qApp->activeWindow(),"Ошибка сохранения файла station.xml","Не найден узел <settings>", QMessageBox::Ok);
        return;
    }

    for(int i = 0; i < cfg.ip.count(); ++i)
    {
        station_settings_node.attribute(QString("host_ip%1").arg(i+1).toStdString().data()).set_value(cfg.ip.at(i).toStdString().data());
    }

    station_settings_node.attribute("port").set_value(cfg.port.toStdString().data());
    station_settings_node.attribute("dbname").set_value(cfg.dbname.toStdString().data());
    station_settings_node.attribute("user").set_value(cfg.user.toStdString().data());
    station_settings_node.attribute("password").set_value(cfg.password.toStdString().data());
    station_settings_node.attribute("utc_time_offset").set_value(cfg.utc_offset.toStdString().data());

    station_settings_node.attribute("clear_old_connections").set_value(cfg.clear_old_connects);
    station_settings_node.attribute("send_alarm_to_scada").set_value(cfg.send_alarm);
    station_settings_node.attribute("checkTagExistance").set_value(cfg.checkTagExistance);
    station_settings_node.attribute("checkServerState").set_value(cfg.checkServerState);
    station_settings_node.attribute("ServerStateTag").set_value(cfg.ServerStateTag.toStdString().data());

    station_doc.save_file(station_path.toLocal8Bit().toStdString().data(), "\t", pugi::format_indent | format_indent_attributes, encoding_utf8);
}
