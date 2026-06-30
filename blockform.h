#ifndef BLOCKFORM_H
#define BLOCKFORM_H

#include <QWidget>
#include <QLabel>

namespace Ui {
class BlockForm;
}

class BlockForm : public QWidget
{
    Q_OBJECT

public:
    explicit BlockForm(QWidget *parent = nullptr);
    ~BlockForm();

    QLabel *label_blockScreen;
    QLabel *label_blockScreenText;

    bool developerMode;

private slots:
    void on_pushButton_developerMode_clicked();

private:
    Ui::BlockForm *ui;
};

#endif // BLOCKFORM_H
