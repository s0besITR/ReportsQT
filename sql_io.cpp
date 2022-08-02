#include "sql_io.h"
#include "ui/ui_mainWindow.h"
#include <chrono>
#include <future>

extern ui_mainWindow* pMainW;
extern Station::xml_station global_station;

sql_io::~sql_io()
{
    disconnect();
}

bool sql_io::checkConnection(QString connection_string, QString & err_msg)
{
    try
    {
        pqxx::connection(connection_string.toStdString().data());
        err_msg = "ok";
        return true;
    }
    catch(const std::exception &e)
    {
        err_msg = e.what();
        return false;
    }
}

bool sql_io::serverAllowed(bool print)
{
    if(!global_station.sql_settings.checkServerState)
        return true;

    if(print)
        pMainW->log_write("Проверяем статус сервера...");

    pqxx::result response = execute(QString("SELECT valbool FROM nodes_values JOIN nodes USING(NodeId) Where nodes.TagName ='%1'").arg(global_station.sql_settings.ServerStateTag));

    if(response.size() <= 0)
        return false;
    if(response[0].size() <= 0)
        return false;

    return QString(response[0][0].c_str()) == "t";
}

bool sql_io::connect(int host_number)
{
    connected = false;
    if(global_station.sql_settings.ip.size() <= host_number)
    {
        pMainW->log_write("Нет доступных хостов для подключения к БД", Qt::GlobalColor::red);
        return false;
    }

    // попытка подключения
    try
    {
        this->connected_ip = global_station.sql_settings.ip.at(host_number);
        pMainW->log_write(QString("Попытка подключения к БД по адресу %1 ...").arg(connected_ip));
        connectionObject = new pqxx::connection(global_station.sql_settings.get_connectionString(this->connected_ip, this->template_name).toStdString().data());

        if(!serverAllowed())
        {
            pMainW->log_write("Сервер не является основным. Переходим к следующему хосту", Qt::black);
            return reconnect(++host_number);
        }

    } catch (...) {
        pMainW->log_write(QString("Ошибка подключения"), Qt::GlobalColor::red);
        return reconnect(++host_number);
    }

    connected = true;
    clearOldConnections();
    checkRMAP();
    return true;
}

//проверка на установку rmap модуля
void sql_io::checkRMAP()
{
    bool rmap_found = false;
    pqxx::result test_result = execute("select * from pg_extension");
    for (auto const & row : test_result)
        for (auto const & cell : row)
            if(QString(cell.c_str()).contains("rmap", Qt::CaseInsensitive))
                rmap_found = true;

    if(!rmap_found)
        pMainW->log_write("Было произведено успешное подключение к БД, но не удалось обнаружить модуль RMAP", Qt::red);
    else
        pMainW->log_write("Подключено успешно", Qt::darkGreen);
}

void sql_io::clearOldConnections()
{
    if(global_station.sql_settings.clear_old_connects)
    {
        QString app_name = global_station.sql_settings.getAppName(template_name);
        QString terminate_sessions_q = QString("SELECT pg_terminate_backend(pg_stat_activity.pid) FROM pg_stat_activity WHERE pg_stat_activity.application_name = '%1' AND pid <> pg_backend_pid()").arg(app_name);
        execute(terminate_sessions_q);
    }
}

void sql_io::disconnect()
{
    connected = false;
    delete connectionObject;
    connectionObject = 0;
}

bool sql_io::reconnect(int host_number)
{
    disconnect();
    return connect(host_number);
}

std::pair<pqxx::result, bool> sql_io::execute_async(const QString& querry) noexcept
{    
    if(!connectionObject)
        return std::make_pair(pqxx::result(), true);
    try {
        pqxx::result response;
        pqxx::work worker(*connectionObject);        
        response =  worker.exec(querry.toStdString());
        return std::make_pair(response, false);
    }
    catch (...) {       
        return std::make_pair(pqxx::result(), true);
    }
    return std::make_pair(pqxx::result(), true);
}

pqxx::result sql_io::execute(QString querry)
{
    pqxx::result response;

    std::future<std::pair<pqxx::result, bool>> fut = std::async(std::launch::async, &sql_io::execute_async, this, querry);

    int timeout_interval = 40;
    std::future_status status;
    do{
        QApplication::processEvents();
        status = fut.wait_for(std::chrono::seconds(1));
        if(status == std::future_status::timeout)
            --timeout_interval;
    }
    while(status != std::future_status::ready && timeout_interval > 0);

    if(timeout_interval == 0)
    {
        pMainW->log_write("Возможно, пока программа ожидает ответа от базы данных, соединение было разорвано. Принудительно закрываем сокет...", Qt::red);
        connectionObject->close();
    }

    std::pair<pqxx::result, bool> res_pair = fut.get();
    response = std::get<0>(res_pair);

    if(std::get<1>(res_pair))
    {
        if(reconnect())
        {            
            return execute(querry);
        }
        else
        {
            pMainW->log_write("Попытка восстановить соединение не удалась", Qt::red);
            return pqxx::result();
        }
    }
   return response;
}

void sql_io::sendAlarm_SCADA(QString msg)
{
    QString alm_msg = QString("<Subcondition Message=''%1'' Value='''' Sound='''' Severity=''9'' Enabled=''1'' SoundEnabled=''0'' />").arg(msg);
    QString query("SELECT * FROM nodes Where nodes.TagName = 'Alarm.GenerateAlarm'");
    QString update_query = QString("UPDATE nodes_values v SET valstring='%1' FROM nodes n WHERE n.nodeid=v.nodeid AND n.tagname='Alarm.GenerateAlarm'").arg(alm_msg);

    pqxx::result res = execute(query);
    if (res.size() > 0)
        execute(update_query);
}
