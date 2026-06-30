#include "fogotform.h"
#include "ui_fogotform.h"

FogotForm::FogotForm(int secretPassword, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FogotForm)
{
    ui->setupUi(this);

    ui->label_code->setText("1" + QStringLiteral("%1").arg(secretPassword, 5, 10, QLatin1Char('0')));
}

FogotForm::~FogotForm()
{
    delete ui;
}

void FogotForm::on_pushButton_clicked()
{
    close();
}
