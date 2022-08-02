#include <QtWidgets>
#include <QDebug>
#include <QTimer>

#include "ui/ui_mainWindow.h"
#include <xml_io.h>

using namespace pugi;

const QString TEMPLATE_DIR = "/templates/";

struct cmd_arguments;
const QString XML_STATION_NAME = "station.xml";
Station::xml_station global_station;
QMap<QString,Template::xml_template> global_templates;
ui_mainWindow* pMainW = nullptr;

cmd_arguments argumentsParserInit(QApplication & app)
{
    QCommandLineParser parser;
    cmd_arguments args;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    QString help_descr = "Программу отчетов можно запускать с параметрами, например:\n"
                         "Для настройки генерации отчетов по расписанию (что бы программа отчетов автономно запускалась планировщиком)\n\n"
                         "Примеры запуска:\n"
                         "-template \"Шаблон аварий (исторический).xml\" -date \"last10\" -close 1 -min\n"
                         "\tОткроет программу отчеты в свернутом режиме (-min)\n"
                         "\tЗагрузит для работы шаблон \"Шаблон аварий (исторический).xml\" (-template)\n"
                         "\tВыставит период времени по отчету – последние 10 дней (-date \"last10\")\n"
                         "\tИ закроет программу через 1 секунду после сохранения отчета (-close 1)\n"
                         "\n"
                         "-template \"Шаблон аварий (исторический).xml\" -tag \"ORK.Masterskie.SHUP_1\" -date \"2022-03-10 11:50|2022-03-15 13:00\" -close 20\n"
                         "\tЗапустит программу в обычном несвернутом режиме\n"
                         "\tЗагрузит для работы шаблон \"Шаблон аварий (исторический).xml\" (-template)\n"
                         "\tНо отчет будет формироваться только по одному тегу \"ORK.Masterskie.SHUP_1\" (-tag)\n"
                         "\tПрограмма будет автоматически закрыта через 20 секунд (-close)\n"
                         "\tВ качестве временных интервалов (-date) используются конкретные даты и время \"start|end\" = \"2022-03-10 11:50|2022-03-15 13:00\"\n"
                         "\tЗначит дата начала периода \"2022-03-10 11:50\", а дата конца \"2022-03-15 13:00\"";


    parser.setApplicationDescription(help_descr);
    parser.addHelpOption();

    QCommandLineOption opt_minimize ("min",            "Открытие окна в свернутом состоянии");
    QCommandLineOption opt_not_fold ("no_fold",        "Убрать кнопку свертывания у окна");
    QCommandLineOption opt_modal    ("modal",          "Окно открывается поверх других окон");
    QCommandLineOption opt_close    ("close",          "Указать временной интервал <sec>, после которого программа автоматически закроется", "sec");
    QCommandLineOption opt_monitor  ("screen",         "Указать номер монитора <mon> (по умолчанию открытие на 1-ом мониторе)", "mon");
    QCommandLineOption opt_template ("template",       "Указать имя шаблона <name>", "name");
    QCommandLineOption opt_interval ("date",           "Указать временной интервал для исторических отчетов <start|end> в формате <yyyy-MM-dd hh:mm:ss|yyyy-MM-dd hh:mm:ss>. Или <lastX>, где X - число дней", "start|end");
    QCommandLineOption opt_tag      ("tag",            "Указать тег объекта <tag>, по которому будет сгенерирован отчет", "tag");

    parser.addOptions({opt_minimize, opt_not_fold, opt_modal, opt_close, opt_monitor, opt_template, opt_interval, opt_tag});
    parser.process(app);

    if(parser.isSet(opt_minimize))  args.minimize   = true;
    if(parser.isSet(opt_not_fold))  args.foldable   = false;
    if(parser.isSet(opt_modal))     args.modal      = true;

    if(parser.isSet(opt_close))     args.close_after    = parser.value(opt_close).toInt();
    if(parser.isSet(opt_monitor))   args.monitor        = parser.value(opt_monitor).toInt();;
    if(parser.isSet(opt_template))  args.template_name  = parser.value(opt_template);
    if(parser.isSet(opt_interval))  args.time           = parser.value(opt_interval);
    if(parser.isSet(opt_tag))       args.tag            = parser.value(opt_tag);

    return args;
}

int main(int argc, char** argv)
{
    QApplication     app(argc, argv);
    app.setWindowIcon(QIcon(":/icon.png"));
    int counter = 0;
    int close_after = -1;

    //Чтение конфигурации
    if(!ui_mainWindow::load_configuration())
        return 1;

    // Создание главного окна
    pMainW = new ui_mainWindow(argumentsParserInit(app));
    pMainW->show();

    //Функция закрытия окна по таймеру
    close_after = pMainW->cmd_args.close_after;
    auto timer_func = [&counter, &close_after, &app](){
        ++counter;
        pMainW->statusBar()->showMessage(QString("Закрытие программы через %1 сек").arg(close_after - counter));
        if(counter >= close_after)
            app.quit();
    };
    QTimer *timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, timer_func);

    //Делаем отчет, если был автоматический запуск
    if(pMainW->cmd_args.template_name != "")
    {
        pMainW->makeReport();
        pMainW->cmd_args.template_name = "";
        //закрываем приложение через close_after секунд
        if(close_after >= 0)         
             timer->start(1000);
    }

    return app.exec();
}
