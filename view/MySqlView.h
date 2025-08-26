#ifndef MYSQLVIEW_H
#define MYSQLVIEW_H

#include <QWidget>
#include "commonZack.h"
#include "QtMyGroupBox.h"

namespace Ui {
class MySqlView;
}


class MySqlView : public QWidget
{
    Q_OBJECT

public:
    explicit MySqlView(QWidget *parent = nullptr, QString qss="");
    ~MySqlView();

    static QList<QString> g_sql_headers_ENG;
    static QList<QString> g_sql_headers;
    static DatabaseManager g_sqlManager;
    static DatabaseManager g_sqlManager_tmp;

    void updateOne(int start);
    void update_tableWidget(int start);
    void update_tableWidget1(QList<QVariantList> lastNRows);

private:
    Ui::MySqlView *ui;
    int m_rows;

    MyDateEdit* v_dateEdit;
    QDate m_currentDate;
    bool  m_isCurrentDate;
    QTimeEdit* v_timeEdit_start;
    QTimeEdit* v_timeEdit_end;
    QLineEdit* v_lineEdit_search;
    QToolButton*  v_button_saveCurrentCSV;

    int m_page = 0;
    int m_pageTotal = 0;
    bool m_is_table_in_page1 = true;

private slots:
    void Init_UI();
    void Init_Sqlite();
    void Init_tableWidget(QTableWidget* tableWidget);

    void tableWidget_clicked(int row, int col);
    void toolButton_saveCurrentCSV_clicked();

    void dateEdit_day_calendarPopup(MyDateEdit* dateEdit);
    void dateEdit_day_dateChanged(const QDate& date);

    void on_comboBox_select_currentIndexChanged(int index);
    void toolButton_search_clicked();
    void toolButton_refresh_clicked();
    void toolButton_first_clicked();
    void toolButton_end_clicked();
    void toolButton_lastone_clicked();
    void toolButton_nextone_clicked();
    void updatePage(int page_idx);
    void label_page_returnPressed();

signals:
    void emit_log(QString text);
};



#endif // MYSQLVIEW_H
