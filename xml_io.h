#pragma once

#include <QtWidgets>
#include <QVector>
#include <QString>
#include <QMap>
#include <QHostInfo>
#include <QCryptographicHash>
#include <QMessageBox>
#include <xlnt/xlnt.hpp>
#include "pugixml/pugixml.hpp"
#include <QException>

extern const QString TEMPLATE_DIR;
extern const QString XML_STATION_NAME;

class ParseException : public QException{
public:
    ParseException(const QString& msg) : msg(msg) {}
    ParseException(const ParseException& ex) {msg = ex.msg;}
    ~ParseException() {}

    void raise() const override {throw *this;}
    ParseException *clone() const override {return new ParseException(*this);}
    virtual QString what() {return msg;}
private:
    QString msg;
    using QException::what;
};



/*      ОПИСАНИЕ СТРУКТУРЫ STATION.XML        */
namespace Station{

    struct object_fields {
        QString name;                       // Имя
        QString location;                   // Расположение
        QString common_type;                // Общий тип
        object_fields() = default;
        object_fields(QString name, QString location, QString common_type)
            : name(name), location(location), common_type(common_type)
        {};
    };

    struct DB_config {

        QVector<QString> ip;                // Массив ip (атрибуты host_ip1, host_ip2 ...).
        QString port;						// Порт для подключения к БД
        QString dbname;						// Имя БД
        QString user;						// Имя пользователя
        QString password;					// Пароль
        QString uniq_name;                  // Уникальное имя приложения
        QString utc_offset;					// БД отдает время в формате UTC, этой строкой мы можем сместить его, задав например + time '07:00'
        bool clear_old_connects;			// Перед подключением снимаем все предыдущие соединения с таким же именем
        bool send_alarm;					// Надо ли слать аларм в скаду по завершении
        bool checkTagExistance;				// Если да, то перед основным SQL запросом будет делаться запрос "SELECT * FROM nodes Where nodes.TagName ='tag'" для проверки существования сигнала в базе
        bool checkServerState;              // Если да, то подключаемся только к серверу, у которого ServerStateTag находится в True (основной сервер)
        QString ServerStateTag;             // Переопределение тега состояния сервера bool (по умолчанию Service.State.Server)

        // Конструктор по умолчанию
        DB_config()
            : ip({"127.0.0.1"}), port("5432"), dbname("postgres"), user("postgres"), password("postgres"),
              uniq_name("ITR Reports"), utc_offset(""), clear_old_connects(false), send_alarm(false), checkTagExistance(false),
              checkServerState(false), ServerStateTag("Service.State.Server")
        {};

        //Конструктор с параметрами
        DB_config(QVector<QString> ip, QString port, QString dbname, QString user, QString pwd, QString utc_offset, bool clear_old_connects, bool send_alarm, bool check_ex, bool check_state, QString stateTag)
            :ip(ip), port(port), dbname(dbname), user(user), password(pwd),
              uniq_name("ITR_Reports"), utc_offset(utc_offset), clear_old_connects(clear_old_connects), send_alarm(send_alarm), checkTagExistance(check_ex),
              checkServerState(check_state), ServerStateTag(stateTag)
        {};

        //Строка для соединения с БД
        QString get_connectionString(const QString & host, const QString &template_name);
        QString getAppName(const QString& template_name);
    };

    struct xml_station {
        QString name;                           // Имя станции
        QString tag_prefix;                     // Префикс для тега
        DB_config sql_settings;                 // Настройки sql, прочитанные из узла <settings>
        QMap<QString, object_fields> objects;   // Карта с информацией по тегу, где ключ это название тега
    };

}

/*      ОПИСАНИЕ СТРУКТУРЫ ШАБЛОНА        */
namespace Template{

    // Стиль. Стили перечислены в шаблоне внутри узла <styles>
    struct style{
        QString textBold;
        QString textSize;
        QString textFontName;
        QString textColor;
        QString textAlign_h;
        QString textAlign_w;
        QString cellColor;
        QString cellBorder;

        style(QString bold, QString t_size, QString t_font, QString t_color, QString t_align_h, QString t_align_w, QString c_color, QString c_border)
            : textBold(bold), textSize(t_size), textFontName(t_font), textColor(t_color), textAlign_h(t_align_h), textAlign_w(t_align_w), cellColor(c_color), cellBorder(c_border)
        {};
    };

    //Структура, хранящая размер ячейки, если он был задан
    struct custom_size
    {
        double width;												// Ширина
        double height;												// Высота
        bool use_custom_width;										// Флаги использования, что бы можно было задавать нулевые значения
        bool use_custom_height;

        custom_size()
            : width(0), height(0), use_custom_width(false), use_custom_height(false)
        {};

        void set_custom_width(double w)     {   width = w;  use_custom_width = true ;} ;		// Задать ширину
        void set_custom_height(double h)    {   height = h; use_custom_height = true;} ;		// Задать высоту
    };

    // Структура, описывающая одну ячейку. В шаблоне ячейка представляется узлом <cell> с атрибутами value, style, mergeCells, height, weight
    struct cell {
        QString value;							// Значение
        QString style;							// Имя стиля
        uint16_t mergeCells;					// Количество объединяемых ячеек по горизонтали
        custom_size size;                       // Размер
    };

    // Тег внутри типа
    struct tag
    {
        QString name;                               // Название (Например Авария)
        QString value;                              // Значение - это тег относительно объекта, а не всей станции, до нужного сигнала (Например .Alarm)
        tag(QString name, QString value)
            : name(name), value(value)
        {};
    };

    // Строка это вектор ячеек (слева направо)
    using row = QVector<cell>;

    // Структура описывает один тип
    struct type {
        /*
            Single_mode - Режим обработки SQL запроса. SQL запрос sql_querry возвращает массив строк и столбцов.
            Single_mode == true  - Независимо от кол-ва строк, которые вернулись по sql запросу, будет выведен на печать один row
            Single_node == false -  Для каждой строки запроса будет напечатан свой row
        */
        bool single_mode;
        QString sql_querry;
        QVector<tag> tags;				// Массив тегов у данного типа объекта. Внутри узла <tags>
        row one_row;                    // Узел <row>, содержащий строку, которая будет выведена на печать по данному объекту
    };

    // Узел report_dat содержит тег объекта и его тип
    struct report_data{
       QString tag;
       QString type;
       bool includeInReport = true;                                 // Включать ли элемент в отчет
    };

    // Узел structure описывает структуру отчета и состоит из строк шапки и строк данных
    struct structure {
        QVector<row> head_rows;
        QVector<report_data> data;
    };

    // Объект одного отчета
    struct xml_template{
        QString name;							// Шаблон имеет имя, которое будет использовано при сохранении шаблона на диск
        QString save_path;						// Путь для сохранения шаблона. Если не указан, то используется расположение рядом с exe файлом
        bool includeNullResult;					// Печатать ли <row> в отчет по объекту, если его SQL запрос не вернул данных.
        bool oper_mode;                         // Режим оперативный, иначе исторический
        QMap<QString, style> styles;	// Используемые стили, прочитанные из узла <styles> шаблона
        QMap<QString, type> types;			// Используемые типы, прочитанные из узла <types> шаблона
        structure report_structure;		// Информация о структуре шаблона из узла <structure>
    };

    void getRow(const pugi::xml_node & row_node, row & cells);
}


namespace XmlParse{

    // Открыть xml документ filename и загрузить его в doc
    bool openFileXml(pugi::xml_document& doc, const QString& filename);
    Station::xml_station readStationCfg();
    Template::xml_template readTemplateCfg(const QString & name);
    void writeStationCfg(Station::DB_config cfg);
}
