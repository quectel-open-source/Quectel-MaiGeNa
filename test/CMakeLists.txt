# 添加测试源文件
add_executable(Test_InspectionRecordRepo
    Test_InspectionRecordRepo.cpp  # 测试源文件
    ../view/inspectionrecordrepo.h      # 头文件（若需要）
    ../view/dbmanager.h
)

find_package(Qt5 REQUIRED Core Sql Test Gui)
# 链接 Qt 模块
target_link_libraries(Test_InspectionRecordRepo PRIVATE
    Qt5::Core
    Qt5::Sql
    Qt5::Gui  
    Qt5::Test  
)

# 将测试添加到 CTest
add_test(NAME InspectionRecordRepoTest COMMAND Test_InspectionRecordRepo)
set_tests_properties(InspectionRecordRepoTest PROPERTIES LABELS "unit")