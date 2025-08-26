#include "DrawView.h"
#include "ui_DrawView.h"
#include <QInputDialog>
#include <qdialog.h>
#include <qpolygon.h>
#define LOG_INFO(...)
#define LOG_ERROR(...)

DrawView::DrawView(QWidget *parent, QString qss)
    : QWidget(parent), ui(new Ui::DrawView), m_isOpenDraw(false), m_isNewItem(false)
{
    ui->setupUi(this);

    m_dir_main = QCoreApplication::applicationDirPath();
    if (!m_dir_main.endsWith("/"))
    {
        m_dir_main += "/";
    }

    Init_UI();
    loadQSS_all(this, qss);
    ui->label_log->setStyleSheet("color: rgb(225, 64, 51);");
    ui->label_log->hide();
}

DrawView::~DrawView()
{
    delete ui;
}
void DrawView::closeEvent(QCloseEvent *event)
{
    if (m_isOpenDraw)
    {
        m_isOpenDraw = false;
    }
    if (ui->toolButton_openDraw->isChecked())
    {
        ui->toolButton_openDraw->setChecked(false);
    }
    return QWidget::closeEvent(event);
}

void DrawView::Init_UI()
{
    ui->graphicsView->needPt();
    connect(ui->graphicsView, &QGraphicsViews::emit_pt, this, &DrawView::onPtRecv);
    m_count = 0;

    //    connect(ui->graphicsView, &QGraphicsViews::emit_itemRemove, this, &DrawView::onButtonRemoveCLicked);
    //    m_buttonRemove = new MyButtonItem(30, 30, ":/Resources/Resources/tolast-2.png");
    //    ui->graphicsView->AddItems(m_buttonRemove);
    //    m_buttonRemove->setZValue(100);
    //    m_buttonRemove->hide();
    ui->toolButton_removeOne->hide();
    ui->comboBox_ptColor->setCurrentIndex(2);

    m_dir_template = m_dir_main + "models/templates";

    QString config_path = m_dir_template + "/config.json";
    m_config_templates.root = readJson(config_path);
    m_config_templates.default_ = m_config_templates.root["default"].toString();
    m_config_templates.template_ENG = m_config_templates.root["template_ENG"].toArray();
    m_config_templates.template_CHN = m_config_templates.root["template_CHN"].toArray();
    LOG_INFO(">>>> Load default: " + m_config_templates.default_);

    for (int m = 0; m < m_config_templates.template_ENG.count(); ++m)
    {
        m_config_templates.temp_eng.append(m_config_templates.template_ENG[m].toString());
    }

    QStringList dir_names = getDirSons(m_dir_template);
    qDebug() << ">>>> Load templates: " << dir_names;

    for (const auto &name : dir_names)
    {
        int index = m_config_templates.temp_eng.indexOf(name);
        if (index >= 0)
        {
            QString name_chn = m_config_templates.template_CHN[index].toString();
            ui->combo_templates->addItem(name_chn);
            m_config_templates.temp_chn.append(name_chn);
        }
        else
        {
            ui->combo_templates->addItem(name);
        }
    }
    int index = m_config_templates.temp_eng.indexOf(m_config_templates.default_);
    if (index < 0)
    {
        m_config_templates.default_ = m_config_templates.temp_eng[0];
        emit emit_log("## default temp is not exist, thus using first...");
        LOG_ERROR("## default temp is not exist, thus using first...");
    }
    ui->combo_templates->setCurrentIndex(index);

    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(ui->tableWidget, &QTableWidget::cellClicked, this, &DrawView::tableWidget_cellClicked);

    QTimer::singleShot(500, this, [=] {
        QString default_template_path = m_dir_template + "/" + m_config_templates.default_ + "/template.json";
        int imwidth = 0, imheight = 0;
        getTemplatePoints(default_template_path, m_config_templates.points_cur, &imwidth, &imheight);
        int cnts = m_config_templates.points_cur.count();
        ui->label_info->setText("有" + QString::number(cnts) + "个模板");

        // for (int m = 0; m < cnts; ++m)
        // {
        //     auto shapePoints = m_config_templates.points_cur.at(m);
        //     for  (int i = 0; i < shapePoints.size(); ++i)
        //     {
        //         onPtRecv(shapePoints.at(i).x(), shapePoints.at(i).y(), shapePoints.at(i));
        //     }

        //     QPolygon shapePoly(shapePoints);
        //     auto image = QImage(imwidth, imheight, QImage::Format_ARGB32);
        //     ui->graphicsView->DispImage(image);
        //     auto shapeROI = image.copy(shapePoly.boundingRect());
        //     updateOne(m, shapeROI);
        // }

        loadConfig(index);
    });
}
QStringList DrawView::getDirSons(const QString &path)
{
    QStringList dirs;
    QDir dir(m_dir_template);
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList list = dir.entryInfoList();
    for (const auto &info : list)
    {
        dirs.append(info.fileName());
    }
    return dirs;
}

void DrawView::updateQImageFromCam(QImage qimage)
{
    if (!qimage.isNull())
    {
        ui->graphicsView->DispImage(qimage);
    }
}
bool DrawView::JudgeItem(const QPointF &scenePt)
{
    QPoint viewPt = ui->graphicsView->mapFromScene(scenePt);
    QGraphicsItem *baseItem = ui->graphicsView->itemAt(viewPt);

    MyPolygonItem *item = qgraphicsitem_cast<MyPolygonItem *>(baseItem);
    if (item)
    {
        m_current_item = item;
        //        qDebug() << ">>> 是 PolygonItem";
        for (int m = 0; m < v_itemGroups.count(); ++m)
        {
            if (m_current_item == v_itemGroups[m].item)
            {
                v_itemGroups[m].item->setZValue(20);
            }
            else
            {
                v_itemGroups[m].item->setZValue(10);
            }
        }

        return true;
    }
    return false;
}
void DrawView::onPtRecv(int img_x, int img_y, QPointF scenePt)
{
    if (JudgeItem(scenePt))
        return;
    if (m_isOpenDraw)
    {
        QPointF pt(img_x, img_y);
        if (m_isNewItem)
        {
            if (img_x >= 0 && img_y >= 0)
            {
                m_isNewItem = false;
                MyPolygonItem *item = new MyPolygonItem();
                ui->graphicsView->AddItems(item);

                item->push(pt, false);
                v_itemGroups.append(MyItemGroup(item));
                m_current_item = v_itemGroups[m_count].item;
            }
        }
        else
        {
            //            m_current_item = v_itemGroups[m_count].item;
            //            const QList<QPointF>& pts_all = m_current_item->getPts();
            //            if (pts_all.empty())
            //                return;
            //            qreal dx = qAbs(pt.x() - pts_all.first().x());
            //            qreal dy = qAbs(pt.y() - pts_all.first().y());
            //            double distance = std::sqrt(dx * dx + dy * dy);
            //            if (distance < BaseItem::ContrSize * 4){
            //                v_itemGroups[m_count].item->push(pt, true);
            //                v_itemGroups[m_count].is_drawEnd = true;

            //                QImage qimage = ui->graphicsView->getImage();
            //                const auto& rect = v_itemGroups[m_count].item->getRect();
            //                QImage roi = qimage.copy(rect);
            //                updateOne(m_count, roi);

            //                m_count++;
            //                ui->label_log->setText("绘制中..已绘制模板：" + QString::number(m_count));
            //                m_isNewItem = true;
            //            }
            //            else{
            v_itemGroups[m_count].item->push(pt, false);
            //            }
        }
    }
}
void DrawView::updateOne(int index, const QImage &qimage)
{
    int row = m_count / 2;
    int gap = 130;
    ui->tableWidget->setRowCount(row + 1);
    ui->tableWidget->setRowHeight(row, gap);
    QComboBox *box =
        newComboBox(this, {"位置" + QString::number(index)}, "位置" + QString::number(index), -1, 24, index);
    box->setMinimumWidth(30);
    QLabel *label = newLabel(this, "", qimage.width(), qimage.height());
    int ww, hh;
    if (qimage.width() > qimage.height())
    {
        ww = gap;
        hh = gap * 1.0 * qimage.height() / qimage.width();
    }
    else
    {
        ww = gap * 1.0 * qimage.width() / qimage.height();
        hh = gap;
    }
    label->setFixedSize(ww, hh);
    label->setPixmap(QPixmap::fromImage(qimage));
    label->setScaledContents(true);
    MyWidget *w = new MyWidget(this);
    newVBoxLayout(w, {box, label});

    ui->tableWidget->setCellWidget(row, m_count % 2, w);
}
/////////////////////
void DrawView::on_toolButton_openDraw_clicked()
{
    if (ui->toolButton_openDraw->isChecked())
    {
        if (ui->graphicsView->getImage().isNull())
        {
            QMessageBox::information(this, "提示", "需要先加载图像");
            ui->toolButton_openDraw->setChecked(false);
            return;
        }
        m_isOpenDraw = true;
        m_isNewItem = true;
        //        m_buttonRemove->setPos(ui->graphicsView->rect().width() - 31, 1);
        ui->toolButton_removeOne->show();

        ui->label_log->setText("绘制中..");
        ui->label_log->show();
    }
    else
    {
        //        bool is_end = true;
        //        if (m_count > 0) {
        //            auto reply = QMessageBox::information(this, "提示", "模板未保存，是否确定结束绘制？");
        //            if (reply != QMessageBox::Ok){
        //                is_end = false;
        //            }
        //        }
        //        if (is_end) {
        //            m_count = 0;
        m_isOpenDraw = false;
        //            ui->toolButton_removeOne->hide();

        //            ui->label_log->setText("绘制结束！");
        //            QTimer::singleShot(2000, this, [=]{ ui->label_log->hide();});
        //        }
    }
}
void DrawView::on_toolButton_removeOne_clicked()
{
    if (m_current_item)
    {
        int index = -1;
        bool is_remove = false;
        for (int m = 0; m < v_itemGroups.count(); ++m)
        {
            if (v_itemGroups[m].item == m_current_item)
            {
                index = m;
                const auto &pts = v_itemGroups[m].item->getPts();
                if (!v_itemGroups[m].is_drawEnd && !pts.empty())
                {
                    is_remove = true;
                }
                break;
            }
        }
        if (is_remove)
        {
            v_itemGroups[index].item->pop();
        }
    }
}
void DrawView::on_toolButton_confirm_clicked()
{
    if (ui->toolButton_openDraw->isChecked())
    {
        if (m_count >= v_itemGroups.count())
        {

            return;
        }
        if (m_current_item != v_itemGroups[m_count].item)
            return;
        if (!v_itemGroups[m_count].is_drawEnd)
        {
            v_itemGroups[m_count].is_drawEnd = true;
            m_isModify = true;
            v_itemGroups[m_count].item->finish();

            QImage qimage = ui->graphicsView->getImage();
            const auto &pts = v_itemGroups[m_count].item->getPts();
            const auto &rect = v_itemGroups[m_count].item->getRect();
            QImage roi = qimage.copy(rect);
            QPainter painter(&roi);
            QPen pen(Qt::green);
            pen.setWidth(2);
            painter.setPen(pen);
            for (int i = 0; i < pts.count() - 1; i++)
            {
                painter.drawLine(pts[i], pts[i + 1]);
            }
            painter.drawLine(pts[pts.count() - 1], pts[0]);
            painter.end();
            updateOne(m_count, roi);

            //            // save roi
            //            int count = m_count;
            //            QString tem_name =
            //            m_config_templates.template_ENG[ui->combo_templates->currentIndex()].toString();
            //            QVector<QPoint> pts_one;
            //            for (const auto &p : pts)
            //            {
            //                pts_one.append(p.toPoint());
            //            }
            //            QtConcurrent::run([=] {
            //                QImage tmp = roi.copy();
            //                tmp.fill(Qt::black);
            //                QPainter painter(&tmp);
            //                painter.setBrush(Qt::white);
            //                QPolygon po;
            //                po.append(pts_one);
            //                painter.drawPolygon(po);
            //                painter.end();

            //                QString path = m_dir_template + "/" + tem_name + "/" + QString::number(count) + ".png";
            //                tmp.save(path);
            //            });

            m_count++;
            ui->label_log->setText("绘制中..已绘制模板：" + QString::number(m_count));
            m_isNewItem = true;
        }
    }
}

void DrawView::getTemplatePoints(const QString &path, QList<QVector<QPoint>> &points_all, int *w, int *h)
{
    points_all.clear();
    QJsonObject jsonObject = readJson(path);
    QJsonArray shapesArray = jsonObject["shapes"].toArray();
    if (w) *w =  jsonObject["imageWidth"].toInt();
    if (h) *h =  jsonObject["imageHeight"].toInt();

    for (const QJsonValue &shapeValue : shapesArray)
    {
        QJsonObject shapeObject = shapeValue.toObject();

        // 获取各个字段
        QString label = shapeObject["label"].toString();
        QString id = shapeObject["id"].toString();

        QVector<QPoint> points;
        QJsonArray pointsArray = shapeObject["points"].toArray();
        for (int n = 0; n < pointsArray.count(); ++n)
        {
            QJsonArray pt = pointsArray[n].toArray();
            double x = pt[0].toDouble();
            double y = pt[1].toDouble();

            points.append(QPoint(x, y));
        }
        points_all.append(points);
    }
}
void DrawView::on_combo_templates_currentIndexChanged(int index)
{
    static int m_combo_index = 0;
    if (m_isModify)
    {
        if (QMessageBox::information(this, "", "当前方案未保存, 是否需要切换?", QMessageBox::Cancel,
                                     QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes)
        {
            ui->combo_templates->setCurrentIndex(m_combo_index);
        }
    }
    m_combo_index = ui->combo_templates->currentIndex();

    m_isNewItem = true;
    m_isOpenDraw = false;
    ui->toolButton_openDraw->setChecked(false);
    m_count = 0;
    m_current_item = nullptr;
    m_isModify = false;

    QString name_eng = m_config_templates.template_ENG[ui->combo_templates->currentIndex()].toString();
    QString template_path = m_dir_template + "/" + name_eng + "/template.json";
    getTemplatePoints(template_path, m_config_templates.points_cur);
    ui->label_info->setText(QString::number(m_config_templates.points_cur.count()) + "个模板");

    ui->tableWidget->clear();
}
void DrawView::on_toolButton_xiafa_clicked()
{
    QString name_chn = ui->combo_templates->currentText();
    bool is_set = false;
    if (m_isModify)
    {
        if (QMessageBox::information(this, "", "是否保存并设置为默认方案? \n  " + name_chn, QMessageBox::Yes,
                                     QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes)
        {
            m_isModify = false;
            QString template_dir_path =
                m_dir_template + "/template" + QString::number(ui->combo_templates->currentIndex());
            QString template_json_path = template_dir_path + "/template.json";
            //            QList<QPair<QImage, QString>> rois; // <image, savepath>
            QList<QString> dirs;
            ////////////
            QJsonArray shapes;
            for (int m = 0; m < v_itemGroups.count(); ++m)
            {
                QJsonObject shape;
                QString id = QString::number(m);
                shape["id"] = id;

                const auto &pts = v_itemGroups[m].item->getPts();
                QJsonArray points;
                for (int n = 0; n < pts.count(); ++n)
                {
                    QJsonArray pt;
                    pt.append(pts[n].x());
                    pt.append(pts[n].y());
                    points.append(pt);
                }
                shape["points"] = points;
                shapes.append(shape);
                auto t_dir_path = template_dir_path + "/" + id + "_T";
                dirs.append(t_dir_path);

                //                const auto &rect = v_itemGroups[m].item->getRect();
                //                QImage roi = ui->graphicsView->getImage().copy(rect);
                //                QString tem_name = template_dir_path;
                //                QString path = m_dir_template + "/" + tem_name + "/" + QString::number(m) + ".png";
                //                rois.append(roi);
            }
            QJsonArray a_scores;
            for (int m = 0; m < v_itemGroups.count(); ++m)
            {
                a_scores.append(0.6);
            }
            QJsonObject root;
            root["shapes"] = shapes;
            const auto &qimage = ui->graphicsView->getImage();
            root["a_scores"] = a_scores;
            root["a_flip"] = -1;
            root["a_jiaoz"] = true;
            root["a_openAlgo"] = true;
            root["imageWidth"] = qimage.width();
            root["imageHeight"] = qimage.height();
            writeJson(root, template_json_path);

            ////////////
            for (const auto &di : dirs)
            {
                QDir dir(di);
                if (!dir.exists())
                {
                    dir.mkdir(di);
                }
            }
            is_set = true;
        }
    }
    else
    {
        if (QMessageBox::information(this, "", "是否设置默认方案为: " + name_chn + " ?", QMessageBox::Yes,
                                     QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes)
        {
            is_set = true;
        }
    }

    if (is_set)
    {
        int row = ui->combo_templates->currentIndex();
        m_config_templates.default_ = m_config_templates.template_ENG[row].toString();
        m_config_templates.root["default"] = m_config_templates.default_;
        writeJson(m_config_templates.root, m_dir_template + "/config.json");

        QString text = ui->label_log->text();
        ui->label_log->setText("设置默认方案为: " + name_chn);
        QTimer::singleShot(2000, this, [=] { ui->label_log->setText(text); });

        loadConfig(row);
    }
}
void DrawView::loadConfig(int row)
{
    QString dir_path = m_dir_template + "/template" + QString::number(row);
    if (!m_image.isNull())
    {
        QString template_image_path = dir_path + "/template.png";
        m_image.save(template_image_path);
    }

    QString template_json_path = dir_path + "/template.json";
    QJsonObject root_json = readJson(template_json_path);
    QJsonArray scores = root_json["a_scores"].toArray();
    QList<double> scores_each;
    for (int m = 0; m < scores.count(); ++m)
    {
        scores_each.append(scores[m].toDouble());
    }
    emit emit_template_param(row, m_config_templates.default_, m_config_templates.temp_chn[row], scores_each);
    emit emit_template(row, m_config_templates.default_, m_config_templates.temp_chn[row], scores_each);
    emit emit_log(">>>> load default template: " + m_config_templates.default_ + "[" + QString::number(row) + "]");
    LOG_INFO(">>>> load default template: " + m_config_templates.default_ + "[" + QString::number(row) + "]");
}

void DrawView::onClearAllRecv(bool is_clear)
{
    if (is_clear)
    {
        QtConcurrent::run([=] {
            QString dir_path = m_dir_template + "/template" + ui->combo_templates->currentIndex();
            QDir dir(dir_path);

            QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            int ii = 0;
            foreach (const QFileInfo &subDirInfo, subDirs)
            {
                QDir dir_son(subDirInfo.absoluteFilePath());
                QFileInfoList files = dir_son.entryInfoList(QDir::Files);
                bool is_clear = true;
                for (const auto &file : files)
                {
                    is_clear = is_clear && QFile::remove(file.absoluteFilePath());
                }
                ii++;
                if (is_clear)
                {
                    emit emit_log("## clear template: 位置" + QString::number(ii));
                }
            }
        });
    }
}

void DrawView::on_toolButton_add_clicked()
{
    // 显示inputdialog， 要求输入方案名称
    bool ok;
    QString name_chn =
        QInputDialog::getText(this, tr("添加方案"), tr("请输入新的方案名称:"), QLineEdit::Normal, "", &ok);

    if (m_config_templates.temp_chn.contains(name_chn))
    {
        QMessageBox::information(this, "", "该方案名称已存在!");
        on_toolButton_add_clicked();
        return;
    }

    int index = m_config_templates.temp_chn.count();

    QString name_eng = "template" + QString::number(index);
    m_config_templates.template_ENG.append(name_eng);
    m_config_templates.template_CHN.append(name_chn);
    m_config_templates.root["template_ENG"] = m_config_templates.template_ENG;
    m_config_templates.root["template_CHN"] = m_config_templates.template_CHN;
    m_config_templates.temp_chn.append(name_chn);
    writeJson(m_config_templates.root, m_dir_template + "/config.json");

    QString dir_path = m_dir_template + "/" + name_eng;
    QDir dir(dir_path);
    if (!dir.exists())
    {
        dir.mkdir(dir_path);
    }
    ui->combo_templates->addItem(name_chn);
    ui->combo_templates->setCurrentText(name_chn);
}

void DrawView::on_toolButton_delete_clicked()
{
    if (!ui->tableWidget->hasFocus())
    {
        QMessageBox::information(this, "", "请先选中下表, 才可以删除");
        return;
    }

    for (int m = 0; m < v_itemGroups.count(); ++m)
    {
        if (v_itemGroups[m].item == m_current_item)
        {
            v_itemGroups.removeAt(m);
            delete m_current_item;
            m_current_item = nullptr;

            int row = m / 2;
            int col = m % 2;
            auto w = ui->tableWidget->cellWidget(row, col);
            ui->tableWidget->removeCellWidget(m, col);
            delete w;
            m_count--;
            ui->label_log->setText("绘制中.. 模板数量" + QString::number(m_count));

            break;
        }
    }
}
void DrawView::tableWidget_cellClicked(int row, int col)
{
    int index = row * 2 + col;
    if (index < v_itemGroups.count())
    {
        m_current_item = v_itemGroups[index].item;
    }
    else
    {
        m_current_item = nullptr;
    }
}

void DrawView::on_toolButton_refresh_clicked()
{
    emit emit_getQImage();
    emit emit_log("refresh one image");
}

void DrawView::on_comboBox_ptSize_currentIndexChanged(int index)
{
    BaseItem::ContrSize = index + 1;
    for (const auto &group : v_itemGroups)
    {
        group.item->update();
    }
    emit emit_log(">> update pointSize: " + ui->comboBox_ptSize->currentText());
}

void DrawView::on_comboBox_ptColor_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ControlItem::MyColor = QColor(255, 60, 0);
    }
    else if (index == 1)
    {
        ControlItem::MyColor = QColor(20, 255, 100);
    }
    else
    {
        ControlItem::MyColor = QColor(0, 100, 255);
    }
    for (const auto &group : v_itemGroups)
    {
        group.item->noSelected.setColor(ControlItem::MyColor);
        group.item->update();
    }
    emit emit_log(">> update pointColor: " + ui->comboBox_ptColor->currentText());
}

void DrawView::on_comboBox_2_currentIndexChanged(int index)
{
    int value = index + 1;
    for (const auto &group : v_itemGroups)
    {
        group.item->LineWidth = value;
        group.item->PolygonLineWidth = value;
        group.item->noSelected.setWidth(value);
    }
    emit emit_log(">> update lineWidth: " + ui->comboBox_ptSize->currentText());
}
