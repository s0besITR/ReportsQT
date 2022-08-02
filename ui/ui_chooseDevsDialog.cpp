#include "ui_chooseDevsDialog.h"
#include<QtWidgets>

extern Station::xml_station global_station;

#include <QPushButton>
ui_chooseDevsDialog::ui_chooseDevsDialog(QVector<Template::report_data> &data, QWidget *parent)
    : QWidget{parent} , data(data)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::Dialog);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setWindowModality(Qt::WindowModal);
    setFixedSize(800,500);
    setWindowTitle("Выберете устройства, включаемые в отчет");

    QPushButton* okBtn = new QPushButton("ОК");
    okBtn->setMaximumWidth(150);
    connect(okBtn, SIGNAL(clicked()), this, SLOT(btnOkClicked()));

    QPushButton* closeBtn = new QPushButton("Отмена");
    closeBtn->setMaximumWidth(150);
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(close()));

    devs_table = new QTableWidget(data.size(), 2);
    MyHeader* myheader = new MyHeader(Qt::Horizontal, devs_table);
    devs_table->setHorizontalHeader(myheader);
    devs_table->setHorizontalHeaderLabels(QStringList({"Название", "Расположение"}));

    connect(myheader, SIGNAL(check(bool)), this, SLOT(checkAll(bool)));

    devs_table->verticalHeader()->setVisible(false);
    devs_table->setColumnWidth(0, 260);
    devs_table->setColumnWidth(1, 500);
    devs_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    devs_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    devs_table->setSelectionMode(QAbstractItemView::SingleSelection);


    for(int i = 0; i < data.size(); ++i)
    {
        QString tag = data.at(i).tag;
        if(!global_station.objects.contains(tag))
        {
            QTableWidgetItem* pitem = new QTableWidgetItem("ОШИБКА!");
            pitem->setCheckState(Qt::Unchecked);
            devs_table->setItem(i, 0, pitem);
            devs_table->setItem(i, 1, new QTableWidgetItem("Не найден " + tag + " в файле station.xml"));
            continue;
        }

        QTableWidgetItem* pitem = new QTableWidgetItem(global_station.objects[tag].name);
        data.at(i).includeInReport ? pitem->setCheckState(Qt::Checked) : pitem->setCheckState(Qt::Unchecked);
        devs_table->setItem(i, 0, pitem);
        devs_table->setItem(i, 1, new QTableWidgetItem(global_station.objects[tag].location));;
    }

    QHBoxLayout * hb_layout = new QHBoxLayout;
    hb_layout->addWidget(okBtn);
    hb_layout->addWidget(closeBtn);

    QVBoxLayout * vb_layout = new QVBoxLayout;
    vb_layout->addWidget(devs_table);
    vb_layout->addLayout(hb_layout);

    this->setLayout(vb_layout);
}

void ui_chooseDevsDialog::checkAll(bool check)
{
    Qt::CheckState state;
    check ? state = Qt::Checked : state= Qt::Unchecked;
    for(int i = 0; i < devs_table->rowCount(); ++i)
        devs_table->item(i,0)->setCheckState(state);
}

void ui_chooseDevsDialog::btnOkClicked()
{
    for(int i = 0; i < devs_table->rowCount(); ++i)
        if(devs_table->item(i,0)->checkState() == Qt::Checked)
            data[i].includeInReport = true;
         else
            data[i].includeInReport = false;
    close();
}

void MyHeader::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{
  painter->save();
  QHeaderView::paintSection(painter, rect, logicalIndex);
  painter->restore();
  if (logicalIndex == 0)
  {
    QStyleOptionButton option;
    option.rect = QRect(2,3,15,15);
    if (isOn)
      option.state = QStyle::State_On;
    else
      option.state = QStyle::State_Off;
    this->style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, painter);
  }

}
void MyHeader::mousePressEvent(QMouseEvent *event)
{
  if (isOn)
      isOn = false;
  else
      isOn = true;

  emit check(isOn);

  this->update();
  viewport()->update();
  QHeaderView::mousePressEvent(event);
}
