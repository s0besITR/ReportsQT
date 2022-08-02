#pragma once

#include <pqxx/pqxx>
#include <QString>

class sql_io
{
public:    
    bool connected;
    QString connected_ip;
private:
    QString template_name;
    pqxx::connection * connectionObject;						// Указатель на объект коннекта с БД
public:

    sql_io(QString template_name)
        : connected(false), connected_ip(""), template_name(template_name), connectionObject(0)//, worker(0)
    {};   
   ~sql_io();

    static bool checkConnection(QString connection_string, QString & err);
    bool connect(int host_number = 0);
    bool reconnect(int host_number = 0);
    void disconnect();
    pqxx::result execute(QString querry);    
    void sendAlarm_SCADA(QString msg);
    bool serverAllowed(bool print=true);

private:
    void clearOldConnections();
    void checkRMAP();

    std::pair<pqxx::result, bool> execute_async(const QString& querry) noexcept;

};
