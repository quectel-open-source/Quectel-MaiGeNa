#ifndef MYPARAMSVIEW_H
#define MYPARAMSVIEW_H

#include <QWidget>
#include <qspinbox.h>

#include "commonZack.h"
#include "QtMyGroupBox.h"

class MyParamsView : public QtMyGroupBox
{
    Q_OBJECT

public:
    explicit MyParamsView(QWidget *parent = nullptr, QString qss="");
    ~MyParamsView();

    void addListWidget(int camid, const QList<double>& scores, QString qss="");
    void addMeanThreshWidget(int camid, double meanThresh);

private:
    MyTitleBar*     v_titleBar;
    QHBoxLayout*    v_mainLayout;
    QList<QListWidget*>   v_listWidgets;
    QList<QList<QDoubleSpinBox*>> v_labels_scoreBoxes;

    QList<QDoubleSpinBox*>   v_meanThreshBoxes;

    QString m_currentName;
    QList<QString> m_templateNames;
    QComboBox*  v_comboBox;

private slots:
    void Init_UI(QString const& qss="");

    void listWidget_itemClicked(int camid, QListWidgetItem* item);
    void toolButton_saveOne_clicked();
    void toolButton_saveBatch_clicked();

signals:
    void saveModeChanged(int, int, int);
    void scoreChanged(int, int, double);
    void meanThreshChanged(int, double);
};

#endif // MYPARAMSVIEW_H
