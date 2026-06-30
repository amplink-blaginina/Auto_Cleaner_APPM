#include "serviceBUConfigElementform.h"
#include "ui_serviceBUConfigElementform.h"

#include "mainwindow.h"

ServiceBUConfigElementForm::ServiceBUConfigElementForm(int boardNum_, SystemConfigure* systemConfigure_, QMap<int, SystemElement*>* systemElements_, bool* BUCPConfigured_, QWidget *parentMain_, QWidget *parent_) :
    QWidget(parent_),
    ui(new Ui::ServiceBUConfigElementForm)
{
    ui->setupUi(this);

    parent = parent_;
    parentMain = parentMain_;

    boardNum = boardNum_;
    systemConfigure = systemConfigure_;
    systemElements = systemElements_;
    BUCPConfigured = BUCPConfigured_;

    boardsString.insert(1, "Дискр. Выход");
    boardsString.insert(2, "Ан. Вход");
    boardsString.insert(3, "Дискр. Вход");
    boardsString.insert(4, "Ан. Выход");
    boardsString.insert(5, "КМ. Вход");
    boardsString.insert(15, "Отсутствует");

    channelsString.append("д. вх.");
    channelsString.append("а. вх. 8");
    channelsString.append("а. вх. 16");
    channelsString.append("сч. вх. 8");
    channelsString.append("сч. вх. 16");
    channelsString.append("шим вх.");
    channelsString.append("д. вых.");
    channelsString.append("шим вых.");
    channelsString.append("чм. вых.");
    channelsString.append("чим вых.");

    ui->label_boardNumber->setText(QString::number(boardNum));

    connect(&mainProgressTimer, SIGNAL(timeout()), this, SLOT(mainProgress()));

    progressBars.append(ui->progressBar_channel1);
    progressBars.append(ui->progressBar_channel2);
    progressBars.append(ui->progressBar_channel3);
    progressBars.append(ui->progressBar_channel4);
    progressBars.append(ui->progressBar_channel5);
    progressBars.append(ui->progressBar_channel6);
    progressBars.append(ui->progressBar_channel7);
    progressBars.append(ui->progressBar_channel8);
    progressBars.append(ui->progressBar_channel9);
    progressBars.append(ui->progressBar_channel10);
    progressBars.append(ui->progressBar_channel11);
    progressBars.append(ui->progressBar_channel12);

    names.append(ui->label_channel1Name);
    names.append(ui->label_channel2Name);
    names.append(ui->label_channel3Name);
    names.append(ui->label_channel4Name);
    names.append(ui->label_channel5Name);
    names.append(ui->label_channel6Name);
    names.append(ui->label_channel7Name);
    names.append(ui->label_channel8Name);
    names.append(ui->label_channel9Name);
    names.append(ui->label_channel10Name);
    names.append(ui->label_channel11Name);
    names.append(ui->label_channel12Name);

    values.append(ui->label_channel1Value);
    values.append(ui->label_channel2Value);
    values.append(ui->label_channel3Value);
    values.append(ui->label_channel4Value);
    values.append(ui->label_channel5Value);
    values.append(ui->label_channel6Value);
    values.append(ui->label_channel7Value);
    values.append(ui->label_channel8Value);
    values.append(ui->label_channel9Value);
    values.append(ui->label_channel10Value);
    values.append(ui->label_channel11Value);
    values.append(ui->label_channel12Value);

    buttonsLess.append(ui->pushButton_channel1Less);
    buttonsLess.append(ui->pushButton_channel2Less);
    buttonsLess.append(ui->pushButton_channel3Less);
    buttonsLess.append(ui->pushButton_channel4Less);
    buttonsLess.append(ui->pushButton_channel5Less);
    buttonsLess.append(ui->pushButton_channel6Less);
    buttonsLess.append(ui->pushButton_channel7Less);
    buttonsLess.append(ui->pushButton_channel8Less);
    buttonsLess.append(ui->pushButton_channel9Less);
    buttonsLess.append(ui->pushButton_channel10Less);
    buttonsLess.append(ui->pushButton_channel11Less);
    buttonsLess.append(ui->pushButton_channel12Less);

    buttonsMore.append(ui->pushButton_channel1More);
    buttonsMore.append(ui->pushButton_channel2More);
    buttonsMore.append(ui->pushButton_channel3More);
    buttonsMore.append(ui->pushButton_channel4More);
    buttonsMore.append(ui->pushButton_channel5More);
    buttonsMore.append(ui->pushButton_channel6More);
    buttonsMore.append(ui->pushButton_channel7More);
    buttonsMore.append(ui->pushButton_channel8More);
    buttonsMore.append(ui->pushButton_channel9More);
    buttonsMore.append(ui->pushButton_channel10More);
    buttonsMore.append(ui->pushButton_channel11More);
    buttonsMore.append(ui->pushButton_channel12More);

    for (int i = 0; i < 12; i++)
    {
        QFont f = names[i]->font();
        f.setPointSize(11);
        names[i]->setFont(f);
        connect(buttonsLess[i], SIGNAL(clicked()), this, SLOT(valuesClicked()));
        connect(buttonsMore[i], SIGNAL(clicked()), this, SLOT(valuesClicked()));
    }

    fillBoard();

    mainProgressTimer.start(100);
}

ServiceBUConfigElementForm::~ServiceBUConfigElementForm()
{
    delete ui;
}

void ServiceBUConfigElementForm::mainProgress()
{
    if (!parent->isVisible())
        return;
    // тут надо получить свой конфиг и отрисовать его на форме
    if (!BUCPConfigured)
    {// неизвестно что на той стороне
        ui->label_boardTypeValue->setText("Nan");
    }
    else
    {// что то есть - надо прознать
        // ведем подсчет долгих нажатий
        for (int i = 0; i < 12; i++)
        {
            if (values.at(i)->property("pressed").toBool())
                values.at(i)->setProperty("pressedCounter", values.at(i)->property("pressedCounter").toUInt() + 1);
        }

        QString boardType = boardsString[getBoardType()];
        QString boardStatus = "";

        QString style_sheet = "";
        bool board_ok = false;
        if (isConfigured())
        {// сконфижена
            boardStatus += " (сконфигур.)";
            style_sheet = "QFrame#frame_board{background-color: rgb(70,248,114);}";
            board_ok = true;
        }
        if (getBoardType() != BOARD_UNKNOWN && systemConfigure->boardsType[boardNum] != getBoardType() && systemConfigure->boardsType[boardNum] != BOARD_UNKNOWN)
        {
            boardStatus += " (непр. тип)";
            style_sheet = "QFrame#frame_board{background-color: rgb(248,233,70);}";
        }
        if (getBoardType() == BOARD_UNKNOWN && systemConfigure->boardsType[boardNum] != BOARD_UNKNOWN)
        {
            boardStatus += " (нет платы)";
            style_sheet = "QFrame#frame_board{background-color: rgb(248,70,73);}";
        }
        if (!isConfigured() && getBoardType() != BOARD_UNKNOWN && systemConfigure->boardsType[boardNum] == getBoardType() && systemConfigure->boardsType[boardNum] != BOARD_UNKNOWN)
        {
            boardStatus += " (ош. конф.)";
            style_sheet = "QFrame#frame_board{background-color: rgb(248,70,73);}";
        }
        if (ui->frame_board->styleSheet() != style_sheet)
            ui->frame_board->setStyleSheet(style_sheet);
        if (ui->label_boardTypeValue->text() != boardType)
            ui->label_boardTypeValue->setText(boardType);
        if (ui->label_boardStatusValue->text() != boardStatus)
            ui->label_boardStatusValue->setText(boardStatus);

        // получим данные о каналах

        for (int i = 0; i < 12; i++)
        {
            if (systemConfigure->boardsType[boardNum] != BOARD_UNKNOWN)
            {
                int val = ((MainWindow*)parentMain)->can0->getOriginalState(boardNum, i).toInt();
                if ((systemConfigure->channelsType[boardNum][i] == IN_MODE_ANALOG_16 || systemConfigure->channelsType[boardNum][i] == IN_MODE_EXTI_16)
                        && progressBars[i]->maximum() != 65535)
                    progressBars[i]->setMaximum(65535);
                if (systemConfigure->channelsType[boardNum][i] == OUT_MODE_NORMAL || systemConfigure->channelsType[boardNum][i] == IN_MODE_NORMAL)
                {
                    if (values[i]->text() != (val?"ВКЛ":"ВЫКЛ"))
                    {
                        values[i]->setText(val?"ВКЛ":"ВЫКЛ");
                        progressBars[i]->setValue(val?255:0);
                    }
                }
                else
                {
                    if (values[i]->text() != QString::number(val))
                    {
                        values[i]->setText(QString::number(val));
                        progressBars[i]->setValue(val);
                        progressBars[i]->repaint();
                    }
                }
            }
            if (!board_ok)
            {
                if (!progressBars[i]->styleSheet().contains("196,196,196"))
                {
                    progressBars[i]->setStyleSheet("QProgressBar\
                                                   {\
                                                       background-color: rgb(196,196,196);\
                                                       border: 1px solid black;\
                                                   }\
                                                   \
                                                   QProgressBar::chunk {\
                                                       background-color: rgb(70,248,114);\
                                                       margin: 0px;\
                                                       width: 10px;\
                                                       border-bottom-right-radius: 10px;\
                                                       border-bottom-left-radius: 10px;\
                                                   }\
                                                   ");
                }
            }
            else
            {
                if (!progressBars[i]->styleSheet().contains("248,70,73"))
                {
                    progressBars[i]->setStyleSheet("QProgressBar\
                                                   {\
                                                       background-color: rgb(248,70,73);\
                                                       border: 1px solid black;\
                                                   }\
                                                   \
                                                   QProgressBar::chunk {\
                                                       background-color: rgb(70,248,114);\
                                                       margin: 0px;\
                                                       width: 10px;\
                                                       border-bottom-right-radius: 10px;\
                                                       border-bottom-left-radius: 10px;\
                                                   }\
                                                   ");
                }
            }
        }
    }
}

int ServiceBUConfigElementForm::getBoardType()
{
    DeviceStates ds;
    switch (boardNum)
    {
        case 1: ds = Board1Type;break;
        case 2: ds = Board2Type;break;
        case 3: ds = Board3Type;break;
        case 4: ds = Board4Type;break;
        case 5: ds = Board5Type;break;
        case 6: ds = Board6Type;break;
        case 7: ds = Board7Type;break;
        case 8: ds = Board8Type;break;
    }
    int ret = ((MainWindow*)parentMain)->can0->getState(ds).toInt();
    if (ret == 0)
        ret = BOARD_UNKNOWN;
    return ret;
}

bool ServiceBUConfigElementForm::isConfigured()
{
    DeviceStates ds;
    switch (boardNum)
    {
        case 1: ds = Board1Configured;break;
        case 2: ds = Board2Configured;break;
        case 3: ds = Board3Configured;break;
        case 4: ds = Board4Configured;break;
        case 5: ds = Board5Configured;break;
        case 6: ds = Board6Configured;break;
        case 7: ds = Board7Configured;break;
        case 8: ds = Board8Configured;break;
    }
    return ((MainWindow*)parentMain)->can0->getState(ds).toBool();
}

void ServiceBUConfigElementForm::fillBoard()
{// выставляет текущие параметры платы (желаемый тип, типы каналов)
    ui->label_boardTypeNeedValue->setText(boardsString[systemConfigure->boardsType[boardNum]]);

    for (int i = 0; i < 12; i++)
    {
        if (systemElements->contains(systemConfigure->id[boardNum][i]))
            names.at(i)->setText("(" + channelsString[systemConfigure->channelsType[boardNum][i]] + ") " + systemElements->value(systemConfigure->id[boardNum][i])->name + ":");
        else
            names.at(i)->setText("(" + channelsString[systemConfigure->channelsType[boardNum][i]] + ") Канал " + QString::number(i + 1) + ":");
        progressBars[i]->setValue(0);
        values[i]->setText("NaN");
    }
}

void ServiceBUConfigElementForm::valuesClicked()
{
    QPushButton* obj = (QPushButton*)sender();
    for (int i = 0; i < 12; i++)
    {
        if (systemConfigure->channelsType[boardNum][i] == OUT_MODE_NORMAL)
        {
            if (buttonsLess.at(i) == obj || buttonsMore.at(i) == obj)
            {
                if (((MainWindow*)parentMain)->can0->getOriginalState(boardNum, i).toInt())
                    ((MainWindow*)parentMain)->can0->setOriginalState(boardNum, i, 0);
                else
                    ((MainWindow*)parentMain)->can0->setOriginalState(boardNum, i, 1);
                break;
            }
        }
        if (systemConfigure->channelsType[boardNum][i] == OUT_MODE_FC || systemConfigure->channelsType[boardNum][i] == OUT_MODE_PFM || systemConfigure->channelsType[boardNum][i] == OUT_MODE_PWM)
        {
            if (buttonsLess.at(i) == obj)
            {
                if (((MainWindow*)parentMain)->can0->getOriginalState(boardNum, i).toInt() >= 10)
                    ((MainWindow*)parentMain)->can0->setOriginalState(boardNum, i, ((MainWindow*)parentMain)->can0->getOriginalState(boardNum, i).toInt() - 10);
                else
                    ((MainWindow*)parentMain)->can0->setOriginalState(boardNum, i, 0);
                break;
            }
            if (buttonsMore.at(i) == obj)
            {
                if (((MainWindow*)parentMain)->can0->getOriginalState(boardNum, i).toInt() <= 245)
                    ((MainWindow*)parentMain)->can0->setOriginalState(boardNum, i, ((MainWindow*)parentMain)->can0->getOriginalState(boardNum, i).toInt() + 10);
                else
                    ((MainWindow*)parentMain)->can0->setOriginalState(boardNum, i, 255);
                break;
            }

        }
    }
}

