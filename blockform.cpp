#include "blockform.h"
#include "ui_blockform.h"

BlockForm::BlockForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BlockForm)
{
    ui->setupUi(this);

    label_blockScreen = ui->label_blockScreen;
    label_blockScreenText = ui->label_blockScreenText;

    developerMode = false;
}

BlockForm::~BlockForm()
{
    delete ui;
}

void BlockForm::on_pushButton_developerMode_clicked()
{
    developerMode = true;
}
