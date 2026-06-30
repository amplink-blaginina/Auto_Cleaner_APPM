#ifndef SETTINGSFORM_H
#define SETTINGSFORM_H

#include <QWidget>
#include <QSettings>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

#include <settings/settingselement.h>

namespace Ui {
class SettingsForm;
}

class SettingsForm : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsForm(QSettings *settings_, QWidget *parent_ = nullptr);
    ~SettingsForm();

    QWidget* parent;

    QSettings *settings;
    QList<SettingsElement*> elementsGlobal;
    QList<SettingsElement*> elementsMediumSweep;
    QList<SettingsElement*> elementsLightSweep;
    QList<SettingsElement*> elementsHeavySweep;
    QList<SettingsElement*> elementsFrontTimings;
    QList<SettingsElement*> elementsMiddleTimings;
    QList<SettingsElement*> elementsBackTimings;

    void fillElements();
    void callGlobal();
    void callLightSweep();
    void callMediumSweep();
    void callHeavySweep();
    void callFrontTimings();
    void callMiddleTimings();
    void callBackTimings();
    void clearGrid();
    void showElements(int col_cnt, QList<SettingsElement*> & elements);
    void pressGrid();

private:
    Ui::SettingsForm *ui;
    QScrollArea* elementsScrollArea;
signals:
    void closedAndSave();
    void Send_Pass_2_pass_form(int);
public slots:
    void on_pushButton_save_clicked();
};

#endif // SETTINGSFORM_H
