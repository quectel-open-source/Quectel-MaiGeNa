#include "MySqlView.h"
#include "ui_MySqlView.h"


QList<QString> MySqlView::g_sql_headers_ENG = {"SN", "timeCur", "loc_1", "loc_2", "loc_3", "loc_4", "loc_5", "result", "countTotal", "countOK", "countNG", "path"};
QList<QString> MySqlView::g_sql_headers = {"SN号", "时间",  "位置 1", "位置 2", "位置 3", "位置 4", "位置 5", "结果", "总数", "OK数", "NG数", "图像位置"}; // 汇总
DatabaseManager MySqlView::g_sqlManager(QDate::currentDate().toString("yyyy-MM-dd"));
DatabaseManager MySqlView::g_sqlManager_tmp(QDate::currentDate().toString("history"));
//////////////////////
MySqlView::MySqlView(QWidget *parent, QString qss) : QWidget(parent), ui(new Ui::MySqlView)
{
    ui->setupUi(this);

    v_dateEdit = new MyDateEdit(this);
    v_dateEdit->setObjectName("table");
//    v_dateEdit->setReadOnly(true);
    v_dateEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(v_dateEdit, &QDateEdit::editingFinished, this, [=](){ dateEdit_day_calendarPopup(v_dateEdit); });
    connect(v_dateEdit, QOverload<const QDate&>::of(&QDateEdit::dateChanged), this, &MySqlView::dateEdit_day_dateChanged);
    v_dateEdit->setDate(QDate::currentDate());  // 设置为当前日期
    ui->verticalLayout_2->addWidget(v_dateEdit);

    m_currentDate = QDate::currentDate();
    QTimer::singleShot(100, this, [=]{Init_Sqlite();});
    Init_UI();
    Init_tableWidget(ui->tableWidget);
    loadQSS_all(this, qss);
}
MySqlView::~MySqlView()
{
    delete ui;
}
void MySqlView::dateEdit_day_calendarPopup(MyDateEdit* dateEdit)
{
    // 查找文件夹下所有文件夹, 存在的添加到QDateEdit
    checkFilesForDate(dateEdit, "/data/Results");
}
void MySqlView::dateEdit_day_dateChanged(const QDate& date)
{
    if (this->parentWidget()->isHidden())
        return;
    QString path =  "/data/Results/" + date.toString("yyyy-MM-dd") + "/sql.db";
    qDebug() << ">> date:" << date;
    qDebug() << "  sql current select file: " << path;
    if (m_currentDate == date){
//        qDebug() << ">> common date: " << date << ", return";
        return;
    }
    QFile file(path);
    if (!file.exists()){
        v_dateEdit->setDate(m_currentDate);
        return;
    }
    if (date == QDate::currentDate()){
        m_isCurrentDate = true;
    }
    else{
        m_isCurrentDate = false;
        g_sqlManager_tmp.init(path);
    }
    update_tableWidget(0);
    m_currentDate = date;
}

void MySqlView::Init_UI()
{
    m_rows = 20;

    int w1 = 110;
    int w2 = w1 * 2 + 30;
    int h1 = 32;
    QFont font = QFont("Microsoft YaHei", 11);
    v_timeEdit_start = newTimeEdit(ui->page_0, "设置起始时间", w1, h1, QTime(00,00,00));
    v_timeEdit_start->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    v_timeEdit_start->setDisplayFormat("HH:mm:ss");
    v_timeEdit_start->setFont(font);
    v_timeEdit_start->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    v_timeEdit_start->setStyleSheet("border: 2px solid rgb(128, 204, 244); padding-right: 10px;");
    v_timeEdit_end = newTimeEdit(ui->page_0, "设置结束时间", w1, h1, QTime(23,59,59));
    v_timeEdit_end->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    v_timeEdit_end->setDisplayFormat("HH:mm:ss");
    v_timeEdit_end->setFont(font);
    v_timeEdit_end->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    v_timeEdit_end->setStyleSheet("border: 2px solid rgb(128, 204, 244); padding-right: 10px;");
    QLabel* label = newLabel(ui->page_0, "~", 15, h1);
    label->setFont(QFont("Microsoft YaMei", 10));
    newHBoxLayout(ui->page_0, {v_timeEdit_start, label, v_timeEdit_end}, 5, {4,2,4,2}, Qt::AlignLeft | Qt::AlignVCenter);
    ui->page_0->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    v_lineEdit_search = newLineEdit(ui->page_1, "", w2-10, h1);
    v_lineEdit_search->setFont(font);
    v_lineEdit_search->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    v_lineEdit_search->setStyleSheet("QLineEdit{border: 2px solid rgb(128, 204, 244); padding-left: 10px;}");
    newHBoxLayout(ui->page_1, {v_lineEdit_search}, 0, {4,2,4,2}, Qt::AlignLeft | Qt::AlignVCenter);
    ui->page_1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->stackedWidget->setMinimumWidth(w2);

    v_button_saveCurrentCSV = newToolButton(this, "另存为CSV", "", -1, h1, ":/file/Resources/saveas1.png", false, false, Qt::ToolButtonTextBesideIcon);
    v_button_saveCurrentCSV->setIconSize(QSize(24, 24));
    v_button_saveCurrentCSV->setFont(font);
//    v_button_saveCurrentCSV->setObjectName("main2");
    connect(v_button_saveCurrentCSV, &QToolButton::clicked, this, &MySqlView::toolButton_saveCurrentCSV_clicked);
    ui->horizontalLayout_6->addWidget(v_button_saveCurrentCSV);
}
void MySqlView::Init_Sqlite()
{
    QList<QString> sqlLabels_Type = {"VARCHAR(100) NOT NULL PRIMARY KEY", "TEXT",
                                     "VARCHAR(100)", "VARCHAR(100)", "VARCHAR(100)", "VARCHAR(100)", "VARCHAR(100)",
                                     "VARCHAR(100)", "INT", "INT", "INT", "VARCHAR(100)"};
    QString path =  "/data/Results/" + QDate::currentDate().toString("yyyy-MM-dd") + "/sql.db";
    if (g_sqlManager.init(path)){
        emit emit_log("连接数据库成功!");
    } else {
        qDebug() << "连接数据库失败...";
    }

    if (g_sqlManager.create("tableTotal", g_sql_headers_ENG, sqlLabels_Type)){
        emit emit_log("数据库创建成功!");
    } else {
        qDebug() << "数据库创建失败...";
    }
    m_isCurrentDate = true;

    connect(ui->toolButton_first, &QToolButton::clicked, this, &MySqlView::toolButton_first_clicked);
    connect(ui->toolButton_end, &QToolButton::clicked, this, &MySqlView::toolButton_end_clicked);
    connect(ui->toolButton_lastone, &QToolButton::clicked, this, &MySqlView::toolButton_lastone_clicked);
    connect(ui->toolButton_nextone, &QToolButton::clicked, this, &MySqlView::toolButton_nextone_clicked);
    connect(ui->toolButton_search, &QToolButton::clicked, this, &MySqlView::toolButton_search_clicked);
    connect(ui->toolButton_refresh, &QToolButton::clicked, this, &MySqlView::toolButton_refresh_clicked);
    connect(ui->label_page, &QLineEdit::returnPressed, this, &MySqlView::label_page_returnPressed);
}

void MySqlView::Init_tableWidget(QTableWidget* tableWidget)
{
    for(int i=1; i<g_sql_headers.count(); i++){
        if (i <= 6 || i == 10) {
            ui->comboBox_select->addItem(g_sql_headers[i]);
        }
    }

    tableWidget->setObjectName("table");
    tableWidget->setRowCount(m_rows);
    tableWidget->setColumnCount(g_sql_headers.count());
    tableWidget->setHorizontalHeaderLabels(g_sql_headers);
    // 设置表头字体加粗
    QFont headerFont = tableWidget->horizontalHeader()->font();
    headerFont.setPointSize(13);
    headerFont.setBold(true);
    tableWidget->horizontalHeader()->setFont(headerFont);

    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setColumnWidth(0, 80);
    tableWidget->setColumnWidth(1, 200);
    for(int i=2; i<g_sql_headers.count(); i++){
        tableWidget->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }
    tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(tableWidget, &QTableWidget::cellClicked, this, &MySqlView::tableWidget_clicked);

}
void MySqlView::tableWidget_clicked(int row, int col)
{
    static QString SN_last = "";
    QString SN = ui->tableWidget->item(row, 0)->text().trimmed();
    if (SN_last != SN) {
        QString path = ui->tableWidget->item(row, ui->tableWidget->columnCount() - 1)->text().trimmed();
        QFile file(path);
        if (file.exists()){
            QImage qimage(path);
            ui->graphicsView->DispImage(qimage);

            if (col == ui->tableWidget->columnCount() - 1){
                QProcess::startDetached("xdg-open", {path});
            }
        }
    }
    SN_last = SN;
}
void MySqlView::update_tableWidget(int start)
{
    DatabaseManager& mysql = m_isCurrentDate ? g_sqlManager : g_sqlManager_tmp;
    QList<QVariantList> lastNRows = mysql.getLastNRows("tableTotal", start, m_rows);

    update_tableWidget1(lastNRows);
}
void MySqlView::update_tableWidget1(QList<QVariantList> lastNRows)
{
    QFont font("Microsoft YaHei", 11);
    int rowCount = ui->tableWidget->rowCount();
    int m_sizeReal = lastNRows.count();
    QString text;
    int rowIs;
    int cols_cnt = ui->tableWidget->columnCount();
    for (int row = 0; row < rowCount; ++row) {
        QVariantList values;
        if (row < m_sizeReal){
            values = lastNRows[row];
        }
        for (int col = 0; col < cols_cnt; ++col) {
            if (row < m_sizeReal){
                text = values[col].toString();
                rowIs = m_sizeReal - 1 - row;
            }
            else{
                text = "";
                rowIs = rowCount - 1 + m_sizeReal - row;
            }
            QTableWidgetItem* item = new QTableWidgetItem(text == "OK" ? "" : text);
            if (col == 0){
                item->setFont(font);
            }
            else if (col == 7){
                if (text == "NG"){
                    item->setBackgroundColor(QColor(225, 64, 51));
                }
                else if (text == "OK"){
                    item->setBackgroundColor(QColor(102, 204, 102));
                }
                item->setFont(font);
            }
            item->setToolTip(text);
            item->setTextAlignment(Qt::AlignCenter);  // 设置数目与占比列居中
            NoElideDelegate* delegate = new NoElideDelegate(ui->tableWidget);
            ui->tableWidget->setItemDelegate(delegate);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            ui->tableWidget->setItem(rowIs, col, item);
        }
    }
}

void MySqlView::toolButton_saveCurrentCSV_clicked()
{
    int reply = QMessageBox::information(this, "保存为CSV", "是否导出今日数据为CSV文件？", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply != QMessageBox::Yes){
        return;
    }
    QString dat = m_currentDate.toString("yyyy-MM-dd");
    QString csv_path = "/data/Results/" + dat + "/缺陷统计_" + dat + ".csv";
    // 打开文件进行写入
    QFile file(csv_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "打开失败  " + csv_path;
        return;
    }

    QTextStream out(&file);
    DatabaseManager& mysql = m_isCurrentDate ? g_sqlManager : g_sqlManager_tmp;

    // 获取表的列名
    QStringList columns = mysql.getTableColumns("tableTotal");

    // 只取前14列
    int columnCount = qMin(columns.size(), 12);  // 如果列数少于15，取实际的列数

    // 写入 CSV 文件的表头
    for (int i = 0; i < columnCount; ++i) {
        out << g_sql_headers[i];
        if (i != columnCount - 1) out << ",";
    }
    out << "\n";  // 换行

    mysql.exportTableData(out, "tableTotal");

    file.close();
    emit emit_log("输出csv：" + csv_path);
    QMessageBox::information(this, "提示", "导出CSV成功：\n " + csv_path);
}

///////////
void MySqlView::updateOne(int start)
{
    if (m_is_table_in_page1) {
        update_tableWidget(0);
    }

    DatabaseManager& mysql = m_isCurrentDate ? g_sqlManager : g_sqlManager_tmp;
    QList<QVariantList> lastRow = mysql.getLastNRows("tableTotal", 1, {"countTotal"});
    int total = lastRow.empty() ? 0 : lastRow[0][0].toInt();
    m_page = 1;
    m_pageTotal = (total % m_rows == 0) ? total / m_rows : total / m_rows + 1;
    ui->label_cnts->setText((QString("共%1项，共%2页").arg(total).arg(m_pageTotal)));
    ui->label_page->setText(QString::number(m_page));
}


void MySqlView::on_comboBox_select_currentIndexChanged(int index)
{
    if (index == 0){
        ui->stackedWidget->setCurrentWidget(ui->page_0);
    }
    else{
        ui->stackedWidget->setCurrentWidget(ui->page_1);
    }
}
void MySqlView::toolButton_search_clicked()
{
    QString text = ui->label_cnts->text();
    if (text.isEmpty() || text.isNull() || text == "0")
        return;

    m_is_table_in_page1 = false;
    int index = ui->comboBox_select->currentIndex();
    QList<QVariantList> datas;
    if (index == 1){
        QString time_start = v_timeEdit_start->text();
        QString time_end   = v_timeEdit_end->text();

        DatabaseManager& mysql = m_isCurrentDate ? g_sqlManager : g_sqlManager_tmp;
        datas = mysql.findRowsByTimeRange("tableTotal", g_sql_headers_ENG[index], time_start, time_end);
    }
    else{
        DatabaseManager& mysql = m_isCurrentDate ? g_sqlManager : g_sqlManager_tmp;
        datas = mysql.findRows("tableTotal", g_sql_headers_ENG[index], v_lineEdit_search->text().trimmed());
    }

    update_tableWidget1(datas);
}
void MySqlView::toolButton_refresh_clicked()
{
    QString text = ui->label_cnts->text();
    if (text.isEmpty() || text.isNull() || text == "0")
        return;

    update_tableWidget(0);
    m_is_table_in_page1 = true;
}

void MySqlView::updatePage(int page_idx)
{
    update_tableWidget((page_idx - 1) * m_rows);
    ui->label_page->setText(QString::number(page_idx));
}
void MySqlView::toolButton_first_clicked()
{
    if (m_pageTotal <= 0) return;
    m_page = 1;
    updatePage(m_page);
}
void MySqlView::toolButton_end_clicked()
{
    if (m_pageTotal <= 0) return;
    m_page = m_pageTotal;
    updatePage(m_page);
}
void MySqlView::toolButton_lastone_clicked()
{
    if (m_pageTotal <= 0) return;
    m_page--;
    if (m_page <= 0)  m_page = 1;
    updatePage(m_page);
}
void MySqlView::toolButton_nextone_clicked()
{
    if (m_pageTotal <= 0) return;
    m_page++;
    if (m_page > m_pageTotal)  m_page = m_pageTotal;
    updatePage(m_page);
}
void MySqlView::label_page_returnPressed()
{
    bool is_ok = true;
    int page = ui->label_page->text().toInt(&is_ok);
    if (!is_ok || page <= 0 || page >m_pageTotal){
        QMessageBox::warning(this, "错误", "请输入正确的页码");
        return;
    }
    m_page = page;
    updatePage(m_page);
}
