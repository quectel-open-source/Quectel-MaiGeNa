#include <QApplication>
#include "MainWindow.h"
#include "appcontext.h"
#include "inspectionrecordrepo.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icon/Resources/main-1.ico"));

    // 注册服务
    AppContext::registerService<InspectionRecordRepo>(new InspectionRecordRepo(&DBManager::instance()));
    AppContext::loadTheme("darkstyle");

    // 
    getMyFont();
    QRect desktop = QApplication::desktop()->screenGeometry();
    MainWindow w;
    w.resize(1400, 900);
    w.move(desktop.center() - w.rect().center());
    w.show();

    qDebug() << "Max threads:" << QThreadPool::globalInstance()->maxThreadCount();
    qDebug() << "Active threads:" << QThreadPool::globalInstance()->activeThreadCount();
    return a.exec();
}
