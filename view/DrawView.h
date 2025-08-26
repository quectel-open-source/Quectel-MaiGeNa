#ifndef DRAWVIEW_H
#define DRAWVIEW_H

#include <QWidget>
#include <iostream>
#include <stdlib.h>
#include <QPainter>
#include <QtConcurrent>

#include "commonZack.h"
#include "BaseItem.h"

struct MyItemGroup
{
    MyPolygonItem*  item;
    bool  is_drawEnd = false;

    MyItemGroup();
    MyItemGroup(MyPolygonItem*  _item) : item(_item){};
};

struct ConfigTemps{
    QJsonObject root;
    QString  default_;
    QJsonArray template_ENG;
    QJsonArray template_CHN;

    QList<QString> temp_chn;
    QList<QString> temp_eng;
    QList<QVector<QPoint>> points_cur;
};


namespace Ui {
class DrawView;
}

class DrawView : public QWidget
{
    Q_OBJECT

public:
    explicit DrawView(QWidget *parent = nullptr, QString qss="");
    ~DrawView();

    void updateQImageFromCam(QImage qimage);

private:
    Ui::DrawView *ui;

    std::atomic_bool m_isOpenDraw;
    std::atomic_bool m_isNewItem;
    int m_count;
    MyPolygonItem*  m_current_item;
    QList<MyItemGroup> v_itemGroups;
//    MyButtonItem*  m_buttonRemove;
    QString m_dir_main;
    QString m_dir_template;
    QImage m_image;

    ConfigTemps m_config_templates;
    bool  m_isModify = false;

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void onClearAllRecv(bool is_clear);

private slots:
    void Init_UI();
    QStringList getDirSons(const QString& path);

    void on_toolButton_openDraw_clicked();
    void on_toolButton_removeOne_clicked();
    void on_toolButton_refresh_clicked();

    void on_toolButton_xiafa_clicked();
    void on_combo_templates_currentIndexChanged(int index);
    void getTemplatePoints(const QString &path, QList<QVector<QPoint>> &points_all, int *w = nullptr, int *h = nullptr);
    void loadConfig(int row);

    void on_toolButton_add_clicked();
    void on_toolButton_delete_clicked();
    void tableWidget_cellClicked(int row, int col);

    void updateOne(int index, const QImage& qimage);

    bool JudgeItem(const QPointF& scenePt);
    void onPtRecv(int img_x, int img_y, QPointF scenePt);

    void on_comboBox_ptSize_currentIndexChanged(int index);

    void on_comboBox_ptColor_currentIndexChanged(int index);

    void on_toolButton_confirm_clicked();


    void on_comboBox_2_currentIndexChanged(int index);

signals:
    void emit_log(QString text);
    void emit_getQImage();
    void emit_template(int, QString, QString, QList<double>);
    void emit_template_param(int, QString, QString, QList<double>);
};

#endif // DRAWVIEW_H
