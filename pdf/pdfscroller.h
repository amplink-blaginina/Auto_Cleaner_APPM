#ifndef PDFSCROLLER_H
#define PDFSCROLLER_H

#include <QObject>
#include <QLabel>
#include <QScrollArea>
#include <QScroller>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QThread>

#include <poppler/qt5/poppler-qt5.h>

class PdfScroller;

class ProgressWidget : public QObject
{
    Q_OBJECT
    public:
    ProgressWidget(QWidget *parent_ = nullptr);
    ~ProgressWidget();
    // прогресс загрузки
    QWidget* parent;
    QLabel* progressLabel;
    quint8 progressCurrentPicture;
    QTimer* progressTimer;

    void startProgress();
    void stopProgress();
public slots:
    void showProgress();
};
class PdfElement : public QObject
{
    Q_OBJECT
public:
    PdfElement(PdfScroller* parent_);
    ~PdfElement();

    PdfScroller* parent;

    Poppler::Document* document;
public slots:
    void threadStarted();
    bool openFile(QString path);
    bool renderPage(int currentPage, double currentScale);
signals:
    void openFileReady(int);
    void renderPageReady(QImage);
};

class ChoosePdfFileWidget
{
    public:
    ChoosePdfFileWidget()
    {
        widget = new QWidget();
        image = new QLabel(widget);
        text = new QLabel(widget);
        button = new QPushButton(widget);

        widget->setGeometry(0, 0, 100, 180);
        image->setGeometry(0, 0, 100, 100);
        image->setAlignment(Qt::AlignHCenter);
        image->setPixmap(QPixmap(":/Images/Images/pdf/pdf_off.png"));
        image->setProperty("status", 0);
        text->setGeometry(0, 110, 100, 70);
        text->setWordWrap(true);
        text->setAlignment(Qt::AlignHCenter);
        button->setGeometry(0, 0, 100, 100);
        button->setStyleSheet("border-style:none;outline: none;");
    }
    ~ChoosePdfFileWidget()
    {
        button->disconnect();
        delete button;
        delete text;
        delete image;
        delete widget;
    }
    QPushButton* button;
    QWidget* widget;
    QLabel* image;
    QLabel* text;
    QString path;
};

class PdfScroller : public QObject
{
    Q_OBJECT
public:

    explicit PdfScroller(QWidget *parent = nullptr);
    ~PdfScroller();

    void chooseFile();

    void setPage(int page);
    void setScale(double scale);
    void fillFilesGrid();
    void setPaths(QString pdfPath, QString importPath);

    QWidget* parent;

    ProgressWidget* progressWidget;

    PdfElement* pdfElement;
    QThread* pdfThread;

    // ошибка
    QWidget* errorPageWidget;
    QLabel* errorPageText;
    QPushButton* errorPageOk;

    // выбор страницы
    QWidget* choosePageWidget;
    //QVBoxLayout* choosePageLayout;
    //QLabel* choosePageTitle;
    QSlider* choosePageSlider;
    QLabel* choosePageText;
    QPushButton* choosePageOk;
    QPushButton* choosePageCancel;

    // выбор файла
    QGraphicsOpacityEffect *pushbuttonOpenEffect;
    QGraphicsOpacityEffect *pushbuttonDeleteEffect;
    QGraphicsOpacityEffect *pushbuttonImportEffect;
    QHBoxLayout* choosePdfHboxLayout;
    QVBoxLayout* choosePdfVboxLayout;
    QScrollArea* choosePdfScrollArea;
    QWidget* choosePdfScrollAreaWidget;
    QPushButton* choosePdfButtonOpen;
    QPushButton* choosePdfButtonDelete;
    QPushButton* choosePdfButtonImport;
    QGridLayout* choosePdfGrid;
    QPushButton* choosePdfButtonExit;
    QList<ChoosePdfFileWidget*> choosePdfFileWidgets;

    // просотр pdf
    QLabel *imageLabel;
    QPushButton* buttonPage;
    QPushButton* buttonExit;
    QWidget* buttonsPage;
    QLabel* labelPage;
    QPushButton* buttonUp;
    QPushButton* buttonDown;
    QWidget* buttonsZoom;
    QPushButton* buttonZoomIn;
    QPushButton* buttonZoomOut;
    QLabel* labelZoom;
    int currentPage;
    int maximalPage;
    double currentScale;
    QHBoxLayout* hbox_layout;
    QVBoxLayout* vbox_layout;
    QScrollArea *scrollArea;
    QTimer buttonsTimer;
//    QTimer renderTimer;
signals:
    void openFile(QString path);
    void renderPage(int, double);
public slots:
    void renderPageReady(QImage image);
    void openFileReady(int);
    void choosePdfButtonOpenClicked();
    void choosePdfButtonDeleteClicked();
    void choosePdfButtonImportClicked();
    void choosePdfButtonExitClicked();
    void clicked();
    void choosePageSliderChanged();
    void choosePageOkClicked();
    void choosePageCancelClicked();
    void buttonExitClicked();
    void errorPageClicked();
    void buttonPageClicked();
    void buttonUpClicked();
    void buttonDownClicked();
    void buttonZoomInClicked();
    void buttonZoomOutClicked();
    void buttonsOff();
};

#endif // PDFSCROLLER_H
