#pragma once

#include <QWidget>
#include <QMap>
class QGroupBox;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QTimeEdit;

class ui_connectionDialog : public QWidget
{
    Q_OBJECT
public:
    explicit ui_connectionDialog(QWidget *parent = nullptr);

signals:

private slots:
    void checkConnection();
    void btnOkClicked();
    void btnEscapeClicked();
    void checkStateListenner(int state);
    void negativeBtnClicked();

private:
    QGroupBox* createTopFrame();
    QGroupBox* createBottomFrame();
    void initElements();

    QMap<QString, QLineEdit*> mapEdit;
    QMap<QString, QCheckBox*> mapCheckBox;
    QPushButton* connect_btn;
    QTimeEdit* utc_time;
    QPushButton* negative_time;
    QLineEdit* server_status_tag;
};
