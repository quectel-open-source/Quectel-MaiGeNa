#include "../view/dbmanager.h"
#include "../view/inspectionrecordrepo.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTest>

class TestInspectionRecordRepo : public QObject
{
    Q_OBJECT

  private:
    DBManager *dbManager;
    InspectionRecordRepo *repo;

  private slots:
    void initTestCase()
    {
        dbManager = new DBManager("testdb");
        repo = new InspectionRecordRepo(dbManager);
    }

    void cleanupTestCase()
    {
        delete repo;
        delete dbManager;
    }

    void testInsertRecord()
    {
        InspectionRecord record;
        record.sn = "TEST-001";
        record.costTime = 123;
        record.status = 0;
        record.total = 10;
        record.okCount = 9;
        record.ngCount = 1;
        record.result = "PASS";
        record.path = "/path/to/image.jpg";

        bool result = repo->insert(record);
        QVERIFY(result);

        // 验证记录是否真的插入
        QSqlQuery query =
            dbManager->query("SELECT * FROM inspection_record WHERE sn = ?", QVariantList() << "TEST-001");
        QVERIFY(query.next());
        QCOMPARE(query.value("sn").toString(), record.sn);
        QCOMPARE(query.value("costTime").toInt(), record.costTime);
        QCOMPARE(query.value("status").toInt(), record.status);
        // 更多字段验证...
    }

    void testUpdateRecord()
    {
        // 先插入一条记录
        InspectionRecord record;
        record.sn = "TEST-002";
        record.status = 0;
        repo->insert(record);

        // 查询获取ID
        QSqlQuery query =
            dbManager->query("SELECT id FROM inspection_record WHERE sn = ?", QVariantList() << "TEST-002");
        QVERIFY(query.next());
        int id = query.value("id").toInt();

        // 更新记录
        record.id = id;
        record.status = 1; // 从OK改为NG
        record.costTime = 200;
        bool result = repo->update(record);
        QVERIFY(result);

        // 验证更新结果
        query = dbManager->query("SELECT * FROM inspection_record WHERE id = ?", QVariantList() << id);
        QVERIFY(query.next());
        QCOMPARE(query.value("status").toInt(), 1);
        QCOMPARE(query.value("costTime").toInt(), 200);
    }

    void testRemoveRecord()
    {
        // 先插入一条记录
        InspectionRecord record;
        record.sn = "TEST-003";
        repo->insert(record);

        // 查询获取ID
        QSqlQuery query =
            dbManager->query("SELECT id FROM inspection_record WHERE sn = ?", QVariantList() << "TEST-003");
        QVERIFY(query.next());
        int id = query.value("id").toInt();

        // 删除记录
        bool result = repo->remove(id);
        QVERIFY(result);

        // 验证记录已删除
        query = dbManager->query("SELECT * FROM inspection_record WHERE id = ?", QVariantList() << id);
        QVERIFY(!query.next());
    }

    void testQueryAll()
    {
        // 先清空表
        dbManager->execute("DELETE FROM inspection_record");

        // 插入几条记录
        InspectionRecord record1;
        record1.sn = "TEST-101";
        repo->insert(record1);

        InspectionRecord record2;
        record2.sn = "TEST-102";
        repo->insert(record2);

        // 查询所有记录
        QList<InspectionRecord> records = repo->queryAll();
        QCOMPARE(records.size(), 2);
    }

    void testQueryByFilter()
    {
        // 先清空表
        dbManager->execute("DELETE FROM inspection_record");

        // 插入测试数据
        InspectionRecord record1;
        record1.sn = "TEST-201";
        record1.status = 0;
        record1.createdAt = QDateTime::currentDateTime().addDays(-1);
        repo->insert(record1);

        InspectionRecord record2;
        record2.sn = "TEST-202";
        record2.status = 1;
        record2.createdAt = QDateTime::currentDateTime();
        repo->insert(record2);

        // 测试按状态筛选
        QDateTime startDate = QDateTime::fromString("2000-01-01", "yyyy-MM-dd");
        QDateTime endDate = QDateTime::fromString("2100-01-01", "yyyy-MM-dd");

        auto result = repo->queryByFilter("TEST-201", startDate, endDate);
        QCOMPARE(result.first, 1);         // 总数
        QCOMPARE(result.second.size(), 1); // 数据大小
        QCOMPARE(result.second[0].sn, "TEST-201");

        // 测试按时间范围筛选
        QDateTime yesterday = QDateTime::currentDateTime().addDays(-2);
        QDateTime tomorrow = QDateTime::currentDateTime().addDays(1);

        result = repo->queryByFilter("", yesterday, tomorrow);
        QCOMPARE(result.first, 2);

        result = repo->queryByFilter("", yesterday, yesterday.addDays(1));
        QCOMPARE(result.first, 1);
    }
};

QTEST_MAIN(TestInspectionRecordRepo)
#include "Test_InspectionRecordRepo.moc"