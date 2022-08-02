#pragma once

#include <QWidget>
#include "xml_io.h"

class ui_chooseDevsDialog : public QWidget
{
    Q_OBJECT
public:
    explicit ui_chooseDevsDialog(QVector<Template::report_data> &data, QWidget *parent = nullptr);
private slots:
    void checkAll(bool check);
    void btnOkClicked();
signals:

private:
    QTableWidget *devs_table;
    QVector<Template::report_data> &data;
};

//Для добавления виджета чекбокс мы переписываем класс headerview
class MyHeader : public QHeaderView
{
    Q_OBJECT
public:
  MyHeader(Qt::Orientation orientation, QWidget * parent = nullptr) : QHeaderView(orientation, parent), isOn(true)
  {}

protected:
  void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const;
  void mousePressEvent(QMouseEvent *event);

private:
  bool isOn;

signals:
  void check(bool check);

};
