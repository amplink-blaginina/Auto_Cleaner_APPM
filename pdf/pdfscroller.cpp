#include <QFile>
#include <QDir>
#include <QMessageBox>

#include "pdfscroller.h"

//QString defaultPdfPath = "/opt/Auto_Cleaner_New/bin/";
//QString defaultImportPath = "/opt/Auto_Cleaner_New/bin/import/";
QString defaultPdfPath = "/home/knight/Auto_Cleaner/";
QString defaultImportPath = "/home/knight/Auto_Cleaner/import/";


PdfScroller::PdfScroller(QWidget *parent_) : QObject(parent_)
{
    parent = parent_;

    pdfElement = new PdfElement(this);
    pdfThread = new QThread();
    pdfElement->moveToThread(pdfThread);
    connect(pdfThread, SIGNAL(started()), pdfElement, SLOT(threadStarted()));//, Qt::DirectConnection
    connect(pdfElement, SIGNAL(destroyed(QObject*)), pdfThread, SLOT(quit()));
    connect(pdfThread, SIGNAL(finished()), pdfThread, SLOT(deleteLater()));
    pdfThread->start();
    connect(pdfElement, SIGNAL(openFileReady(int)), this, SLOT(openFileReady(int)), Qt::QueuedConnection);
    connect(pdfElement, SIGNAL(renderPageReady(QImage)), this, SLOT(renderPageReady(QImage)), Qt::QueuedConnection);
    connect(this, SIGNAL(openFile(QString)), pdfElement, SLOT(openFile(QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(renderPage(int, double)), pdfElement, SLOT(renderPage(int, double)), Qt::QueuedConnection);

    // choose pdf file dialog
    choosePdfHboxLayout = new QHBoxLayout();
    choosePdfVboxLayout = new QVBoxLayout();

    choosePdfScrollArea = new QScrollArea;
    choosePdfScrollAreaWidget = new QWidget;
    choosePdfButtonOpen = new QPushButton();
    choosePdfButtonOpen->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/pdf/button_open.png);");
    choosePdfButtonOpen->setMinimumSize(81, 81);
    choosePdfButtonOpen->setMaximumSize(81, 81);
    choosePdfButtonDelete = new QPushButton();
    choosePdfButtonDelete->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/pdf/button_delete.png);");
    choosePdfButtonDelete->setMinimumSize(81, 81);
    choosePdfButtonDelete->setMaximumSize(81, 81);
    choosePdfButtonImport = new QPushButton();
    choosePdfButtonImport->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/pdf/button_import.png);");
    choosePdfButtonImport->setMinimumSize(81, 81);
    choosePdfButtonImport->setMaximumSize(81, 81);

    choosePdfGrid = new QGridLayout();

    choosePdfButtonExit = new QPushButton();
    choosePdfButtonExit->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/pdf/button_exit.png);");
    choosePdfButtonExit->setMinimumSize(83, 83);
    choosePdfButtonExit->setMaximumSize(83, 83);

    choosePdfHboxLayout->setGeometry(QRect(0, 0, 1024, 600));

    choosePdfScrollArea->setBackgroundRole(QPalette::Dark);
    choosePdfScrollArea->setWidget(choosePdfScrollAreaWidget);
    choosePdfScrollAreaWidget->setLayout(choosePdfGrid);

    choosePdfHboxLayout->addWidget(choosePdfScrollArea);
    choosePdfHboxLayout->addLayout(choosePdfVboxLayout);

    choosePdfVboxLayout->addStretch(1);
    choosePdfVboxLayout->addWidget(choosePdfButtonOpen);
    choosePdfVboxLayout->addWidget(choosePdfButtonDelete);
    choosePdfVboxLayout->addStretch(1);
    choosePdfVboxLayout->addWidget(choosePdfButtonImport);
    choosePdfVboxLayout->addStretch(1);
    choosePdfVboxLayout->addWidget(choosePdfButtonExit);
    QScroller::grabGesture(choosePdfScrollArea->viewport(), QScroller::LeftMouseButtonGesture);
    pushbuttonOpenEffect = new QGraphicsOpacityEffect(this);
    pushbuttonDeleteEffect = new QGraphicsOpacityEffect(this);
    pushbuttonImportEffect = new QGraphicsOpacityEffect(this);
    choosePdfButtonDelete->setGraphicsEffect(pushbuttonDeleteEffect);
    choosePdfButtonOpen->setGraphicsEffect(pushbuttonOpenEffect);
    choosePdfButtonImport->setGraphicsEffect(pushbuttonImportEffect);
    pushbuttonDeleteEffect->setOpacity(0.5);
    pushbuttonOpenEffect->setOpacity(0.5);
    pushbuttonImportEffect->setOpacity(0.5);
    choosePdfButtonDelete->setEnabled(false);
    choosePdfButtonOpen->setEnabled(false);
    choosePdfButtonImport->setEnabled(false);

    fillFilesGrid();

    // main window with pdf
    hbox_layout = new QHBoxLayout();
    vbox_layout = new QVBoxLayout();

    imageLabel = new QLabel;

    scrollArea = new QScrollArea;

    buttonsPage = new QWidget();
    buttonsPage->setObjectName("buttonsPage");
    buttonsPage->setStyleSheet("QWidget#buttonsPage{background-image: url(:/Images/Images/pdf/buttons_page.png);}");
    buttonsPage->setMinimumSize(70, 158);
    buttonsPage->setMaximumSize(70, 158);
    //buttonsPage->setGeometry(0, 0, 70, 158);
    buttonUp = new QPushButton("", buttonsPage);
    buttonUp->setGeometry(0, 0, 70, 158 / 2);
    buttonUp->setStyleSheet("border-style:none;outline: none;");
    buttonDown = new QPushButton("", buttonsPage);
    buttonDown->setGeometry(0, 158 / 2, 70, 158 / 2);
    buttonDown->setStyleSheet("border-style:none;outline: none;");
    buttonsZoom = new QWidget();
    buttonsZoom->setObjectName("buttonsZoom");
    buttonsZoom->setStyleSheet("QWidget#buttonsZoom{background-image: url(:/Images/Images/pdf/buttons_zoom.png);}");
    buttonsZoom->setMinimumSize(70, 158);
    buttonsZoom->setMaximumSize(70, 158);
    buttonZoomIn = new QPushButton("", buttonsZoom);
    buttonZoomIn->setGeometry(0, 0, 70, 158 / 2);
    buttonZoomIn->setStyleSheet("border-style:none;outline: none;");
    buttonZoomOut = new QPushButton("", buttonsZoom);
    buttonZoomOut->setGeometry(0, 158 / 2, 70, 158 / 2);
    buttonZoomOut->setStyleSheet("border-style:none;outline: none;");

    buttonExit = new QPushButton();
    buttonExit->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/pdf/button_exit.png);");
    buttonExit->setMinimumSize(83, 83);
    buttonExit->setMaximumSize(83, 83);

    buttonPage = new QPushButton();
    buttonPage->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/pdf/button_page_off.png);");
    buttonPage->setMinimumSize(81, 81);
    buttonPage->setMaximumSize(81, 81);
    labelPage = new QLabel("1/1");
    labelZoom = new QLabel("1x");

    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);

    hbox_layout->setGeometry(QRect(0, 0, 1024, 600));
    hbox_layout->addWidget(scrollArea);
    hbox_layout->addLayout(vbox_layout);
    currentPage = 0;
    maximalPage = 0;
    currentScale = 1.0;
    vbox_layout->addStretch(1);
    vbox_layout->addWidget(labelPage);
    vbox_layout->setAlignment(labelPage, Qt::AlignCenter);
    vbox_layout->addWidget(buttonPage);
    vbox_layout->addWidget(buttonsPage);
    vbox_layout->setAlignment(buttonsPage, Qt::AlignCenter);
    //vbox_layout->addWidget(buttonUp);
    //vbox_layout->addWidget(buttonDown);
    vbox_layout->addStretch(1);
    vbox_layout->addWidget(labelZoom);
    vbox_layout->setAlignment(labelZoom, Qt::AlignCenter);
    vbox_layout->addWidget(buttonsZoom);
    vbox_layout->setAlignment(buttonsZoom, Qt::AlignCenter);
    //vbox_layout->addWidget(buttonZoomIn);
    //vbox_layout->addWidget(buttonZoomOut);
    vbox_layout->addStretch(1);
    vbox_layout->addWidget(buttonExit);
    QScroller::grabGesture(scrollArea->viewport(), QScroller::LeftMouseButtonGesture);

    // error page dialog
    errorPageWidget = new QWidget(NULL);
    errorPageWidget->setGeometry(250, 300, 400, 181);
    errorPageWidget->setObjectName("errorPageWidget");
    errorPageText = new QLabel("Ошибка", errorPageWidget);
    errorPageText->setGeometry(20, 20, 300, 100);
    errorPageOk = new QPushButton("OK", errorPageWidget);
    errorPageOk->setGeometry(160, 130, 80, 40);
    errorPageWidget->hide();

    // choose page dialog
    choosePageWidget = new QWidget(NULL);
    choosePageWidget->setGeometry(10, 400, 869, 181);
    choosePageWidget->setObjectName("choosePageWidget");
    choosePageWidget->setStyleSheet("QWidget#choosePageWidget{border: 2px solid; background-image: url(:/Images/Images/pdf/slider_background.png);}");
    //choosePageLayout = new QVBoxLayout(choosePageWidget);
    //choosePageTitle = new QLabel("Выбор страницы");
    //choosePageLayout->addWidget(choosePageTitle);
    choosePageText = new QLabel("Страница 1/1", choosePageWidget);
    choosePageText->setGeometry(350, 15, 200, 30);
    //choosePageLayout->addWidget(choosePageText);
    choosePageSlider = new QSlider(Qt::Horizontal, choosePageWidget);
    choosePageSlider->setStyleSheet(
                   "QSlider{min-height: 75px;max-height: 75px;height:75px;}QSlider::groove:horizontal {background-color: transparent; border: 0px solid;height: 5px;margin: 0 35px;}QSlider::handle:horizontal {"
                   "image : url(:/Images/Images/pdf/slider_element.png); margin: -50px -35px;width 70px;         height 70px;border: 5px solid transparent;"
                   "}"    );
    choosePageSlider->setGeometry(73 + 74, 70, 725 - 74 * 2, 50);
    //choosePageLayout->addWidget(choosePageSlider);
    choosePageOk = new QPushButton("", choosePageWidget);
    choosePageOk->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/pdf/button_ok.png);");
    choosePageOk->setGeometry(75, 70, 74, 74);
    //choosePageLayout->addWidget(choosePageOk);
    choosePageCancel = new QPushButton("", choosePageWidget);
    choosePageCancel->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/pdf/button_cancel.png);");
    choosePageCancel->setGeometry(723, 70, 74, 74);
    //choosePageLayout->addWidget(choosePageCancel);
    choosePageWidget->hide();

    // ожидания гифка
    progressWidget = new ProgressWidget(scrollArea);

    connect(errorPageOk, SIGNAL(clicked()), this, SLOT(errorPageClicked()));
    connect(buttonPage, SIGNAL(clicked()), this, SLOT(buttonPageClicked()));
    connect(buttonExit, SIGNAL(clicked()), this, SLOT(buttonExitClicked()));
    connect(choosePageSlider, SIGNAL(valueChanged(int)), this, SLOT(choosePageSliderChanged()));
    connect(choosePageOk, SIGNAL(clicked()), this, SLOT(choosePageOkClicked()));
    connect(choosePageCancel, SIGNAL(clicked()), this, SLOT(choosePageCancelClicked()));
    connect(buttonUp, SIGNAL(clicked()), this, SLOT(buttonUpClicked()));
    connect(buttonDown, SIGNAL(clicked()), this, SLOT(buttonDownClicked()));
    connect(buttonZoomIn, SIGNAL(clicked()), this, SLOT(buttonZoomInClicked()));
    connect(buttonZoomOut, SIGNAL(clicked()), this, SLOT(buttonZoomOutClicked()));
    connect(choosePdfButtonOpen, SIGNAL(clicked()), this, SLOT(choosePdfButtonOpenClicked()));
    connect(choosePdfButtonDelete, SIGNAL(clicked()), this, SLOT(choosePdfButtonDeleteClicked()));
    connect(choosePdfButtonImport, SIGNAL(clicked()), this, SLOT(choosePdfButtonImportClicked()));
    connect(choosePdfButtonExit, SIGNAL(clicked()), this, SLOT(choosePdfButtonExitClicked()));

    connect(&buttonsTimer, SIGNAL(timeout()), this, SLOT(buttonsOff()));
}

PdfScroller::~PdfScroller()
{
    disconnect(errorPageOk, SIGNAL(clicked()), this, SLOT(errorPageClicked()));
    disconnect(choosePdfButtonOpen, SIGNAL(clicked()), this, SLOT(choosePdfButtonOpenClicked()));
    disconnect(choosePdfButtonDelete, SIGNAL(clicked()), this, SLOT(choosePdfButtonDeleteClicked()));
    disconnect(choosePdfButtonImport, SIGNAL(clicked()), this, SLOT(choosePdfButtonImportClicked()));
    disconnect(choosePdfButtonExit, SIGNAL(clicked()), this, SLOT(choosePdfButtonExitClicked()));
    disconnect(buttonPage, SIGNAL(clicked()), this, SLOT(buttonPageClicked()));
    disconnect(buttonExit, SIGNAL(clicked()), this, SLOT(buttonExitClicked()));
    disconnect(choosePageSlider, SIGNAL(valueChanged(int)), this, SLOT(choosePageSliderChanged()));
    disconnect(choosePageOk, SIGNAL(clicked()), this, SLOT(choosePageOkClicked()));
    disconnect(choosePageCancel, SIGNAL(clicked()), this, SLOT(choosePageCancelClicked()));
    disconnect(buttonUp, SIGNAL(clicked()), this, SLOT(buttonUpClicked()));
    disconnect(buttonDown, SIGNAL(clicked()), this, SLOT(buttonDownClicked()));
    disconnect(buttonZoomIn, SIGNAL(clicked()), this, SLOT(buttonZoomInClicked()));
    disconnect(buttonZoomOut, SIGNAL(clicked()), this, SLOT(buttonZoomOutClicked()));
    buttonsTimer.stop();
    buttonsTimer.disconnect();
    //renderTimer.stop();

    delete progressWidget;

    pdfElement->deleteLater();

    delete pushbuttonOpenEffect;
    delete pushbuttonDeleteEffect;
    delete pushbuttonImportEffect;
    delete choosePdfButtonOpen;
    delete choosePdfButtonDelete;
    delete choosePdfButtonImport;
    delete choosePdfButtonExit;
    QLayoutItem* item;
    while ( ( item = choosePdfGrid->takeAt( 0 ) ) != NULL )
    {
        item->widget()->setParent(NULL);
//        delete item;
    }
    while (choosePdfFileWidgets.size() > 0)
    {
        disconnect(choosePdfFileWidgets.first()->button, SIGNAL(clicked()), this, SLOT(clicked()));
        delete choosePdfFileWidgets.takeFirst();
    }
    delete choosePdfGrid;
    delete choosePdfScrollAreaWidget;
    delete choosePdfScrollArea;
    if (choosePdfHboxLayout)
    {
        delete choosePdfHboxLayout;
        //delete choosePdfVboxLayout;
    }

    //delete choosePageTitle;
    delete choosePageSlider;
    delete choosePageText;
    delete choosePageOk;
    delete choosePageCancel;
    //delete choosePageLayout;
    delete choosePageWidget;

    delete errorPageOk;
    delete errorPageText;
    delete errorPageWidget;

    delete imageLabel;
    delete labelZoom;
    delete buttonPage;
    delete buttonExit;
    delete buttonsPage;
    delete labelPage;
    //delete buttonUp;
    //delete buttonDown;
    delete buttonsZoom;
    //delete buttonZoomIn;
    //delete buttonZoomOut;
    delete scrollArea;
    delete vbox_layout;
    delete hbox_layout;

    //parent->deleteLater();
}








PdfElement::PdfElement(PdfScroller* parent_): QObject(NULL)
{
    document = NULL;
    parent = parent_;
}

PdfElement::~PdfElement()
{
    if (document)
        delete document;
}

void PdfElement::threadStarted()
{

}

bool PdfElement::openFile(QString path)
{
    document = Poppler::Document::load(path);

    // Paranoid safety check
    if (document == 0)
    {
        parent->errorPageText->setText("Не могу открыть файл");
        parent->errorPageWidget->show();
        // ... error message ...
        return false;
    }
    if (document->isLocked())
    {
        parent->errorPageText->setText("Не могу открыть файл. Файл заблокирован");
        parent->errorPageWidget->show();
        // ... error message ....
        delete document;
        document = NULL;
        return false;
    }

    // Access page of the PDF file
    Poppler::Page* pdfPage = document->page(0);  // Document starts at page 0
    if (pdfPage == 0)
    {
        parent->errorPageText->setText("Не могу прочитать страницы");
        parent->errorPageWidget->show();
        // ... error message ...
        delete document;
        document = NULL;
        return false;
    }
    delete pdfPage;
    emit openFileReady(document->numPages());

    return true;
}

bool PdfElement::renderPage(int currentPage, double currentScale)
{
    // Access page of the PDF file
    Poppler::Page* pdfPage = document->page(currentPage);  // Document starts at page 0
    if (pdfPage == 0) {
      // ... error message ...
      return false;
    }
    // Generate a QImage of the rendered page
    QImage image = pdfPage->renderToImage(72 * currentScale, 72 * currentScale, -1, -1, -1, -1);
    if (image.isNull()) {
      // ... error message ...
      return false;
    }
    delete pdfPage;
    emit renderPageReady(image);

    return true;
}










ProgressWidget::ProgressWidget(QWidget *parent_): QObject(NULL)
{
    parent = parent_;
    progressLabel = new QLabel(parent);
    progressLabel->setStyleSheet("border-style:none;outline: none;");
    progressCurrentPicture = 0;
    progressLabel->setMinimumSize(128, 128);
    progressLabel->setMaximumSize(128, 128);
    progressLabel->setScaledContents(true);
    progressLabel->raise();
    progressLabel->hide();
    progressTimer = new QTimer();
    connect(progressTimer, SIGNAL(timeout()), this, SLOT(showProgress()));
}

ProgressWidget::~ProgressWidget()
{
    progressTimer->stop();
    progressTimer->disconnect();
    delete progressTimer;
    delete progressLabel;
}

void ProgressWidget::startProgress()
{
    progressLabel->show();
    progressTimer->start(100);
    progressLabel->setGeometry(parent->geometry().width() / 2 - 64, parent->geometry().height() / 2 - 64, 128, 128);
}

void ProgressWidget::stopProgress()
{
    progressLabel->hide();
    progressTimer->stop();
}

void ProgressWidget::showProgress()
{
    progressCurrentPicture++;
    if (progressCurrentPicture > 11)
        progressCurrentPicture = 0;
    progressLabel->setPixmap(QPixmap(":/Images/Images/pdf/wait_" + QString::number(progressCurrentPicture + 1) + ".png"));
}









void PdfScroller::openFileReady(int numPages)
{
    maximalPage = numPages;
    choosePageSlider->setRange(1, maximalPage);
    choosePageSliderChanged();
    setPage(0);
    setScale(1.0);
    // starts with 0
    parent->setLayout(hbox_layout);
    progressWidget->startProgress();
    emit renderPage(0, 1);
}

void PdfScroller::renderPageReady(QImage image)
{
    progressWidget->stopProgress();
    imageLabel->setPixmap(QPixmap::fromImage(image));
    imageLabel->setGeometry(0, 0, image.width(), image.height());
}

void PdfScroller::setPaths(QString pdfPath, QString importPath)
{
    defaultImportPath = importPath;
    defaultPdfPath = pdfPath;
}

void PdfScroller::buttonsOff()
{
    buttonsPage->setStyleSheet("QWidget#buttonsPage{background-image: url(:/Images/Images/pdf/buttons_page.png);}");
    buttonsZoom->setStyleSheet("QWidget#buttonsZoom{background-image: url(:/Images/Images/pdf/buttons_zoom.png);}");
    buttonsTimer.stop();
}

void PdfScroller::choosePdfButtonOpenClicked()
{
    for (int i = 0; i < choosePdfFileWidgets.size(); i++)
    {
        if (choosePdfFileWidgets.at(i)->image->property("status").toUInt() == 1)
        {
            choosePdfHboxLayout->removeWidget(parent);
            delete choosePdfHboxLayout;
            choosePdfHboxLayout = NULL;
            choosePdfScrollAreaWidget->hide();
            choosePdfScrollArea->hide();
            choosePdfButtonOpen->hide();
            choosePdfButtonDelete->hide();
            choosePdfButtonImport->hide();
            choosePdfButtonExit->hide();
            parent->setLayout(hbox_layout);
            emit openFile(choosePdfFileWidgets.at(i)->path);
            break;
        }
    }

}

void PdfScroller::choosePdfButtonDeleteClicked()
{
    int reply;
    reply = QMessageBox::question(NULL, "Удаление файла", "Удалить файл?", "Да", "Нет", "", 0, -1);
    if (reply == 1)
        return;
    for (int i = 0; i < choosePdfFileWidgets.size(); i++)
    {
        if (choosePdfFileWidgets.at(i)->image->property("status").toUInt() == 1)
        {
            QFile::remove(choosePdfFileWidgets.at(i)->path);
            delete choosePdfFileWidgets.takeAt(i);
        }
    }
    pushbuttonDeleteEffect->setOpacity(0.5);
    pushbuttonOpenEffect->setOpacity(0.5);
    choosePdfButtonDelete->setEnabled(false);
    choosePdfButtonOpen->setEnabled(false);
    fillFilesGrid();
}

void PdfScroller::choosePdfButtonImportClicked()
{// копируем все pdf из одной директории в другую
    QDir dir(defaultImportPath);
    QStringList allFiles = dir.entryList( QStringList()<<"*.pdf", QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::Files, QDir::Name);
    for (int i = 0; i < allFiles.count(); i++)
    {
        QFile::copy(defaultImportPath + "/" + allFiles.at(i), defaultPdfPath + "/" + allFiles.at(i));
    }
    pushbuttonDeleteEffect->setOpacity(0.5);
    pushbuttonOpenEffect->setOpacity(0.5);
    choosePdfButtonDelete->setEnabled(false);
    choosePdfButtonOpen->setEnabled(false);
    fillFilesGrid();
}

void PdfScroller::choosePdfButtonExitClicked()
{
    deleteLater();
    parent->deleteLater();
}

void PdfScroller::fillFilesGrid()
{
    QLayoutItem* item;
    while ( ( item = choosePdfGrid->takeAt( 0 ) ) != NULL )
    {
        item->widget()->setParent(NULL);
        //delete item;
    }
    while (choosePdfFileWidgets.size() > 0)
        delete choosePdfFileWidgets.takeFirst();
    QDir dir(defaultPdfPath);
    QStringList allFiles = dir.entryList( QStringList()<<"*.pdf", QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::Files, QDir::Name);
    for (int i = 0; i < allFiles.count(); i++)
    {
        choosePdfFileWidgets.append(new ChoosePdfFileWidget());
        choosePdfFileWidgets.last()->image->setScaledContents(true);
        QString pathName = allFiles.at(i);
        if( pathName.length() > 9)
        {
            for(int i = 1; i <= pathName.length()/9; i++)
            {
                int n = i * 9;
                pathName.insert(n, " ");
            }
        }
        choosePdfFileWidgets.last()->text->setText(pathName);
        choosePdfFileWidgets.last()->path = defaultPdfPath + "/" + allFiles.at(i);
        connect(choosePdfFileWidgets.last()->button, SIGNAL(clicked()), this, SLOT(clicked()));
    }
//    for (int i = 0; i < 20; i++)
//    {
//        choosePdfFileWidgets.append(new ChoosePdfFileWidget());
//        choosePdfFileWidgets.last()->image->setScaledContents(true);
//        choosePdfFileWidgets.last()->text->setText("pdf_" + QString::number(i));
//        choosePdfFileWidgets.last()->path = "path";
//        connect(choosePdfFileWidgets.last()->button, SIGNAL(clicked()), this, SLOT(clicked()));
//    }
    int current_col = 0;
    for (int i = 0; i < choosePdfFileWidgets.size(); i++)
    {
        choosePdfGrid->addWidget(choosePdfFileWidgets.at(i)->widget, i / 3, current_col);
        current_col++;
        if (current_col == 3)
            current_col = 0;
    }
    choosePdfScrollAreaWidget->setFixedSize(200 * 3, ((choosePdfFileWidgets.size() / 3) + 1) * 170);
}

void PdfScroller::clicked()
{
    QPushButton* caller = (QPushButton*)QObject::sender();
    for (int i = 0; i < choosePdfFileWidgets.size(); i++)
    {
        if (choosePdfFileWidgets.at(i)->button == caller)
        {// нажимаем кнопку
            if (choosePdfFileWidgets.at(i)->image->property("status").toUInt() == 0)
            {
                choosePdfFileWidgets.at(i)->image->setPixmap(QPixmap(":/Images/Images/pdf/pdf_on.png"));
                choosePdfFileWidgets.at(i)->image->setProperty("status", 1);
                pushbuttonDeleteEffect->setOpacity(1.0);
                pushbuttonOpenEffect->setOpacity(1.0);
                choosePdfButtonDelete->setEnabled(true);
                choosePdfButtonOpen->setEnabled(true);
            }
            else
            {
                choosePdfFileWidgets.at(i)->image->setPixmap(QPixmap(":/Images/Images/pdf/pdf_off.png"));
                choosePdfFileWidgets.at(i)->image->setProperty("status", 0);
                pushbuttonDeleteEffect->setOpacity(0.5);
                pushbuttonOpenEffect->setOpacity(0.5);
                choosePdfButtonDelete->setEnabled(false);
                choosePdfButtonOpen->setEnabled(false);
            }
        }
        else
        {
            if (choosePdfFileWidgets.at(i)->image->property("status").toUInt() == 1)
            {// сбрасываем уже выбранные объекты
                choosePdfFileWidgets.at(i)->image->setPixmap(QPixmap(":/Images/Images/pdf/pdf_off.png"));
                choosePdfFileWidgets.at(i)->image->setProperty("status", 0);
            }
        }
    }
}


void PdfScroller::chooseFile()
{
    parent->setLayout(choosePdfHboxLayout);

}

void PdfScroller::choosePageSliderChanged()
{
    choosePageText->setText("Страница " + QString::number(choosePageSlider->value()) + "/" + QString::number(maximalPage));
}

void PdfScroller::choosePageOkClicked()
{
    setPage(choosePageSlider->value() - 1);
//    renderTimer.singleShot(100, this, SLOT(renderPage()));
    progressWidget->startProgress();
    emit renderPage(currentPage, currentScale);
    choosePageCancelClicked();
}

void PdfScroller::choosePageCancelClicked()
{
    choosePageWidget->hide();
    buttonPage->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/pdf/button_page_off.png);");
}

void PdfScroller::errorPageClicked()
{
    errorPageWidget->hide();
    buttonExitClicked();
}

void PdfScroller::buttonPageClicked()
{
    if (buttonPage->styleSheet() == "border-style:none;outline: none;background-image: url(:/Images/Images/pdf/button_page_on.png);")
    {
        choosePageCancelClicked();
        return;
    }
    buttonPage->setStyleSheet("border-style:none;outline: none;background-image: url(:/Images/Images/pdf/button_page_on.png);");
    choosePageSlider->setValue(currentPage + 1);
    choosePageWidget->show();
}

void PdfScroller::buttonExitClicked()
{
    deleteLater();
    parent->deleteLater();
}

void PdfScroller::buttonUpClicked()
{
    buttonsPage->setStyleSheet("QWidget#buttonsPage{background-image: url(:/Images/Images/pdf/buttons_page_up.png);}");
    buttonsPage->repaint();
//    renderTimer.stop();
    if (currentPage - 1 >= 0 && currentPage - 1 < maximalPage)
    {
        emit renderPage(currentPage - 1, currentScale);
        progressWidget->startProgress();
    }
    buttonsTimer.stop();
    buttonsTimer.start(500);
    setPage(currentPage - 1);
    //renderPage();
}

void PdfScroller::buttonDownClicked()
{
    buttonsPage->setStyleSheet("QWidget#buttonsPage{background-image: url(:/Images/Images/pdf/buttons_page_down.png);}");
    buttonsPage->repaint();
//    renderTimer.stop();
    if (currentPage + 1 >= 0 && currentPage + 1 < maximalPage)
    {
        emit renderPage(currentPage + 1, currentScale);
        progressWidget->startProgress();
    }
    buttonsTimer.stop();
    buttonsTimer.start(500);
    setPage(currentPage + 1);
    //renderPage();
}

void PdfScroller::buttonZoomInClicked()
{
    buttonsZoom->setStyleSheet("QWidget#buttonsZoom{background-image: url(:/Images/Images/pdf/buttons_zoom_up.png);}");
    buttonsZoom->repaint();
//    renderTimer.stop();
    if (currentScale + 0.5 >= 0.5 && currentScale + 0.5 <= 3)
    {
        emit renderPage(currentPage, currentScale + 0.5);
        progressWidget->startProgress();
    }
    buttonsTimer.stop();
    buttonsTimer.start(500);
    setScale(currentScale + 0.5);
    //renderPage();
}

void PdfScroller::buttonZoomOutClicked()
{
    buttonsZoom->setStyleSheet("QWidget#buttonsZoom{background-image: url(:/Images/Images/pdf/buttons_zoom_down.png);}");
    buttonsZoom->repaint();
//    renderTimer.stop();
    if (currentScale - 0.5 >= 0.5 && currentScale - 0.5 <= 3)
    {
        emit renderPage(currentPage, currentScale - 0.5);
        progressWidget->startProgress();
    }
    buttonsTimer.stop();
    buttonsTimer.start(500);
    setScale(currentScale - 0.5);
    //renderPage();
}

void PdfScroller::setPage(int page)
{
    if (page >= 0 && page < maximalPage)
    {
        currentPage = page;

        choosePageSlider->setValue(currentPage + 1);
        labelPage->setText(QString::number(currentPage + 1) + "/" + QString::number(maximalPage));
    }
}

void PdfScroller::setScale(double scale)
{
    if (scale >= 0.5 && scale <= 3)
    {
        currentScale = scale;

        labelZoom->setText(QString::number(currentScale) + "x");
    }
}
