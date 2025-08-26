# 依赖库
- Qt 5.12.8
- perception-guizi： 头文件和动态库分别放入3rdparty/perception-guizi下
├─include
├───Interface.hpp
├───ObjectType.hpp
├─lib
└───libperception.so

- snpe-2.21
├─lib
├───libSNPE.so 
├───libSnpeHtpV68Skel.so 
├───libSnpeHtpV68Stub.so

- opencv-4.2.0
- mvs
- jsoncpp-1.9.5
- halcon20.11

# quecinfer 模型
- 下载模型放入 static/models 下 (应包含 model.json 和 sim.engine)

# 编译
mkdir build && cd
cmake ../