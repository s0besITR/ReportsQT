#include "ui_connectionDialog.h"
#include <QGroupBox>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QDebug>
#include <QTimeEdit>

#include "xml_io.h"
#include "sql_io.h"
#include "ui/ui_mainWindow.h"

extern Station::xml_station global_station;
extern ui_mainWindow* pMainW;

ui_connectionDialog::ui_connectionDialog(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::Dialog);
    setWindowModality(Qt::WindowModal);   
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setWindowTitle("Настройки подключения к базе данных");
    setFixedSize(600,550);

    QPushButton* cancelBtn = new QPushButton("Отмена");
    cancelBtn->setMaximumWidth(150);
    connect(cancelBtn, SIGNAL(clicked()), this, SLOT(btnEscapeClicked()));
    QPushButton* okBtn = new QPushButton("ОК");
    okBtn->setMaximumWidth(150);
    connect(okBtn, SIGNAL(clicked()), this, SLOT(btnOkClicked()));

    QHBoxLayout* hb_layout = new QHBoxLayout;
    hb_layout->addWidget(okBtn);
    hb_layout->addWidget(cancelBtn);

    QVBoxLayout* vb_layout = new QVBoxLayout;
    vb_layout->addWidget(createTopFrame());
    vb_layout->addWidget(createBottomFrame());
    vb_layout->addLayout(hb_layout);
    this->setLayout(vb_layout);

    initElements();
}

QGroupBox* ui_connectionDialog::createTopFrame()
{
    QGroupBox* frame = new QGroupBox("Настройки подключения", this);

    QGridLayout* g_layout = new QGridLayout;
    g_layout->setMargin(5);
    g_layout->setSpacing(15);  

    QString aStringLables [2][4] = {
        {"Хост 1", "Хост 2", "Хост 3", "Хост 4"}, {"Порт", "Имя БД", "Пользователь", "Пароль"}};

    QString aMapKeys [2][4] = {
        {"ip1", "ip2", "ip3", "ip4"}, {"port", "db_name", "user", "pwd"}};

    for(int i = 0; i < 2; ++i)
        for(int j = 0; j < 4; ++j)
        {
            QLineEdit* tmp_edit =  new QLineEdit();
            tmp_edit->setAlignment(Qt::AlignHCenter);
            mapEdit.insert(aMapKeys[i][j],tmp_edit);

            g_layout->addWidget(new QLabel(aStringLables[i][j]) ,j+1, i*2);
            g_layout->addWidget(tmp_edit ,j+1, i*2 + 1);
        }

    connect_btn = new QPushButton("Проверить подключение");
    connect_btn->setMinimumSize(QSize(100,25));
    connect(connect_btn,SIGNAL(clicked()),this, SLOT(checkConnection()));
    g_layout->addWidget(connect_btn,5,0,1,4, Qt::AlignHCenter);
    frame->setLayout(g_layout);

    return frame;
}
QGroupBox* ui_connectionDialog::createBottomFrame()
{
    QGroupBox* frame = new QGroupBox("Прочее",this);
    QVBoxLayout* vb_layout = new QVBoxLayout;
    vb_layout->setMargin(5);
    vb_layout->setSpacing(15);

    QString aStringLables [4] = {"Сбрасывать прошлые соединения перед работой с БД", "Формировать аларм в SCADA", "Проверять наличие тега в БД", "Соединяться только с основным сервером"};
    QString aMapKeys [4] = {"clear_connection", "scada_alarm", "check_tag", "check_state"};

    for(int i = 0; i < 4; ++i)
    {
       QCheckBox* tmp = new QCheckBox(aStringLables[i]);
       mapCheckBox.insert(aMapKeys[i],tmp);
       vb_layout->addWidget(tmp,Qt::AlignLeft);
    }

    server_status_tag = new QLineEdit();
    server_status_tag->setAlignment(Qt::AlignCenter);

    QHBoxLayout *hb_layout0 = new QHBoxLayout;
    hb_layout0->addWidget(new QLabel("Тег, хранящий статсус сервера (bool)"));
    hb_layout0->addWidget(server_status_tag);
    vb_layout->addLayout(hb_layout0);

    negative_time = new QPushButton("?");
    negative_time->setMaximumWidth(30);

    QHBoxLayout *hb_layout = new QHBoxLayout;    
    hb_layout->addWidget(new QLabel("Смещение от UTC (ч:м)"));

    utc_time = new QTimeEdit();
    utc_time->setAlignment(Qt::AlignHCenter);
    hb_layout->addWidget(negative_time);
    hb_layout->addSpacing(-10);
    hb_layout->addWidget(utc_time);
    hb_layout->addStretch(1);
    vb_layout->addLayout(hb_layout);
    frame->setLayout(vb_layout);
    return frame;
}
void ui_connectionDialog::initElements()
{
    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegExp ipRegex ("^" + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange + "$");
    QRegExp portRegex("\\d*");
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
    QRegExpValidator *portValidator = new QRegExpValidator(portRegex, this);

    for(int i = 0; i < global_station.sql_settings.ip.count(); ++i)
    {
        if(i==4)
            break;
        mapEdit[QString("ip%1").arg(i+1)]->setText(global_station.sql_settings.ip.at(i));        
        mapEdit[QString("ip%1").arg(i+1)]->setValidator(ipValidator);
    }
    mapEdit["port"]->setText(global_station.sql_settings.port);
    mapEdit["port"]->setValidator(portValidator);
    mapEdit["db_name"]->setText(global_station.sql_settings.dbname);
    mapEdit["user"]->setText(global_station.sql_settings.user);
    mapEdit["pwd"]->setText(global_station.sql_settings.password);

    QRegularExpression reg("(?<=\')[^']*");
    QRegularExpressionMatchIterator matches = reg.globalMatch(global_station.sql_settings.utc_offset);
    if(matches.hasNext())
    {
        utc_time->setTime(QTime::fromString(matches.next().captured(0),"hh:mm"));
    }

    if(global_station.sql_settings.utc_offset.contains("+"))
        negative_time->setText("+");
    else if(global_station.sql_settings.utc_offset.contains("-"))
        negative_time->setText("-");
    connect(negative_time, SIGNAL(clicked()), this, SLOT(negativeBtnClicked()));

    mapCheckBox["clear_connection"]->setChecked(global_station.sql_settings.clear_old_connects);
    mapCheckBox["scada_alarm"]->setChecked(global_station.sql_settings.send_alarm);
    mapCheckBox["check_tag"]->setChecked(global_station.sql_settings.checkTagExistance);
    mapCheckBox["check_state"]->setChecked(global_station.sql_settings.checkServerState);

    connect(mapCheckBox["check_state"], SIGNAL(stateChanged(int)), this, SLOT(checkStateListenner(int)));

    server_status_tag->setText(global_station.sql_settings.ServerStateTag);
    this->server_status_tag->setEnabled(mapCheckBox["check_state"]->isChecked());
}

void ui_connectionDialog::checkConnection(){
    QString err;
    QStringList hosts;
    hosts << mapEdit["ip1"]->text() << mapEdit["ip2"]->text() << mapEdit["ip3"]->text() << mapEdit["ip4"]->text();

    connect_btn->setText("Установка соединения...");
    for(const QString & host : hosts)
    {
        if(sql_io::checkConnection(global_station.sql_settings.get_connectionString(host, ""), err))        {

            QMessageBox msgb(
                        QMessageBox::Question,
                        "Успешное подключение",
                        QString("Соединение установлено с хостом %1\nПерейти к следующему хосту?").arg(host),
                        QMessageBox::Yes|QMessageBox::No,
                        this);
            msgb.setButtonText(QMessageBox::Yes, tr("Да"));
            msgb.setButtonText(QMessageBox::No, tr("Нет"));

            if(msgb.exec() == QMessageBox::No)
                break;
        }
        else
        {
            QMessageBox msgb(
                        QMessageBox::Question,
                        "Ошибка подключения",
                        QString("Не удалось подключиться к хосту %1\nПерейти к следующему хосту?").arg(host),
                        QMessageBox::Yes|QMessageBox::No,
                        this);
            msgb.setButtonText(QMessageBox::Yes, tr("Да"));
            msgb.setButtonText(QMessageBox::No, tr("Нет"));
            if(msgb.exec() == QMessageBox::No)
                break;
        }
    }
    connect_btn->setText("Проверить подключение");
}
void ui_connectionDialog::btnOkClicked(){
    global_station.sql_settings.ip.clear();
    global_station.sql_settings.ip.push_back(mapEdit["ip1"]->text());
    global_station.sql_settings.ip.push_back(mapEdit["ip2"]->text());
    global_station.sql_settings.ip.push_back(mapEdit["ip3"]->text());
    global_station.sql_settings.ip.push_back(mapEdit["ip4"]->text());

    global_station.sql_settings.port = mapEdit["port"]->text();
    global_station.sql_settings.dbname = mapEdit["db_name"]->text();
    global_station.sql_settings.user = mapEdit["user"]->text();
    global_station.sql_settings.password = mapEdit["pwd"]->text();

    QRegularExpression reg("(?<=\')[^']*");
    QRegularExpressionMatchIterator matches = reg.globalMatch(global_station.sql_settings.utc_offset);

    if(matches.hasNext())
        global_station.sql_settings.utc_offset = global_station.sql_settings.utc_offset.replace(matches.next().captured(0), utc_time->time().toString("hh:mm"));

    if(global_station.sql_settings.utc_offset.contains("+"))
        global_station.sql_settings.utc_offset = global_station.sql_settings.utc_offset.replace("+",negative_time->text());
    else if(global_station.sql_settings.utc_offset.contains("-"))
        global_station.sql_settings.utc_offset = global_station.sql_settings.utc_offset.replace("-",negative_time->text());


    global_station.sql_settings.clear_old_connects = mapCheckBox["clear_connection"]->isChecked();
    global_station.sql_settings.send_alarm = mapCheckBox["scada_alarm"]->isChecked();
    global_station.sql_settings.checkTagExistance = mapCheckBox["check_tag"]->isChecked();

    global_station.sql_settings.checkServerState = mapCheckBox["check_state"]->isChecked();
    global_station.sql_settings.ServerStateTag = server_status_tag->text();

    XmlParse::writeStationCfg(global_station.sql_settings);


    //дисконнект от базы данных, если айпи адреса поменялись
    if(pMainW->sql_obj)
         if(pMainW->sql_obj->connected)
            pMainW->sql_obj->disconnect();

    this->close();
}
void ui_connectionDialog::btnEscapeClicked(){
    this->close();
}

void ui_connectionDialog::checkStateListenner(int state)
{
    this->server_status_tag->setEnabled(state == Qt::CheckState::Checked);
    this->server_status_tag->setReadOnly(state != Qt::CheckState::Checked);
}

void ui_connectionDialog::negativeBtnClicked()
{
    QString t = this->negative_time->text();
    if( t == "-")
        this->negative_time->setText("+");
    if( t == "+")
        this->negative_time->setText("-");
}
