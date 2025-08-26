#ifndef COMMONZACK_H
#define COMMONZACK_H
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QApplication>
#include <QWidget>
#include <QtCore>
#include <Qt>
#include <QtGui>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QToolButton>
#include <QComboBox>
#include <QTextEdit>
#include <QGridLayout>
#include <QTabWidget>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QStackedLayout>
#include <QProgressBar>
#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>
#include <QTimer>
//#include "qsqlquery.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QScrollBar>
#include <QVariant>
#include <QVariantList>
#include <QMenu>
#include <QVariant>
#include <QScrollArea>
#include <QIntValidator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QList>
#include <QObject>
#include <QStyleOptionHeader>
//#include <QtConcurrent>
#include <QFileDialog>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QGraphicsTextItem>
#include <QItemDelegate>
#include <QCalendarWidget>

#include <iostream>
#include <vector>
#include <thread>
#include <string.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <queue>
#include <opencv2/opencv.hpp>

#define ArrayLength 10

/////////////////////////////
class MyWidget : public QWidget
{
    Q_OBJECT

public:
    MyWidget(QWidget* parent=0, QString qss="border: 0px;");

protected:
    void paintEvent(QPaintEvent *event);
};

class MyTitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit MyTitleBar(QWidget *parent = nullptr, QList<bool> show_buttons={true, true, true}, QString qss="", QString title="AI检测");
    ~MyTitleBar();

    void set_iconMin(QString path);
    void set_iconMax(QString path);
    void set_iconClose(QString path);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    bool   m_isPressed = false;
    QPoint m_mousePressPoint;
    QPoint m_windowPosition;

    QLabel *label_icon;
    QLabel *label_title;
    QToolButton *toolButton_min;
    QToolButton *toolButton_max;
    QToolButton *toolButton_close;

private slots:
    void toolButton_min_clicked();
    void toolButton_max_clicked();
    void toolButton_close_clicked();
};

extern QString g_FontTitle;
QString getMyFont();

// Widget
QWidget* newWidget(QWidget* parent=nullptr);

QLabel* newLabel(QWidget* parent, QString text="", int w=0, int h=0,
                 QString qssStr="border: none;");
QLineEdit* newLineEdit(QWidget* parent, QString text="", int w=0, int h=0, QList<int> validator={0, 0},
                       QString qssStr="border:2px solid rgb(119,183,244); background-color:transparent; padding-left: 8px;", QString slotName="", QVariantList args={});
QCheckBox* newCheckBox(QWidget* parent, QString text="", int w=0, int h=0, bool isChecked=false,
                       QString qssStr="QCheckBox{border: 0px;} QCheckBox::indicator {width: 20px; height: 20px; outline: none;} QCheckBox::checked{color: rgb(0，0，0);} QCheckBox::unchecked{color: rgb(255,165,0);}");
QRadioButton* newRadioButton(QWidget* parent, QString text, int w, int h, bool isChecked,
                          QString qssStr="border: none;");
QPushButton* newPushButton(QWidget* parent, QString text="", QString tooltip="", int w=0, int h=0,
                           QString qssStr="", QString qssStr2="QPushButton{border:2px solid rgb(119,183,244);}  QPushButton:hover{background-color:rgb(119,183,244);}");
QToolButton* newToolButton(QWidget* parent, QString text="", QString tooltip="", int w=0, int h=0, QString iconPath="", bool checkable=true, bool isChecked=false, Qt::ToolButtonStyle style=Qt::ToolButtonTextOnly,
                           QString qssStr="", QString qssStr2="QToolButton{border:2px solid rgb(119,183,244);}  QToolButton:hover{background-color:rgb(119,183,244);}");
QComboBox* newComboBox(QWidget* parent, QList<QString> texts={}, QString defaultText="", int w=0, int h=0, int defaultIdx=1,
                       QString qssBorder="rgb(119,183,244)");
QTextEdit* newTextEdit(QWidget* parent, int w=0, int h=0, bool isReadOnly=true);
QTabWidget* newTabWidget(QWidget* parent, QStringList texts, QStringList tooltips={},int w=0, int h=0, QTabWidget::TabPosition loc=QTabWidget::North,
                         QString qssStr="");
QProgressBar* newProgressBar(QWidget* parent, int w=0, int h=0, int valueCur=0, QList<int> range={0, 100},
                             Qt::Orientation orientation=Qt::Horizontal, Qt::Alignment align=Qt::AlignLeft,
                             QString format="%p%", bool textVisible=true);

QDateTimeEdit* newDateTimeEdit(QWidget* parent, QString tooltip, int w=0, int h=0, QList<int> days={-1, 1}, bool isPopup=false,
                               QString qssStr="border: 1px solid rgb(119,183,244);", QString displayFormat="yyyy-MM-dd hh:mm:ss");
QTimeEdit* newTimeEdit(QWidget* parent, QString tooltip, int w=0, int h=0, QTime time=QTime(00,00,00), bool isPopup=false,
                       QString qssStr="border: 1px solid rgb(119,183,244);", QString displayFormat="yyyy-MM-dd hh:mm:ss");


QIcon newIcon(QString path, int w=0, int h=0);

QFrame* newSplit(int w, int h);

void newHBoxLayout(QWidget* parent, QList<QWidget*> widgets, int space=0, QList<int> margins={0,0,0,0}, Qt::Alignment align=Qt::AlignCenter);

void newVBoxLayout(QWidget* parent, QList<QWidget*> widgets, int space=0, QList<int> margins={0,0,0,0}, Qt::Alignment align=Qt::AlignCenter);


QMessageBox* newMessageBox(QWidget* parent, QString title, QString text, QMessageBox::StandardButtons buttons=QMessageBox::Ok, QString qss="QLabel {font-size: 18px;} QPushButton{border: 1px solid rgb(119,183,244); font-size: 18px;}");
bool MessageBoxIsPressedYes(const QString& text, QWidget* parent=nullptr, const QString& boxType="提示");

///////////////
QGraphicsTextItem* newGraphicsTextItem();

void loadQSS_all(QWidget* parent, const QString& qss);

// 功能
bool endsWith(std::string const &str, std::string const &suffix);

QImage Mat2QImage(const cv::Mat& m);

cv::Mat QImage2Mat(const QImage& image);
QString Int2QString(int num, int countsFor0=6);


QJsonObject readJson(const QString& path);
void writeJson(const QJsonObject& root, const QString& path);

void checkFilesForDate(QDateEdit* dateEdit, const QString &folderPath);


//////////////
class ThreadPool
{
public:
    ThreadPool(size_t threads, int cpu_id, int priority = 99)
    {
        for (size_t i = 0; i < threads; ++i)
            workers.emplace_back(
                [this, cpu_id, priority] {
                    // 设置 CPU 亲和性
                    cpu_set_t mask;
                    CPU_ZERO(&mask);
                    CPU_SET(cpu_id, &mask);
                    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
                        std::cerr << "Error setting CPU affinity" << std::endl;
                    }

                    // 设置调度策略和优先级
                    struct sched_param param;
                    param.sched_priority = priority;
                    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
                        std::cerr << "Error setting scheduler" << std::endl;
                    }

                    for (;;) {
                        std::function<void()> task;

                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                            if (this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }

                        task();
                    }
                }
        );
    }

    void stopPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all(); // 通知所有线程停止
        for (std::thread& worker : workers)
            worker.join(); // 等待所有线程完成
    }

    ~ThreadPool() {
        stopPool();
    }

    // void不返回
    template<class F, class... Args>
    void enqueue1(F&& f, Args&&... args)
    {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");

            tasks.emplace(task);
        }
        condition.notify_one();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;
};

////////////////
class SystemUsedWidget : public QWidget
{
    Q_OBJECT

public:
    SystemUsedWidget(QWidget* parent=0);
    ~SystemUsedWidget() {};

    void updateSystemUsad(bool isStarted);

    void updateShow(int cpu, int mem, int disk, int gpu);
    void updateShowMem(const QString& path="/data");

private:
    void init_UI();

    int getCPUUsage();
    int getMemoryUsage();
    void getDiskSpace(double& usedMemGB, double& totalMemGB, const std::string& path="/media/quecDatas");//"/home/q");//
    int getGPUUsage();


    QProgressBar* _bar1;
    QProgressBar* _bar2;
    QProgressBar* _bar3;
    QLabel*       _label3;
    QProgressBar* _bar4;
};

/////////////
// 定义登录对话框
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    LoginDialog(QWidget *parent = nullptr, QList<bool> show={true, true, true}, QList<QString> text={"请输入账号", "请输入密码", "登录"});

    QString getUsername() const;
    QString getPassword() const;

private slots:
    void onLoginClicked();

private:
    QComboBox *box;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
};

//////////////
class MyLabel : public QLabel{
    Q_OBJECT
public:
    MyLabel(QWidget* parent=0, bool isClickable=false);
    ~MyLabel() {};
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    bool m_isClickable;
signals:
    void emit_text(const QString& text);
};
class HistoryLabelView : public QWidget
{
    Q_OBJECT
public:
    HistoryLabelView(QWidget *parent = nullptr, QList<QString> headers={});
    ~HistoryLabelView() {};

    void updateUI(int pushNum, const QList<QString> &result_each, const QString& path);
    int  getCount();
protected:
//    void resizeEvent(QResizeEvent* event) override;
private:
    QHBoxLayout*    v_layout;
    QList<QWidget*> v_widgets_all;
    QList<QList<MyLabel*>> v_labelsTable_all;
    int m_rownCount;
    int m_columnCount;
    int m_col = 0;
    QList<QString> whoIsOK;
    QPixmap v_pixOK;
    QPixmap v_pixNG;

    void Init_UI();
    void updateColumn(int pushNum, const QList<bool>& infos_bool, const QList<QString>& infos, const QString& qss_ok, const QString& qss_ng);
signals:
    void emit_text_dir(QString dir);
};
    

/////////////////
class MyDateEdit : public QDateEdit
{
    Q_OBJECT

public:
    MyDateEdit(QWidget* parent = nullptr);
    ~MyDateEdit() {};
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
};
class NoElideDelegate : public QItemDelegate {
    Q_OBJECT

public:
    NoElideDelegate(QObject *parent = nullptr) : QItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem newOption = option;
        newOption.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;  // 设置文本对齐方式
        QItemDelegate::paint(painter, newOption, index);
    }
};



#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>
#include <QVariantMap>
#include <QVariantList>
class DatabaseManager : public QObject {
    Q_OBJECT

public:
    explicit DatabaseManager(const QString& connectionName, QObject *parent = nullptr);
    ~DatabaseManager();

    bool init(const QString& path);  // 初始化数据库
    bool switchDatabase(const QString& connection_name, const QString& dbPath);

    bool create(const QString& table, const QStringList& columnNames, const QStringList& columnTypes);
    bool insert(const QString& table, const QVariantMap& data);
    bool update(const QString& table, const QVariantMap& conditions, const QVariantMap& updates);
    bool remove(const QString& table, const QVariantMap& conditions);
    int  getCounts(const QString& table);
    QVariantList getRow(const QString& table, const QString& idName, const QString& idValue);
    QList<QVariantList> getLastNRows(const QString& table, int start=0, int length=20);
    QList<QVariantList> getLastNRows(const QString& table, int N, const QList<QString>& columns);

    QList<QVariantList> findRows(const QString& table, const QString& columnName, const QString& value);
    QList<QVariantList> findRowsByTimeRange(const QString& table, const QString& timeColumn, const QString& startTime, const QString& endTime);
    QList<QVariantList> findRowsByDate(const QString& table, const QString& dateColumn, const QDateTime& startDate, const QDateTime& endDate);

    QVariantList query(const QString& queryStr);
    bool executeQuery(QSqlQuery& query, const QVariantMap& bindValues);

    // 新增方法：获取表的列名
    QStringList getTableColumns(const QString& tableName);
    // 传入 columnCount 为 -1，则默认导出所有列。如果传入了具体的列数，那么就只导出前 columnCount 列的数据
    // conditions：可选条件。如果你不需要过滤条件，可以传递一个空的 QVariantMap
    void exportTableData(QTextStream& out, const QString& tableName, QVariantMap conditions={}, int columnCount = -1);
private:
    QSqlDatabase db;
    QString connectionName;
};





#endif // COMMONZACK_H
