#include "EditSolution.h"
#include "qdrawer.h"
#include "solutionmanager.h"
#include "ui_EditSolution.h"
#include "utils.h"
#include <qchar.h>
#include <qcolor.h>
#include <qfont.h>
#include <qglobal.h>
#include <qgraphicsitem.h>
#include <qlist.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qpolygon.h>
#include <qtimer.h>
#include <qwindowdefs.h>

EditSolution::EditSolution(QWidget *parent) : QWidget(parent), ui(new Ui::EditSolution)
{
    ui->setupUi(this);
    initUI();
    initConnections();
}

EditSolution::~EditSolution()
{
    qDebug() << "EditSolution::~EditSolution()";
    delete ui;
}

void EditSolution::initUI()
{
    // 初始化场景
    m_scene = ui->graphicsView->scene();
    // m_scene = new QGraphicsScene(this);
    // ui->graphicsView->setScene(m_scene);
    // ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    // ui->graphicsView->setSceneRect(INT_MIN / 2, INT_MIN / 2, INT_MAX, INT_MAX);

    // 初始化形状选择框
    ui->shapesCombo->addItem(tr("矩形"), "Rectangle");
    ui->shapesCombo->addItem(tr("椭圆形"), "Ellipse");
    ui->shapesCombo->addItem(tr("多边形"), "Polygon");

    // 初始化ROI表格
    ui->tableWidget->setColumnWidth(0, 60); // ID
    ui->tableWidget->setColumnWidth(1, 80); // 标签
    ui->tableWidget->setColumnWidth(2, 80); // 类型
    ui->tableWidget->setColumnWidth(3, 80); // 阈值
    ui->tableWidget->setColumnWidth(4, 80); // 操作

    QTimer::singleShot(500, this, &EditSolution::loadSolutions);
    // 设置初始状态
    updateStatus(tr("就绪"));
}

void EditSolution::initConnections()
{
    // 方案管理
    connect(ui->addTemplateBtn, &QToolButton::clicked, this, &EditSolution::onAddSolution);
    connect(ui->delTemplateBtn, &QToolButton::clicked, this, &EditSolution::onDeleteSolution);
    connect(ui->saveTemplateBtn, &QToolButton::clicked, this, &EditSolution::onSaveSolution);
    connect(ui->templatesCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        qDebug() << "templatesCombo::currentTextChanged" << text;
        auto solId = ui->templatesCombo->currentData().toString();
        SolutionManager::instance()->loadSolution(solId);
    });

    // 图像操作
    connect(ui->captureBtn, &QToolButton::clicked, this, &EditSolution::onCaptureImage);
    connect(ui->openImageBtn, &QToolButton::clicked, this, &EditSolution::onOpenImage);
    connect(ui->saveImageBtn, &QToolButton::clicked, this, &EditSolution::onSaveImage);

    // ROI管理
    connect(ui->drawShapeBtn, &QToolButton::clicked, this, &EditSolution::onDrawROI);
    connect(ui->tableWidget, &QTableWidget::itemClicked, this, &EditSolution::onTableItemClicked);
    connect(ui->tableWidget, &QTableWidget::itemChanged, this, &EditSolution::onTableItemChanged);

    connect(SolutionManager::instance(), &SolutionManager::solutionsLoaded, this, &EditSolution::setSolutions);
    connect(SolutionManager::instance(), &SolutionManager::solutionLoaded, this, &EditSolution::setSolution);
    connect(SolutionManager::instance(), &SolutionManager::errorOccurred, this, &EditSolution::updateStatusError);
}

void EditSolution::updateStatus(const QString &message, bool isError)
{
    QString text = message;
    if (isError)
    {
        text = "<font color='red'>" + text + "</font>";
    }
    ui->statusLabel->setText(text);
}

void EditSolution::loadSolutions()
{
    SolutionManager::instance()->loadConfig();
}
void EditSolution::setSolutions(QList<Solution *> solutions)
{
    ui->templatesCombo->clear();
    int defaultIndex = -1;
    for (int i = 0; i < solutions.size(); ++i)
    {
        auto &solution = solutions[i];
        ui->templatesCombo->addItem(solution->name, solution->id);
        if (solution->isDefault)
        {
            defaultIndex = i;
        }
    }

    if (defaultIndex == -1 && !solutions.isEmpty())
    {
        defaultIndex = 0;
    }
    if (defaultIndex != -1)
    {
        if (!SolutionManager::instance()->loadSolution(solutions.at(defaultIndex)->id))
        {
            qCritical() << "load solution failed";
            return;
        }
        setSolution(solutions.at(defaultIndex));
    }
}

// setimage
void EditSolution::setSolution(const Solution *solution)
{
    qDebug() << "setSolution" << solution->id;
    ui->templatesCombo->setCurrentText(solution->name);

    m_scene->clear();
    setImage(solution->image);
    setROIs(solution->rois.values());
}

void EditSolution::setImage(const QImage &image)
{
    qDebug() << "setImage" << image;
    if (image.isNull())
    {
        updateStatusError(tr("无法设置空图像"));
        return;
    }

    ui->graphicsView->show_image(image);

    // if (!m_backgroundItem)
    // {
    //     m_backgroundItem = m_scene->addPixmap(QPixmap::fromImage(image));
    //     m_backgroundItem->setZValue(0);
    //     m_backgroundItem->setPos({0, 0});
    // }
    // else
    // {
    //     m_backgroundItem->setPixmap(QPixmap::fromImage(image));
    // }
    // ui->graphicsView->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);

    updateStatus(tr("已设置新图像"));
}

void EditSolution::setROIs(const QList<ROI> &rois)
{
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);

    // 清空m_scene 中的z=2的全部item 以及他们的子item
    for (auto item : m_scene->items())
    {
        if (item->zValue() == 2)
        {
            for (auto child : item->childItems())
            {
                m_scene->removeItem(child);
                delete child;
            }
            m_scene->removeItem(item);
            delete item;
        }
    }

    foreach (auto &roi, rois)
    {
        addROIToTable(roi);
        paintROI(roi);
    }
}

void EditSolution::addROIToTable(const ROI &roi)
{
    qDebug() << "roi tablewidget rows:" << ui->tableWidget->rowCount();
    ui->tableWidget->blockSignals(true);
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    // ID列
    QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(roi.id));
    idItem->setFlags(idItem->flags() ^ Qt::ItemIsEditable);
    ui->tableWidget->setItem(row, 0, idItem);

    // 标签列
    QTableWidgetItem *labelItem = new QTableWidgetItem(roi.label);
    ui->tableWidget->setItem(row, 1, labelItem);

    // 类型列
    QTableWidgetItem *typeItem = new QTableWidgetItem(tr(roi.type.toLatin1()));
    typeItem->setFlags(typeItem->flags() ^ Qt::ItemIsEditable);
    ui->tableWidget->setItem(row, 2, typeItem);

    // 阈值列
    QTableWidgetItem *thresholdItem = new QTableWidgetItem(QString::number(roi.threshold));
    ui->tableWidget->setItem(row, 3, thresholdItem);

    // 操作列 - 使用按钮
    QToolButton *actionButton = new QToolButton();
    actionButton->setIcon(QIcon(":/Resources/Resources/delete.png"));
    // actionButton->setText(tr("删除"));
    actionButton->setProperty("row", row);
    connect(actionButton, &QToolButton::clicked, this, &EditSolution::onDeleteROIClicked);
    ui->tableWidget->setCellWidget(row, 4, actionButton);
    ui->tableWidget->blockSignals(false);

    if (m_selectedROIId == roi.id)
    {
        ui->tableWidget->selectRow(row);
    }
}

void EditSolution::paintROI(const ROI &roi)
{
    qDebug() << "paintROI:" << roi.id << " " << roi.type << " " << " " << roi.color << roi.points;
    // 在 GraphicsView 中绘制 ROI
    QAbstractGraphicsShapeItem *item = nullptr;

    if (QString::compare(roi.type, "Ellipse", Qt::CaseInsensitive) == 0)
    {
        if (roi.points.size() != 2)
        {
            qCritical() << "Invalid number of points for Circle ROI:" << roi.points;
            updateStatusError(tr("圆形ROI和坐标个数不匹配"));
            return;
        }

        auto rectt = QRectF(roi.points.at(0), roi.points.at(1));
        item = new BEllipse(rectt.center().x(), rectt.center().y(), rectt.width(), rectt.height());
    }
    else if (QString::compare(roi.type, "Rectangle", Qt::CaseInsensitive) == 0)
    {
        if (roi.points.size() != 2)
        {
            qCritical() << "Invalid number of points for Rectangle ROI:" << roi.points;
            updateStatusError(tr("四边形ROI和坐标个数不匹配"));
            return;
        }

        item = new BRectangle(roi.points[0].x(), roi.points[0].y(), roi.points[1].x(), roi.points[1].y());
    }
    else if (QString::compare(roi.type, "Polygon", Qt::CaseInsensitive) == 0)
    {
        if (roi.points.size() < 3)
        {
            qCritical() << "Invalid number of points for Polygon ROI:" << roi.points;
            updateStatusError(tr("多边形ROI和坐标个数不匹配"));
            return;
        }

        auto polygon = new BPolygon();
        QList<QPointF> list;
        foreach (auto edge, roi.points)
        {
            list.append(edge);
            polygon->pushPoint(edge, list, false);
        }
        polygon->setX(QPolygonF(list.toVector()).boundingRect().center().x());
        polygon->setY(QPolygonF(list.toVector()).boundingRect().center().y());
        polygon->pushPoint(QPointF(0, 0), list, true);
        item = polygon;
    }
    else
    {
        qCritical() << "Invalid ROI type:" << roi.type;
        updateStatusError(tr("无效图形类型:") + roi.type);
        return;
    }

    item->setZValue(2);
    item->setPen(QPen(roi.color, roi.thickness));
    m_scene->addItem(item);

    if (m_selectedROIId == roi.id)
    {
        item->setSelected(true);
    }
}

// 方案管理槽函数
void EditSolution::onAddSolution()
{
    bool ok;
    QString solutionName =
        QInputDialog::getText(this, tr("新增方案"), tr("请输入方案名称:"), QLineEdit::Normal, "", &ok);

    if (!ok)
        return;

    qDebug() << "onAddSolution" << solutionName;
    if (solutionName.trimmed().isEmpty() || solutionName.contains(" ") || solutionName.contains("/"))
    {
        QMessageBox::warning(this, tr("错误"), tr("方案名称不能为空或包含空格或/"));
        return;
    }

    if (ui->templatesCombo->findText(solutionName) != -1)
    {
        QMessageBox::warning(this, tr("错误"), tr("方案名称已存在"));
        return;
    }

    auto newSol = SolutionManager::instance()->createSolution(solutionName, currentSolutionImage());

    if (newSol)
    {
        updateStatus(tr("已添加方案: ") + solutionName);
        setSolution(newSol);
    }
}

void EditSolution::onDeleteSolution()
{
    if (ui->templatesCombo->count() <= 1)
    {
        QMessageBox::warning(this, tr("错误"), tr("不能删除最后一个方案!"));
        return;
    }

    QString solutionName = ui->templatesCombo->currentText();
    QString solutionId = ui->templatesCombo->currentData().toString();
    int index = ui->templatesCombo->currentIndex();

    if (QMessageBox::Yes != QMessageBox::question(this, tr("确认删除"), tr("确定要删除方案: ") + solutionName + "?",
                                                  QMessageBox::Yes | QMessageBox::No))
    {
        qDebug() << "取消删除";
        return;
    }

    if (!SolutionManager::instance()->deleteSolution(solutionId))
    {
        updateStatusError(tr("删除方案失败:") + solutionName);
        return;
    }

    ui->templatesCombo->removeItem(index);
    updateStatus(tr("已删除方案: ") + solutionName);

    if (!SolutionManager::instance()->solutions().empty())
    {
        setSolution(SolutionManager::instance()->solutions().first());
    }
}

void EditSolution::onSaveSolution()
{
    auto sol = currentSolution();
    qDebug() << "onSaveSolution:" << sol->toString();
    if (sol == nullptr)
    {
        QMessageBox::warning(this, tr("错误"), tr("保存方案失败"));
        return;
    }
    updateStatus(tr("正在保存方案: ") + sol->name);

    if (!SolutionManager::instance()->saveSolution(sol->id))
    {
        updateStatusError(tr("保存方案失败:") + sol->name);
        return;
    }

    setSolution(SolutionManager::instance()->getSolution(sol->id));
    updateStatus(tr("方案已保存: ") + sol->name);
}

// 图像操作槽函数
void EditSolution::onCaptureImage()
{
    emit captureRequested();
}

void EditSolution::onOpenImage()
{
    QString filePath = QFileDialog::getOpenFileName(this, "打开图像", "", "图像文件 (*.png *.jpg *.bmp)");
    if (!filePath.isEmpty())
    {
        QPixmap pixmap(filePath);
        setImage(pixmap.toImage());
    }
}

void EditSolution::onSaveImage()
{
    if (currentSolutionImage().isNull())
    {
        updateStatusError(tr("没有可保存的图像"));
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "保存图像", "", "PNG图像 (*.png);;JPEG图像 (*.jpg)");
    if (!filePath.isEmpty())
    {
        if (currentSolutionImage().save(filePath))
        {
            updateStatus(tr("图像已保存: ") + QFileInfo(filePath).fileName());
        }
        else
        {
            updateStatusError(tr("保存图像失败"));
        }
    }
}

// ROI管理槽函数
void EditSolution::onDrawROI()
{
    QString shape = ui->shapesCombo->currentText();
    QString shapeType = ui->shapesCombo->currentData().toString();
    qDebug() << "ROI形状: " << shape << shapeType;
    updateStatus(tr("绘制ROI形状: ") + shape);

    ROI roi;
    roi.type = shapeType;
    roi.color = QColor(Qt::green);
    if (QString::compare(shapeType, "rectangle", Qt::CaseInsensitive) == 0)
    {
        roi.points = {QPointF(0, 0), QPointF(80, 60)};
    }
    else if (QString::compare(shapeType, "ellipse", Qt::CaseInsensitive) == 0)
    {
        roi.points = {QPointF(0, 0), QPointF(80, 60)};
    }
    else if (QString::compare(shapeType, "polygon", Qt::CaseInsensitive) == 0)
    {
        roi.points = {QPointF(0, 0), QPointF(80, 60), QPointF(0, 60), QPointF(80, 0)};
    }
    else
    {
        updateStatusError(tr("无效的ROI类型:") + shape);
        return;
    }

    SolutionManager::instance()->addROI(currentSolutionId(), roi);
    setROIs(currentSolution()->rois.values());
}

void EditSolution::onTableItemClicked(QTableWidgetItem *item)
{
    // emit ROISelected(ui->tableWidget->item(item->row(), 0)->text().toInt());
    m_selectedROIId = ui->tableWidget->item(item->row(), 0)->text().toInt();
    qDebug() << "EditSolution::onTableItemClicked:" << item->text() << "selectedROIId:" << m_selectedROIId;
    setROIs(currentSolution()->rois.values());
}
void EditSolution::onTableItemChanged(QTableWidgetItem *item)
{
    const int row = item->row();
    const int column = item->column();
    auto headerItem = ui->tableWidget->horizontalHeaderItem(column);
    qDebug() << "ROI Table item changed: " << headerItem->text() << ", value=" << item->text();

    bool ok;
    auto roiId = ui->tableWidget->item(row, 0)->text().toInt(&ok);
    auto &roi = currentSolution()->rois[roiId];

    if (column == 0) // ID
    {
    }
    else if (column == 1) // 标签
    {
        roi.label = item->text();
    }
    else if (column == 2) // 类型
    {
    }
    else if (column == 3) // 阈值
    {
        double threshold = item->text().toDouble(&ok);
        if (ok && threshold >= 0 && threshold <= 1)
        {
            roi.threshold = threshold;
        }
        else
        {
            QMessageBox::warning(this, tr("错误"), tr("阈值必须在0-1之间!"));
            item->setText(QString::number(roi.threshold)); // 恢复默认值
        }
    }
    else if (column == 4) // 操作
    {
    }
    setROIs(currentSolution()->rois.values());
}

void EditSolution::onDeleteROIClicked()
{
    qDebug() << "onDeleteROIClicked:" << sender();

    QToolButton *button = qobject_cast<QToolButton *>(sender());
    if (!button)
    {
        qDebug() << "delete ROI button is null";
        updateStatusError(tr("删除ROI按钮为空"));
        return;
    }

    int row = button->property("row").toInt();
    qDebug() << "onDeleteROIClicked: row=" << row;
    auto roiId = ui->tableWidget->item(row, 0)->text().toInt();

    if (QMessageBox::Yes != QMessageBox::question(this, tr("确认删除"), tr("确定要删除ROI: ") + roiId + "?",
                                                  QMessageBox::Yes | QMessageBox::No))
    {
        qDebug() << "delete ROI canceled";
        return;
    }

    if (auto sol = currentSolution())
    {
        sol->rois.remove(roiId);
        setROIs(sol->rois.values());
        updateStatus(tr("已删除ROI: %1").arg(roiId));
    }
    else
    {
        qWarning() << "current solution is null";
        updateStatusError(tr("当前方案为空"));
    }
}

QString EditSolution::currentSolutionId() const
{
    return ui->templatesCombo->currentData().toString();
}

Solution *EditSolution::currentSolution()
{
    return SolutionManager::instance()->getSolution(currentSolutionId());
}
QImage EditSolution::currentSolutionImage()
{
    if (m_backgroundItem)
        return m_backgroundItem->pixmap().toImage();
    return QImage();
}
