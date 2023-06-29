#include "ui/ui_mainWindow.h"
#include <QLayout>
#include <QStatusBar>
#include <QAction>
#include <QMenuBar>
#include <QDebug>
#include <QPushButton>
#include <QProgressBar>
#include <QDateTimeEdit>
#include <QFrame>
#include <QLabel>
#include <QComboBox>
#include <QDesktopServices>

QString report_timestamp;

MyTextEdit::MyTextEdit(QWidget *parent) : QTextEdit(parent) {
   clearAction = new QAction("Clear",this);
   connect(clearAction, SIGNAL(triggered(bool)), this, SLOT(clear()));
};

MyTextEdit::MyTextEdit(const QString &text, QWidget *parent) : QTextEdit(text, parent) {
   clearAction = new QAction("Clear",this);
   connect(clearAction, SIGNAL(triggered(bool)), this, SLOT(clear()));
};

void MyTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu* menu = createStandardContextMenu(event->pos());
    menu->addAction(clearAction);
    menu->exec(event->globalPos());
    delete menu;
}

void MyTextEdit::clearLog()
{
    this->clear();
}

ui_mainWindow::ui_mainWindow(cmd_arguments args, QWidget *parent)
    : QMainWindow{parent}, sql_obj(0), cmd_args(args), stop_clicked(false)
{
    QWidget *widget = new QWidget;
    setCentralWidget(widget);
    installEventFilter(this);

    pProgressBar = new QProgressBar(widget);
    pProgressBar->setAlignment(Qt::AlignCenter);    
    pProgressBar->hide();

    btn_stopReport = new QPushButton("Стоп", widget);
    connect(btn_stopReport, SIGNAL(clicked()), this, SLOT(stopReport()));
    btn_stopReport->hide();

    createActions();
    createMenus();

    logEdit = new MyTextEdit("Программа готова к работе");
    logEdit->setReadOnly(true);
    QLabel * log_label = new QLabel("Лог");

    print_querries = new QCheckBox("Печатать SQL запросы");
    print_querries->setCheckState(Qt::Unchecked);

    //Компоновка
    QHBoxLayout * hb_layout_log = new QHBoxLayout;
    hb_layout_log->addWidget(log_label);
    hb_layout_log->addStretch(10);
    hb_layout_log->addWidget(print_querries);

    QHBoxLayout * hb_layout = new QHBoxLayout;
    hb_layout->addWidget(createFrameReports(),1);

    QVBoxLayout *vb_layout_tab = new QVBoxLayout;
    QLabel * tab_label = new QLabel("Отчеты");
    vb_layout_tab->addWidget(tab_label);
    vb_layout_tab->addWidget(createFrameExplorer());

    hb_layout->addLayout(vb_layout_tab,4);

    QVBoxLayout *layout = new QVBoxLayout;
    layout ->setContentsMargins(5,5,5,5);
    layout->addLayout(hb_layout,4);
    layout->addLayout(hb_layout_log);
    layout->addWidget(logEdit,5);

    widget->setLayout(layout);
    statusBar()->addPermanentWidget(pProgressBar);
    statusBar()->addPermanentWidget(btn_stopReport);
    setWindowTitle(QString("Отчеты для Alpha.Server (%1)").arg(global_station.name));
    setMinimumSize(1050, 550);


    // Аргументы командной строки
    if(cmd_args.modal)
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    if(cmd_args.minimize)
        setWindowState(windowState() | Qt::WindowMinimized);
    if(cmd_args.monitor != 1)
    {
        if(QGuiApplication::screens().size() <= cmd_args.monitor)
        {
            QPoint center_point = QGuiApplication::screens().at(cmd_args.monitor - 1)->geometry().center();
            center_point.setX(center_point.x() - this->width()/2);
            center_point.setY(center_point.y() - this->height()/2);
            move(center_point);
        }
    }

    // Аргументы, которые делают сразу же и отчет
    if(cmd_args.template_name != "")
    {
        int pos = chooseTemplate->findText(cmd_args.template_name, Qt::MatchExactly);
        if(pos != -1)
            chooseTemplate->setCurrentIndex(pos);
        else
        {
            log_write(QString("В списке шаблонов не был найден %1").arg(cmd_args.template_name),Qt::red);
            cmd_args.template_name = "";
        }
    }
    if(cmd_args.tag != "")
    {
        Template::xml_template & cur_template = global_templates[chooseTemplate->currentText()];
        for(int i = 0; i < cur_template.report_structure.data.size(); ++i)
            if(cur_template.report_structure.data[i].tag.compare(cmd_args.tag,Qt::CaseInsensitive) != 0 )
                cur_template.report_structure.data[i].includeInReport = false;
    }
    if(cmd_args.time != "")
    {
        if(cmd_args.time.startsWith("last"))
        {
            int day_offset = cmd_args.time.replace("last","",Qt::CaseInsensitive).toInt() * (-1);
            QDate date = date_to->dateTime().date().addDays(day_offset);
            date_from->setDate(date);
        }
        else
        {
            QStringList dates = cmd_args.time.split("|");
            if(dates.length() == 2)
            {
                QDateTime from = QDateTime::fromString(dates.at(0),"yyyy-MM-dd hh:mm:ss");
                QDateTime to = QDateTime::fromString(dates.at(1),"yyyy-MM-dd hh:mm:ss");
                if(from.toString().size() == 0)
                    from = QDateTime::fromString(dates.at(0),"yyyy-MM-dd hh:mm");
                if(to.toString().size() == 0)
                    to = QDateTime::fromString(dates.at(1),"yyyy-MM-dd hh:mm");

                if(from.toString().size() == 0 || to.toString().size() == 0)
                    log_write(QString("Неверный формат даты и времени %1").arg(cmd_args.time),Qt::red);
                else
                {
                    date_from->setDate(from.date());
                    time_from->setTime(from.time());
                    date_to->setDate(to.date());
                    time_to->setTime(to.time());
                }
            }
        }


    }
}

// Запрещаем сворачивание приложения, если исходя из cmd_args.foldable
bool ui_mainWindow::eventFilter(QObject *obj, QEvent *event) {

    if (event->type() == QEvent::WindowStateChange && isMinimized() && !cmd_args.foldable) {
           setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
           statusBar()->showMessage("Программа была запущена с параметром, который запрещает сворачивание приложения", 2000);
           return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

QGroupBox* ui_mainWindow::createFrameReports()
{
    QGroupBox* frame = new QGroupBox("Шаблоны");
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chooseTemplate = new QComboBox;
    chooseTemplate->addItems(QStringList(global_templates.keys()));
    chooseTemplate->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(chooseTemplate, SIGNAL(currentIndexChanged(int)), this, SLOT(templateChanged()));

    QPushButton * devsBtn = new QPushButton("Выбрать устройства");
    devsBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    devsBtn->setFixedWidth(200);
    devsBtn->setStatusTip("Открыть окно выбора устройств, которые войдут в отчет");
    connect(devsBtn,SIGNAL(clicked()), this, SLOT(chooseDevicesDlgOpen()));

    date_from = new QDateEdit(QDate(QDate::currentDate().addMonths(-1)));
    date_from->setDisplayFormat("dd.MM.yyyy");
    date_from->setCalendarPopup(true);
    date_to = new QDateEdit(QDate(QDate::currentDate()));
    date_to->setDisplayFormat("dd.MM.yyyy");
    date_to->setCalendarPopup(true);

    time_from = new QTimeEdit(QTime::currentTime());
    time_from->setDisplayFormat("hh:mm:ss");
    time_to = new QTimeEdit(QTime::currentTime());
    time_to->setDisplayFormat("hh:mm:ss");

    doReport = new QPushButton("Сформировать отчет");
    doReport->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    doReport->setFixedWidth(200);
    doReport->setStatusTip("Создание отчета по выбранным параметрам");
    connect(doReport,SIGNAL(clicked()), this, SLOT(makeReport()));
    doReport->setDisabled(global_templates.empty());

    QLabel* hist_label_begin = new QLabel("Начало");
    QLabel* hist_label_end = new QLabel("Конец");
    hist_label_begin->setAlignment(Qt::AlignHCenter);
    hist_label_end->setAlignment(Qt::AlignHCenter);

    QVBoxLayout *vb_layout_interval = new QVBoxLayout;
    QHBoxLayout *hb_layout_labels = new QHBoxLayout;
    QHBoxLayout *hb_layout_date = new QHBoxLayout;
    QHBoxLayout *hb_layout_time = new QHBoxLayout;

    QGroupBox* gr_box = new QGroupBox("Выбор временного интервала  ", this);

    hb_layout_labels->addWidget(hist_label_begin);
    hb_layout_labels->addWidget(hist_label_end);
    hb_layout_date->addWidget(date_from);
    hb_layout_date->addWidget(date_to);
    hb_layout_time->addWidget(time_from);
    hb_layout_time->addWidget(time_to);

    vb_layout_interval->addLayout(hb_layout_labels);
    vb_layout_interval->addLayout(hb_layout_date);
    vb_layout_interval->addLayout(hb_layout_time);

    gr_box->setLayout(vb_layout_interval);

    QVBoxLayout *vb_layout = new QVBoxLayout;
    vb_layout->addWidget(chooseTemplate);
    vb_layout->addWidget(devsBtn);
    vb_layout->setAlignment(devsBtn, Qt::AlignHCenter);
    vb_layout->addStretch();
    vb_layout->addWidget(gr_box);
    vb_layout->addStretch();
    vb_layout->addWidget(doReport);
    vb_layout->setAlignment(doReport, Qt::AlignHCenter);

    vb_layout->addStretch();

    frame->setLayout(vb_layout);

    templateChanged();
    return frame;
}

QTabWidget* ui_mainWindow::createFrameExplorer()
{
    tabs = new QTabWidget();
    QString root_path_oper;
    QString root_path_hist;

    root_path_oper = "./reports/Оперативные";
    root_path_hist = "./reports/Исторические";

    if(!QDir(root_path_oper).exists())
    {
        QDir().mkpath(root_path_oper);
    }
    if(!QDir(root_path_hist).exists())
    {
        QDir().mkpath(root_path_hist);
    }

    auto setupModel = [](QFileSystemModel* model, QTreeView* treeView, QString root_path)
    {
        model->setRootPath(root_path);
        model->setReadOnly(false);
        treeView->setModel(model);
        treeView->setRootIndex(model->index(root_path));
        treeView->hideColumn(1);
        treeView->hideColumn(2);
        treeView->hideColumn(3);
        treeView->setHeaderHidden(true);
        treeView->setEditTriggers(QAbstractItemView::SelectedClicked);
        treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    };

    QFileSystemModel* model_oper = new QFileSystemModel(this);
    treeView_oper = new QTreeView(this);
    setupModel(model_oper, treeView_oper,root_path_oper);
    connect(treeView_oper, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotCustomMenuRequested(QPoint)));

    QFileSystemModel* model_hist = new QFileSystemModel(this);
    treeView_hist = new QTreeView(this);
    setupModel(model_hist, treeView_hist,root_path_hist);
    connect(treeView_hist, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotCustomMenuRequested(QPoint)));

    connect(treeView_oper, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(openFileFromExplorer(const QModelIndex&)));
    connect(treeView_hist, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(openFileFromExplorer(const QModelIndex&)));

    tabs->addTab(treeView_oper,"Оперативные");
    tabs->addTab(treeView_hist,"Исторические");

    return tabs;
}

void ui_mainWindow::slotCustomMenuRequested(QPoint pos)
{
    QMenu * menu = new QMenu(this);
    QAction * deleteOne = new QAction("Удалить", this);
    QAction * deleteAll = new QAction("Удалить все", this);

    deleteOne->setStatusTip("Удалить выбранный файл");
    deleteAll->setStatusTip("Удалить все файлы из текущей дирректориии");

    connect(deleteOne, SIGNAL(triggered()), this, SLOT(deleteSelectedReport()));
    connect(deleteAll, SIGNAL(triggered()), this, SLOT(deleteAllReports()));

    QTreeView *tree_p = nullptr;
    if(tabs->currentIndex() == 0) //Оперативные
    {
        tree_p = treeView_oper;
    }
    else
    {
        tree_p = treeView_hist;
    }

    QModelIndex index = tree_p->indexAt(pos);
    if (index.isValid()) {
         menu->addAction(deleteOne);
         menu->addAction(deleteAll);
    }
    else
    {
        menu->addAction(deleteAll);
    }
    menu->popup(tree_p->viewport()->mapToGlobal(pos));
}

void ui_mainWindow::deleteAllReports()
{
    QTreeView *tree_p = nullptr;
    if(tabs->currentIndex() == 0) //Оперативные
        tree_p = treeView_oper;
    else
        tree_p = treeView_hist;

    QString path = dynamic_cast<const QFileSystemModel*>(tree_p->rootIndex().model())->filePath(tree_p->rootIndex());

    QMessageBox msgb(
                QMessageBox::Question,
                "Подтвердите дейтствие",
                QString("Вы действительно хотите удалить все файлы по пути\n%1 ?").arg(path),
                QMessageBox::Yes|QMessageBox::No,
                tree_p);
    msgb.setButtonText(QMessageBox::Yes, tr("Да"));
    msgb.setButtonText(QMessageBox::No, tr("Нет"));

    if(msgb.exec() == QMessageBox::Yes)
    {
        QDir dir(path);
        dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
        foreach(QString dirItem, dir.entryList())
            dir.remove( dirItem );

        dir.setFilter(QDir::Dirs |  QDir::NoDotAndDotDot);
        foreach(QString dirItem, dir.entryList())
        {
            QDir subDir( dir.absoluteFilePath( dirItem ) );
            subDir.removeRecursively();
        }
        log_write(QString("Удалены все файлы по пути: %1").arg(path));
    }
}

void ui_mainWindow::deleteSelectedReport()
{
    QTreeView *tree_p = nullptr;
    if(tabs->currentIndex() == 0) //Оперативные
        tree_p = treeView_oper;
    else
        tree_p = treeView_hist;

    QModelIndex index = tree_p->currentIndex();
    if (index.isValid()) {
         QString path = dynamic_cast<const QFileSystemModel*>(index.model())->filePath(index);
         QString name = dynamic_cast<const QFileSystemModel*>(index.model())->fileName(index);

         QMessageBox msgb(
                     QMessageBox::Question,
                     "Подтвердите дейтствие",
                     QString("Вы действительно хотите удалить файл %1 ?").arg(name),
                     QMessageBox::Yes|QMessageBox::No,
                     tree_p);
         msgb.setButtonText(QMessageBox::Yes, tr("Да"));
         msgb.setButtonText(QMessageBox::No, tr("Нет"));

        if(msgb.exec() == QMessageBox::Yes)
        {
            const QFileInfo outputDir(path);
            if(outputDir.isFile())
            {
                QDir().remove(path);
                log_write(QString("Удален файл по пути: %1").arg(path));
            }
            else
            {
               QDir(path).removeRecursively();
               log_write(QString("Удалена папка по пути: %1").arg(path));
            }
        }
    }
}

void ui_mainWindow::openFileFromExplorer(const QModelIndex& index)
{

   QString path = dynamic_cast<const QFileSystemModel*>(index.model())->filePath(index);
   if(path.endsWith(".xlsx"))
        QDesktopServices::openUrl(QUrl(QUrl::fromLocalFile(path)));
}

void ui_mainWindow::createActions()
{
    showConnectionDlg = new QAction("Подключение", this);
    showConnectionDlg->setStatusTip("Изменение настроек подключения к БД postgres");
    connect(showConnectionDlg, &QAction::triggered, this, &ui_mainWindow::EditConnectionCfg);

    reload = new QAction("Перезагрузить конфигурацию", this);
    reload->setStatusTip("Обновить список шаблонов и файл конфигурации station.xml");
    connect(reload, &QAction::triggered, this, &ui_mainWindow::ReloadConfiguration);

    activateDebug = new QAction("Отладочный режим", this);
    activateDebug->setCheckable(true);
    activateDebug->setChecked(false);
    activateDebug->setStatusTip("В отладочном режиме sql запросы в БД не посылаются. Отчет печатается без них");

    showAboutDlg = new QAction("О программе", this);
    showAboutDlg->setStatusTip("Показать информацию о версии приложения");
    connect(showAboutDlg, &QAction::triggered, this, &ui_mainWindow::About);

    help = new QAction("Помощь", this);
    help->setStatusTip("Открыть инструкцию");
    connect(help, &QAction::triggered, this, &ui_mainWindow::Help);
}

void ui_mainWindow::createMenus()
{
    connectionMenu = menuBar()->addMenu("Настройки");
    connectionMenu->addAction(showConnectionDlg);
    connectionMenu->addAction(reload);
    connectionMenu->addAction(activateDebug);

    helpMenu = menuBar()->addMenu("Справка");
    helpMenu->addAction(showAboutDlg);
    helpMenu->addAction(help);
}

bool ui_mainWindow::load_configuration()
{
    global_templates.clear();
    try{

        global_station =  XmlParse::readStationCfg();

        QDir directory("." + TEMPLATE_DIR);
        QStringList templates = directory.entryList(QStringList() << "*.xml" << "*.XML",QDir::Files);
        foreach(QString filename, templates) {
            if(filename != "station.xml")
                global_templates.insert(filename, XmlParse::readTemplateCfg(filename));
        }
    }
    catch(ParseException& e)
    {
        QMessageBox::critical(nullptr,"Критическая ошибка", e.what(), QMessageBox::Ok);
        return false;
    }

    foreach(QString t, global_templates.keys())
    {
        if(t=="")
             global_templates.remove(t);
    }
    return true;
}
void ui_mainWindow::ReloadConfiguration()
{
    if(!this->doReport->isEnabled())
        return;

    chooseTemplate->clear();
    if(!load_configuration())
        this->close();
    chooseTemplate->addItems(QStringList(global_templates.keys()));
    log_write("Конфигурация обновлена");
}

void ui_mainWindow::EditConnectionCfg()
{
    if(!this->doReport->isEnabled())
        return;
    ui_connectionDialog* connectionDlg = new ui_connectionDialog(this);
    connectionDlg->show();          // Окно саморазрушается при закрытии
}

void ui_mainWindow::About()
{
    ui_AboutDialog* aboutDlg = new ui_AboutDialog(this);
    aboutDlg->show();
}

void ui_mainWindow::chooseDevicesDlgOpen()
{
    ui_chooseDevsDialog* devsDlg = new ui_chooseDevsDialog(global_templates[chooseTemplate->currentText()].report_structure.data, this);
    devsDlg->show();
}

void ui_mainWindow::templateChanged()
{
    Template::xml_template & cur_template = global_templates[chooseTemplate->currentText()];

    for(int i = 0; i < cur_template.report_structure.data.size(); ++i)
        cur_template.report_structure.data[i].includeInReport = true;

    date_from->setEnabled(!cur_template.oper_mode);
    date_to->setEnabled(!cur_template.oper_mode);
    time_from->setEnabled(!cur_template.oper_mode);
    time_to->setEnabled(!cur_template.oper_mode);
}

void ui_mainWindow::replaceCtrlStrings(Template::xml_template & t)
{
    for(Template::row & r : t.report_structure.head_rows)
        for(Template::cell &c : r)
            if(c.value.contains("%"))
            {
                c.value.replace("%report_time%",report_timestamp, Qt::CaseInsensitive);
                c.value.replace("%report_dt_start%", date_from->date().toString("yyyy-MM-dd") + " " + time_from->time().toString("hh:mm:ss"), Qt::CaseInsensitive);
                c.value.replace("%report_dt_end%", date_to->date().toString("yyyy-MM-dd") + " " + time_to->time().toString("hh:mm:ss"), Qt::CaseInsensitive);
            }
    // Удаляем элементы, в которых есть ошибка
    for(auto it = t.report_structure.data.begin(); it != t.report_structure.data.end();)
    {
        if(!t.types.contains((*it).type))
        {
            log_write(QString("Не найден тип %1 у элемента с тегом %2. Элемент исключен из отчета").arg((*it).type, (*it).tag), Qt::darkYellow);
            it = t.report_structure.data.erase(it);
            continue;
        }

        if(!global_station.objects.contains((*it).tag))
        {
            log_write(QString("Не найден тег %1 среди тегов файла station.xml. Элемент исключен из отчета").arg((*it).tag), Qt::darkYellow);
            it = t.report_structure.data.erase(it);
            continue;
        }
        ++it;
    }

    for(Template::report_data& d : t.report_structure.data)
    {
        Template::type & type = t.types[d.type];
        type.sql_querry.replace("%report_dt_start%", date_from->date().toString("yyyy-MM-dd") + " " + time_from->time().toString("hh:mm:ss"), Qt::CaseInsensitive);
        type.sql_querry.replace("%report_dt_end%", date_to->date().toString("yyyy-MM-dd") + " " + time_to->time().toString("hh:mm:ss"), Qt::CaseInsensitive);

        QString offset = global_station.sql_settings.utc_offset;
        type.sql_querry.replace("%time%", QString("(time%1)").arg(offset), Qt::CaseInsensitive);
        type.sql_querry.replace("%actual_time%", QString("(actual_time%1)").arg(offset), Qt::CaseInsensitive);
        type.sql_querry.replace("%utc_offset%", offset.replace("+","", Qt::CaseInsensitive).replace("-","", Qt::CaseInsensitive), Qt::CaseInsensitive);
    }
}

void ui_mainWindow::replaceCtrlStrings(Template::row & r,QString& sql_querry, const Template::tag & low_tag, const QString & data_tag)
{
    Station::object_fields & field = global_station.objects[data_tag];

    for(Template::cell &c : r)
        if(c.value.contains("%"))
        {
            c.value.replace("%obj_name%",field.name, Qt::CaseInsensitive);
            c.value.replace("%obj_location%",field.location, Qt::CaseInsensitive);
            c.value.replace("%obj_type%",field.common_type, Qt::CaseInsensitive);
            c.value.replace("%tag_name%",low_tag.name, Qt::CaseInsensitive);
            c.value.replace("%station_name%",global_station.name, Qt::CaseInsensitive);
            c.value.replace("%full_tag%",global_station.tag_prefix + data_tag + low_tag.value, Qt::CaseInsensitive);
        }

    if(sql_querry.contains("%"))
    {
        sql_querry.replace("%obj_name%",field.name, Qt::CaseInsensitive);
        sql_querry.replace("%obj_location%",field.location, Qt::CaseInsensitive);
        sql_querry.replace("%obj_type%",field.common_type, Qt::CaseInsensitive);
        sql_querry.replace("%tag_name%",low_tag.name, Qt::CaseInsensitive);
        sql_querry.replace("%station_name%",global_station.name, Qt::CaseInsensitive);
        sql_querry.replace("%full_tag%",global_station.tag_prefix + data_tag + low_tag.value, Qt::CaseInsensitive);
    }
}

void ui_mainWindow::updateReportTimestamp()
{
    QDateTime date = QDateTime::currentDateTime();
    report_timestamp = date.toString("dd.MM.yyyy, hh:mm:ss");
}


int ui_mainWindow::countQueries()
{
    int result = 0;

    Template::xml_template cur_template = global_templates[chooseTemplate->currentText()];
    for(Template::report_data& data : cur_template.report_structure.data)
    {
        if(!data.includeInReport)
            continue;
        Template::type const & type = cur_template.types[data.type];
        result+= type.tags.size();
    }
    return result;
}

void ui_mainWindow::makeReport()
{
    debug_ = activateDebug->isChecked();
    doReport->setEnabled(false);   
    activateDebug->setCheckable(false);
    QApplication::processEvents();

    updateReportTimestamp();
    int skipped_queries = 0;
    int run_queries = 0;
    int pg_step = 0;

    Template::xml_template cur_template = global_templates[chooseTemplate->currentText()];
    doc_io  doc(cur_template.name, cur_template.save_path, report_timestamp, cur_template.styles);

    logEdit->clear();
    if(this->cmd_args.template_name != "")
    {
        log_write("Программа была запущена с параметрами: ");
        log_write(QString("\tНазвание отчета: %1").arg(this->cmd_args.template_name));
        if(this->cmd_args.tag != "")
            log_write(QString("\tОтчет по тегу: %1").arg(this->cmd_args.tag));
        if(this->cmd_args.time != "")
        {
            if(this->cmd_args.time.contains("|",Qt::CaseInsensitive))
                log_write(QString("\tВременной интервал: %1").arg(this->cmd_args.time));
            else
                log_write(QString("\tВременной интервал: последние %1 дней").arg(this->cmd_args.time));

        }

        if(this->cmd_args.close_after != -1)
            log_write(QString("\tАвтоматическое закрытие программы через: %1 секунд").arg(this->cmd_args.close_after));
        log_write("Запускаем процедуру создания отчета\n");
    }

    if(!sql_obj)
        sql_obj = new sql_io(cur_template.name);

    if(debug_)
        log_write(QString("Включен отладочный режим. Соединение с БД postgresql производится не будет"), Qt::GlobalColor::darkYellow);
    else if(!sql_obj->connected)
        sql_obj->connect();
    else if(sql_obj->connected)
    {
        if(!sql_obj->serverAllowed())
        {
            log_write("Текущий подключенный сервер перестал быть основным. Перед созданием отчета будет произведен поиск основного сервера");
            sql_obj->reconnect();
        }
        else
            log_write(QString("Подключено к БД по адресу %1").arg(sql_obj->connected_ip), Qt::darkGreen);
    }

    this->pProgressBar->setRange(0, countQueries());
    pProgressBar->setValue(pg_step);
    pProgressBar->show();
    this->btn_stopReport->show();
    replaceCtrlStrings(cur_template);   

    log_write(QString("Начинаем создание отчета: %1").arg(cur_template.name));    

    //Шапка
    for(Template::row & r : cur_template.report_structure.head_rows)
        doc.drawRow(r);

    // Данные
    for(Template::report_data& data : cur_template.report_structure.data)
    {
        if(!data.includeInReport)
            continue;
        if(stop_clicked)
            break;

        this->statusBar()->showMessage(global_station.objects.value(data.tag).name);
        Template::type const & type = cur_template.types[data.type];
        for(Template::tag const & tag : type.tags)
        {
            if(stop_clicked)
                break;

            pProgressBar->setValue(++pg_step);
            Template::row tmp_row = type.one_row;
            QString sql_querry = type.sql_querry;
            replaceCtrlStrings(tmp_row,sql_querry, tag, data.tag);
            QApplication::processEvents();

            if(!checkTagExistsInDB(global_station.tag_prefix +  data.tag + tag.value))
            {
                ++skipped_queries;
                continue;
            }

            if(this->print_querries->isChecked())
                log_write(sql_querry,Qt::GlobalColor::gray);

            if(sql_obj->connected && !sql_obj->serverAllowed(false))
            {
                log_write("Текущий подключенный сервер перестал быть основным. Будет произведен поиск основного сервера");
                sql_obj->reconnect();
            }

            ++run_queries;

            for(Template::row const & r : get_sql_data(tmp_row, sql_querry, type.single_mode, cur_template.includeNullResult))
                doc.drawRow(r);
        }
    }

    if(!debug_ && this->print_querries->isChecked())
    {
        log_write(QString("Выполнено запросов всего: %1/%2").arg(run_queries).arg(countQueries()));
        if(skipped_queries > 0)
            log_write(QString("Пропущеных из-за отсутствия тега в БД: %1").arg(skipped_queries));
    }

   QString msg =  doc.save();
   if(global_station.sql_settings.send_alarm && !debug_ && sql_obj->connected)
       sql_obj->sendAlarm_SCADA(QString("Сообщение о генерации отчета: %1").arg(msg));

   this->statusBar()->showMessage("Завершено", 2000);
   pProgressBar->hide();
   if(cmd_args.close_after == -1)
   {
        doReport->setEnabled(true);
   }

   activateDebug->setCheckable(true);
   activateDebug->setChecked(debug_);
   stop_clicked = false;
   this->btn_stopReport->hide();
}

void ui_mainWindow::stopReport()
{
    stop_clicked = true;
}

void ui_mainWindow::printResponse(const pqxx::result & response)
{
    QString msg;
    int result_max = 5;

    for (auto const & row : response)
    {
        msg = "";
        for (auto const & cell : row)
            msg+= QString("%1\t").arg(cell.c_str());
        log_write(msg, Qt::GlobalColor::gray);
        if (result_max-- <= 0)
        {
            log_write(QString("Всего = %1").arg(response.size()), Qt::GlobalColor::gray);
            return;
        }
    }
}

QVector<Template::row> ui_mainWindow::get_sql_data(const Template::row & r, const QString & querry, bool single_mode, bool include_null_result)
{
    QVector<Template::row> result;    
    if(debug_ || !this->sql_obj->connected)
    {
        result << r;
        return result;
    }

    pqxx::result response = this->sql_obj->execute(querry);

    if(this->print_querries->isChecked())
        printResponse(response);

    // Если вернулся пустой запрос и мы должны его отобразить
    if(include_null_result && response.size() == 0)
    {
        Template::row t_row = r;
        for(Template::cell & c : t_row)
            c.value.replace(QRegExp("%sql_result_[0-9]*%"),"-");
        result << t_row;
        return result;
    }

    // Проходимся по response и формируем массив result
    for(int i = 0; i < response.size(); ++i)
    {
        Template::row t_row = r;
        for(int j = 0; j < response[i].size(); ++j)
            for(Template::cell & c : t_row)
            {
                c.value.replace(QString("%sql_result_%1%").arg(j+1), response[i][j].c_str());
                if(c.value == "f") c.value = '0';
                if(c.value == "t") c.value = '1';
            }

        result << t_row;
        if(single_mode)
            return result;
    }
    return result;
}

void ui_mainWindow::log_write(const QString& s, const QColor& color)
{
    this->logEdit->setTextColor(color);
    this->logEdit->append(s);
    this->logEdit->viewport()->update();
    this->logEdit->repaint();
}

QString ui_mainWindow::getActiveTemplateName()
{
    return this->chooseTemplate->currentText();
}

bool ui_mainWindow::checkTagExistsInDB(const QString& tag)
{
    if(!global_station.sql_settings.checkTagExistance)
        return true;
    if(debug_ || !this->sql_obj->connected)
        return true;

    if(this->print_querries->isChecked())
        log_write(QString("Проверяем наличие тега %1 в базе данных").arg(tag), Qt::GlobalColor::gray);

    bool result = false;
    QString query = QString("SELECT * FROM nodes Where nodes.TagName = '%1'").arg(tag);

    pqxx::result res = sql_obj->execute(query);
    result = res.size() > 0;

    if(this->print_querries->isChecked())
        log_write(result ? "Найден!" : "Отсутствует! Пропуск", Qt::GlobalColor::gray);

    return result;
}

void ui_mainWindow::Help()
{
    QFile HelpFile(":/doc/help.pdf");;
    HelpFile.copy(qApp->applicationDirPath().append("/help.pdf"));
    QDesktopServices::openUrl(QUrl::fromLocalFile(qApp->applicationDirPath().append("/help.pdf")));
}
