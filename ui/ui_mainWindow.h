#pragma once

#include <QMainWindow>
#include "ui_connectionDialog.h"
#include "ui_aboutDialog.h"
#include "ui_chooseDevsDialog.h"
#include <xml_io.h>
#include "doc_io.h"
#include "sql_io.h"

class QGroupBox;
class QComboBox;
class QDateEdit;
class QTimeEdit;
class QPushButton;

extern QMap<QString,Template::xml_template> global_templates;
extern Station::xml_station global_station;

struct cmd_arguments{
    bool minimize = false;
    bool foldable = true;
    bool modal = false;

    int monitor = 1;
    int close_after  = -1;

    QString tag = "";
    QString template_name = "";
    QString time = "";
};

// Переопределяем QTextEdit, что бы добавить контекстное меню "Clear" для удаления всех логов
class MyTextEdit : public QTextEdit {
    Q_OBJECT
public:
    explicit MyTextEdit(QWidget *parent = nullptr);
    explicit MyTextEdit(const QString &text, QWidget *parent = nullptr);
    void contextMenuEvent(QContextMenuEvent *event);
private slots:
    void clearLog();
private:
    QAction* clearAction;
};

class ui_mainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ui_mainWindow(cmd_arguments args, QWidget *parent = nullptr);
    void log_write(const QString& s, const QColor& color = Qt::black);
    QString getActiveTemplateName();
    sql_io * sql_obj;    
    cmd_arguments cmd_args;
    static bool load_configuration();

public slots:
    void makeReport();
private slots:
    void templateChanged();
    void chooseDevicesDlgOpen();
    void stopReport();
    void slotCustomMenuRequested(QPoint pos);
    void openFileFromExplorer(const QModelIndex& index);
    void deleteAllReports();
    void deleteSelectedReport();
    void EditConnectionCfg();
    void About();
    void Help();
    void ReloadConfiguration();    

private:
    bool eventFilter(QObject *obj, QEvent *event);
    void updateReportTimestamp();
    void createActions();
    void createMenus();
    void replaceCtrlStrings(Template::xml_template & t);
    void replaceCtrlStrings(Template::row & r,QString& sql_querry, const Template::tag & low_tag, const QString & data_tag);
    int countQueries();

    QVector<Template::row> get_sql_data(const Template::row & r, const QString & querry, bool single_mode, bool include_null_result);
    void printResponse(const pqxx::result & response);
    bool checkTagExistsInDB(const QString& tag);

    QGroupBox* createFrameReports();
    QTabWidget* createFrameExplorer();

    QMenu* connectionMenu;
    QMenu* helpMenu;
    QMenu* contextMenu;
    QAction* showConnectionDlg;
    QAction* showAboutDlg;
    QAction* activateDebug;
    QAction* reload;
    QAction* help;

    QTreeView        *treeView_oper;
    QTreeView        *treeView_hist;
    QTabWidget* tabs;

    QComboBox* chooseTemplate;
    QDateEdit* date_from;
    QDateEdit* date_to;
    QTimeEdit* time_from;
    QTimeEdit* time_to;
    QPushButton* doReport;

    MyTextEdit* logEdit;
    QCheckBox* print_querries;

    QProgressBar *pProgressBar;
    QPushButton* btn_stopReport;

    bool debug_;
    bool stop_clicked;    
signals:

};
