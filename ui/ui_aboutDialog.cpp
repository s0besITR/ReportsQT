#include "ui_aboutDialog.h"
#include <QPushButton>
#include <QBoxLayout>
#include <QLabel>
#include <QPixmap>

ui_AboutDialog::ui_AboutDialog(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::Dialog);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setWindowModality(Qt::WindowModal);
    setFixedSize(350,200);
    setWindowTitle("О программе");

    QPushButton* okBtn = new QPushButton("ОК");
    okBtn->setMaximumWidth(150);
    connect(okBtn, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *hb_layout = new QHBoxLayout;
    QHBoxLayout *hb_layout_btn = new QHBoxLayout;
    QVBoxLayout *vb_layout = new QVBoxLayout;

    QLabel* icon = new QLabel;
    icon->setPixmap(QPixmap(":/icon.png").scaled(80,80));

    QString build_dt =  QStringLiteral(__DATE__) + QStringLiteral(" ") + QStringLiteral(__TIME__);

    QLabel* text = new QLabel("<b>Продукт: </b>Отчеты для Alpha.Server<br>"
                              "<b>Разработал: </b>Собецкий Александр<br>"
                              "<b>Дата сборки: </b>" + build_dt + "<br><br>"
                              "(c) 2023 ООО ИТР<br>"
                              "<a href=\"mailto:sobetsky.alexander@gmail.com\">sobetsky.alexander@gmail.com</a>");

    hb_layout->addWidget(icon);
    hb_layout->addWidget(text);
    hb_layout->addStretch(1);

    hb_layout_btn->addWidget(okBtn);

    vb_layout->addLayout(hb_layout);
    vb_layout->addLayout(hb_layout_btn);

    this->setLayout(vb_layout);
}
