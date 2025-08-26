#include "MVSCamera.h"
#include "CameraParams.h"
#include "MvCameraControl.h"
#include "MvErrorDefine.h"
#include <cstdio>
#include <qglobal.h>
#include <string>

#include <cstdio>
#include <string>

MV_CC_DEVICE_INFO_LIST MVSCamera::stDevList_ = {};
int g_exposure_high = 5000;
int g_exposure_low = 2500;

static void onCameraExceptionCallback(unsigned int nMsgType, void *pUser)
{
    qCritical() << "OnCameraExceptionCallback: msgtype=" << QString::number(nMsgType, 16).toUpper();
    MVSCamera *pThis = static_cast<MVSCamera *>(pUser);
    pThis->onError(nMsgType);
}

static void onCameraFrameCallback(unsigned char *data, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser)
{
    qDebug() << "onCameraFrameCallback: frameNum=" << QString::number(pFrameInfo->nFrameNum, 16).toUpper();
    MVSCamera *pThis = (MVSCamera *)pUser;
    pThis->onFrame(pFrameInfo, data);
}

void MVSCamera::onError(int msgType)
{
    if (errorCallback_)
        errorCallback_(msgType);
}

void MVSCamera::onFrame(MV_FRAME_OUT_INFO_EX *pFrameInfo, unsigned char *pData)
{
    static std::atomic_int pushNum = 0;
    pushNum++;

    qDebug() << "pushNum:" << pushNum << "frameNum:" << pFrameInfo->nFrameNum << " width:" << pFrameInfo->nWidth
             << "  height:" << pFrameInfo->nHeight << "  pixelType:" << pFrameInfo->enPixelType
             << "  frameNum:" << pFrameInfo->nFrameNum << "  hostTimeStamp:" << pFrameInfo->nHostTimeStamp;

    if (frameCallback_)
    {
        FrameData frame = {
            .camid = currentCamIndex_,
            .pushNum = pushNum,
            .frameNum = static_cast<int>(pFrameInfo->nFrameNum),
            .width = pFrameInfo->nWidth,
            .height = pFrameInfo->nHeight,
            .pixelType = pFrameInfo->enPixelType,
            .data = pData,
            .timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count(),
        };

        assert(frameCallback_);
        frameCallback_(std::move(frame));
    }
}
void MVSCamera::ReconnectDevice(unsigned int nMsgType)
{
    sendmessage("设备[%d]掉线，尝试重连", currentCamIndex_);
    if (m_bStartGrabbing[currentCamIndex_] == true)
    {
        stopGrabbing(currentCamIndex_);
        closeCamera(currentCamIndex_);
        int nRet;
        while (true)
        {
            nRet = connectCamera(currentCamIndex_);
            nRet = startGrabbing(currentCamIndex_);
            if (MV_OK == nRet)
            {
                sendmessage("设备[%d]已重连", currentCamIndex_);
                break;
            }
        }
    }
}

bool MVSCamera::isConnected(int camIndex)
{
    if (auto camHandle = getCamHandle(camIndex))
    {
        return MV_CC_IsDeviceConnected(camHandle);
    }
    return false;
}
static void *WorkThread(void *pUser)
{
    MVSCamera *camera = (MVSCamera *)pUser;
    camera->ReadBuffer();
    return (void *)0;
}

MVSCamera::MVSCamera()
{
    for (int i = 0; i < CAMERA_NUM_MAX; i++)
    {
        m_pDevHandle[i] = nullptr;
        m_pImageBuffer[i] = nullptr;
        m_nImageBufferSize[i] = 0;
        m_bStartGrabbing[i] = false;
        memset(&m_stImageInfo[i], 0, sizeof(MV_FRAME_OUT_INFO_EX));
    }
    m_bThreadState = false;
    currentCamIndex_ = -1;
}
MVSCamera::~MVSCamera()
{
    if (NULL != m_pDevHandle[currentCamIndex_])
    {
        stopGrabbing(currentCamIndex_);
        closeCamera(currentCamIndex_);
    }
}
//////////// 回调
void MVSCamera::sendmsg(CallBack call_back, void *handle)
{
    callHandle1 = handle;
    _my_call_back = call_back;
}
void MVSCamera::sendImage(CallBack2 call_back, void *Handle)
{
    callHandle2 = Handle;
    _sendImage_call_back = call_back;
}

//////////// 枚举
bool MVSCamera::enumDevice(std::vector<std::vector<std::string>> &cameraInfoList)
{
    memset(&stDevList_, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDevList_);
    if (MV_OK != nRet)
    {
        // sendmessage("枚举设备失败，错误码:0x%x", nRet);
        return false;
    }

    cameraInfoList.clear();

    for (unsigned int i = 0; i < stDevList_.nDeviceNum; i++)
    {
        std::vector<std::string> camInfo;
        auto pDeviceInfo = stDevList_.pDeviceInfo[i];
        if (NULL == pDeviceInfo)
        {
            // sendmessage("设备信息为空");
            continue;
        }

        if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
        {
            string camerakey = (char *)pDeviceInfo->SpecialInfo.stGigEInfo.chManufacturerName;
            int nIp1 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
            int nIp2 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
            int nIp3 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
            int nIp4 = (pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);
            string ipadress = formatStr("%d.%d.%d.%d", nIp1, nIp2, nIp3, nIp4);
            string modelName = (char *)pDeviceInfo->SpecialInfo.stGigEInfo.chModelName;
            string serialNumber = (char *)pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber;
            string deviceVersion = (char *)pDeviceInfo->SpecialInfo.stGigEInfo.chDeviceVersion;

            camInfo.push_back(camerakey + ":" + serialNumber);
            camInfo.push_back(camerakey);
            camInfo.push_back(modelName);
            camInfo.push_back(serialNumber);
            camInfo.push_back(deviceVersion);
            camInfo.push_back(ipadress);
            camInfo.push_back(std::to_string(i));
            cameraInfoList.push_back(camInfo);
        }
        else if (pDeviceInfo->nTLayerType == MV_USB_DEVICE)
        {
            string camerakey = (char *)pDeviceInfo->SpecialInfo.stUsb3VInfo.chManufacturerName;
            string serialNumber = (char *)pDeviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber;
            string modelName = (char *)pDeviceInfo->SpecialInfo.stUsb3VInfo.chModelName;
            string deviceVersion = (char *)pDeviceInfo->SpecialInfo.stUsb3VInfo.chDeviceVersion;

            camInfo.push_back(camerakey + ":" + serialNumber);
            camInfo.push_back(modelName);
            camInfo.push_back(serialNumber);
            camInfo.push_back(deviceVersion);
            camInfo.push_back("USB3");
            camInfo.push_back(std::to_string(i));
            cameraInfoList.push_back(camInfo);
        }

        // sendmessage("相机[%d]:  %s", i, camInfo[0].c_str());
    }

    // sendmessage("找到%d个相机", cameraInfoList.size());
    return true;
}

int MVSCamera::connectCamera(int camIndex)
{
    if (camIndex >= stDevList_.nDeviceNum)
    {
        qCritical("Camera index out of range: %d", camIndex);
        sendmessage("相机索引超出范围:%d", camIndex);
        return -1;
    }

    currentCamIndex_ = camIndex;
    auto deviceInfo = stDevList_.pDeviceInfo[camIndex];

    int nRet = MV_CC_CreateHandle(&m_pDevHandle[currentCamIndex_], deviceInfo);
    if (0 != nRet)
    {
        sendmessage("创建设备句柄失败，错误码:0x%x", nRet);
        return -1;
    }

    nRet = MV_CC_OpenDevice(m_pDevHandle[currentCamIndex_]);
    if (nRet != 0)
    {
        sendmessage("打开设备失败，错误码:0x%x", nRet);
        MV_CC_DestroyHandle(m_pDevHandle[currentCamIndex_]);
        m_pDevHandle[currentCamIndex_] = nullptr;
        return -1;
    }

    sendmessage("打开设备成功");

    nRet = MV_CC_RegisterExceptionCallBack(m_pDevHandle[currentCamIndex_], onCameraExceptionCallback, this);
    if (nRet != 0)
    {
        sendmessage("注册异常回调失败，错误码:0x%x", nRet);
        return -1;
    }

    return 0;
}

int MVSCamera::closeCamera(int index)
{
    if (!getCamHandle(index))
        return -1;

    if (!isConnected(index))
        return -1;

    if (m_bStartGrabbing[index])
    {
        int ret = stopGrabbing(index);
        // if (ret != 0)
        //     return ret;
    }

    int ret = MV_CC_CloseDevice(m_pDevHandle[index]);
    ret = MV_CC_DestroyHandle(m_pDevHandle[index]);
    m_pDevHandle[index] = NULL;

    if (ret != 0)
    {
        sendmessage("相机[%d]: 关闭设备失败, 错误码:0x%x", index, ret);
        return -1;
    }
    else
    {
        sendmessage("相机[%d]: 关闭设备成功", index);
        return 0;
    }
}

/////////////// 取流
int MVSCamera::startGrabbing(int camIndex)
{
    // if (auto ret = setBestPacketSize(); ret != MV_OK)
    // {
    //     sendmessage("相机[%d]: 设置最佳包大小失败, 错误码:0x%x", index, ret);
    //     // return ret;
    // }
    // if (auto ret = getWidth(currentCamIndex_, m_width); ret != MV_OK)
    //     return ret;
    // if (auto ret = getHeight(currentCamIndex_, m_height); ret != MV_OK)
    //     return ret;
    // if(auto ret = getPayloadSize(currentCamIndex_, m_nPayloadSize); ret != MV_OK)
    //     return ret;
    // initData(m_width, m_height);
    // sendmessage("初始化成功, 宽高: %dx%d, PayloadSize: %d", m_width, m_height, m_nPayloadSize);

    if (!getCamHandle(camIndex))
        return -1;

    int nRet = MV_OK;
    nRet = MV_CC_RegisterImageCallBackEx(m_pDevHandle[camIndex], onCameraFrameCallback, this);
    if (MV_OK != nRet)
    {
        sendmessage("注册回调函数失败，错误码:0x%x", nRet);
        return nRet;
    }

    nRet = MV_CC_StartGrabbing(m_pDevHandle[camIndex]);
    if (MV_OK != nRet)
    {
        sendmessage("开始采集图像失败，错误码:0x%x", nRet);
        return nRet;
    }

    m_bStartGrabbing[camIndex] = true;

    sendmessage("开始采集图像成功");
    return MV_OK;
}

int MVSCamera::stopGrabbing(int index)
{
    if (!getCamHandle(index))
        return -1;

    int nRet = MV_CC_StopGrabbing(m_pDevHandle[index]);
    if (nRet != 0)
    {
        sendmessage("相机[%d]: 停止采集失败, 错误码:0x%x", index, nRet);
        return nRet;
    }

    nRet = MV_CC_RegisterImageCallBackEx(m_pDevHandle[index], nullptr, this);

    m_bStartGrabbing[index] = false;
    sendmessage("相机[%d]: 停止采集成功", index);
    return MV_OK;
}

int MVSCamera::setBestPacketSize()
{
    auto pDev = stDevList_.pDeviceInfo[currentCamIndex_];
    if (!pDev)
    {
        sendmessage("设备信息为空, 相机索引为: %d", currentCamIndex_);
        return -1;
    }
    // ch:探测网络最佳包大小(只对GigE相机有效) | en:Detection network optimal package size(It only works for the GigE
    // camera)
    unsigned int nPacketSize = 0;
    if (pDev->nTLayerType == MV_GIGE_DEVICE)
    {
        nPacketSize = MV_CC_GetOptimalPacketSize(m_pDevHandle[currentCamIndex_]);
        if (nPacketSize > 0)
        {
            int nRet = MV_CC_SetIntValueEx(m_pDevHandle[currentCamIndex_], "GevSCPSPacketSize", nPacketSize);
            if (nRet != MV_OK)
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    return MV_OK;
}

///////////////  取流-2
void MVSCamera::initData(int width, int height)
{
    for (int i = 0; i < ArrayLength; i++)
    {
        m_dataList[i].width = width;
        m_dataList[i].height = height;
        if (m_dataList[i].data != nullptr)
        {
            free(m_dataList[i].data);
        }
        m_dataList[i].data = (unsigned char *)malloc(sizeof(unsigned char) * m_nPayloadSize);
    }
}
int MVSCamera::StartCamera_2(int run_grab, int trigger_type)
{
    int nRet = setBestPacketSize();
    nRet = getWidth(currentCamIndex_, m_width);
    nRet = getHeight(currentCamIndex_, m_height);
    // Get payload size
    MVCC_INTVALUE stParam;
    memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    nRet = MV_CC_GetIntValue(m_pDevHandle[currentCamIndex_], "PayloadSize", &stParam);
    if (MV_OK != nRet)
    {
        sendmessage("获取PayloadSize失败，错误码:0x%x", nRet);
        return -1;
    }
    m_nPayloadSize = stParam.nCurValue;
    sendmessage("初始化成功, 宽高: %dx%d, PayloadSize: %d", m_width, m_height, m_nPayloadSize);
    initData(m_width, m_height);

    if (run_grab == 0)
    {
        if (SetTriggerMode(currentCamIndex_, MV_TRIGGER_MODE_ON))
            sendmessage("触发模式: ON");
        if (SetTriggerSource(currentCamIndex_, MV_TRIGGER_SOURCE_LINE0))
            sendmessage("触发源: LINE%d", 0);
        if (SetTriggerActivation(1)) // 下降沿
            sendmessage("触发方式: 下降沿");
    }
    else
    {
        if (trigger_type >= 0 && trigger_type <= 3)
        {
            if (SetTriggerMode(currentCamIndex_, MV_TRIGGER_MODE_ON))
                sendmessage("触发模式: ON");
            if (SetTriggerSource(currentCamIndex_, trigger_type))
                sendmessage("触发源: LINE%d", trigger_type);
            if (SetTriggerActivation(0)) // 1: 下降沿
                sendmessage("触发方式: 下降沿");
        }
        else if (trigger_type == 7)
        {
            if (SetTriggerMode(currentCamIndex_, MV_TRIGGER_MODE_ON))
                sendmessage("触发模式: ON");
            if (SetTriggerSource(currentCamIndex_, MV_TRIGGER_SOURCE_SOFTWARE))
                sendmessage("触发源: 软件触发");
            m_count_software = 0;
        }
        else
        {
            sendmessage("触发模式错误: %d", trigger_type);
            return -1;
        }
    }

    nRet = MV_CC_RegisterImageCallBackEx(m_pDevHandle[currentCamIndex_], onCameraFrameCallback, this);
    if (MV_OK != nRet)
    {
        sendmessage("注册回调函数失败，错误码:0x%x", nRet);
        return -1;
    }

    nRet = MV_CC_StartGrabbing(m_pDevHandle[currentCamIndex_]);
    if (nRet != 0)
    {
        sendmessage("开始采集图像失败，错误码:0x%x", nRet);
        return -1;
    }

    m_bStartGrabbing[currentCamIndex_] = true;
    return MV_OK;
}
int MVSCamera::ReadBuffer_2()
{
    m_readBufferIndex = 0;
    MV_FRAME_OUT_INFO_EX stImageInfo = {0};
    memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));

    while (m_bStartGrabbing[currentCamIndex_] == true)
    {
        int nRet = MV_CC_GetOneFrameTimeout(m_pDevHandle[currentCamIndex_], m_dataList[m_readBufferIndex].data,
                                            m_nPayloadSize, &stImageInfo, 1000);
        if (nRet == MV_OK)
        {
            std::unique_lock<std::mutex> locker(m_mutexCallback);

            m_dataList[m_readBufferIndex].camid = currentCamIndex_;
            m_dataList[m_readBufferIndex].frameNum = stImageInfo.nFrameNum;
            m_dataList[m_readBufferIndex].pixelType = stImageInfo.enPixelType;
            m_readBufferIndex = (m_readBufferIndex + 1) % ArrayLength;
            m_isDataReady = true;

            m_conditionCallback.notify_one(); // 通知相机的处理线程
        }
    }
    for (int i = 0; i < ArrayLength; i++)
    {
        free(m_dataList[i].data);
    }
    return MV_OK;
}

std::string MVSCamera::getCurrentTimeString()
{
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();

    // 转换为 time_t 类型，以便格式化日期和时间
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto local_time = *std::localtime(&time_t_now);

    // 获取毫秒部分
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // 使用字符串流格式化时间
    std::ostringstream oss;
    oss << std::put_time(&local_time, "%H-%M-%S");                 // 格式化时分秒
    oss << '.' << std::setw(4) << std::setfill('0') << ms.count(); // 格式化毫秒

    return oss.str();
}

// 条件变量，返回当前数据
bool MVSCamera::waitForData(CameraData &data, int timeoutMs)
{
    std::unique_lock<std::mutex> lock(m_mutexCallback);
    if (timeoutMs > 0)
    {
        auto status = m_conditionCallback.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] {
            return m_isDataReady || m_bStartGrabbing[currentCamIndex_] == false;
        });

        if (!status || m_bStartGrabbing[currentCamIndex_] == false)
            return false;
    }
    else
    {
        m_conditionCallback.wait(lock, [this] { return m_isDataReady || m_bStartGrabbing[currentCamIndex_] == false; });
        if (m_bStartGrabbing[currentCamIndex_] == false)
            return false;
    }
    m_isDataReady = false;

    data = m_dataList[(m_readBufferIndex - 1 + ArrayLength) % ArrayLength];

    return true;
}

/*
void MVSCamera::mvsImageCallBack(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
    MVSCamera *pThis = (MVSCamera*)pUser;
    if (LogIsShow)
        LOG_INFO("trigger one: camid=" + QString::number(pThis->indexcamera) + ", campicid=" +
QString::number(pThis->m_campicid));

    std::unique_lock<std::mutex> lock(pThis->g_mutexCallback);
    CameraData data;
    data.pushNum = pThis->m_pushNum;
    data.camid = pThis->indexcamera;
    data.campicid = pThis->m_campicid;
    data.frameNum = pFrameInfo->nFrameNum;
    data.time_hhmmss_zzz = getCurrentTimeString();

    data.width = pFrameInfo->nWidth;
    data.height = pFrameInfo->nHeight;
    data.data = (unsigned char*)malloc(data.width * data.height * sizeof(unsigned char));
    memcpy(data.data, pData, data.width * data.height * sizeof(unsigned char));

//    memcpy(pThis->m_datas[pThis->m_dataIndex], pData, data.width * data.height * sizeof(unsigned char));
//    data.mat_img = cv::Mat(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, pThis->m_datas[pThis->m_dataIndex]);
//    pThis->m_dataIndex = (pThis->m_dataIndex + 1) % CameraCounts;

//    MV_CC_PIXEL_CONVERT_PARAM CvtParam;
//    // 从上到下依次是：图像宽，图像高，输入数据缓存，输入数据大小，源像素格式，
//    // 目标像素格式，输出数据缓存，提供的输出缓冲区大小
//    CvtParam.nWidth           = pFrameInfo->nWidth;
//    CvtParam.nHeight          = pFrameInfo->nHeight;
//    CvtParam.pSrcData         = pData;
//    CvtParam.nSrcDataLen      = pFrameInfo->nFrameLen;
//    CvtParam.enSrcPixelType   = pFrameInfo->enPixelType;
//    CvtParam.enDstPixelType   = PixelType_Gvsp_Mono8;
//    CvtParam.pDstBuffer       = pConvertData;
//    MV_CC_ConvertPixelType(pThis->m_pDevHandle[data.camid], &CvtParam);
//    data.mat_img = cv::Mat(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, pConvertData);

    pThis->m_campicid++;
    if (pThis->m_campicid >= pThis->m_capacity){
        pThis->m_campicid = 0;
        pThis->m_pushNum++;
    }
    pThis->g_cameraDatas.push(data);
//    free(pConvertData);
    pThis->g_conditionCallback.notify_one(); // 通知相机的处理线程
}
void MVSCamera::mvsImageCallBack_1(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
    MVSCamera *pThis = (MVSCamera*)pUser;
    if (LogIsShow)
        LOG_INFO("trigger one: camid=" + QString::number(pThis->indexcamera) + ", campicid=" +
QString::number(pThis->m_campicid));

    std::unique_lock<std::mutex> lock(pThis->g_mutexCallback);
    if (pThis->m_campicid == 0){
        pThis->SetExposureTime(pThis->indexcamera, g_exposure_low);
    }
    else if (pThis->m_campicid == pThis->m_capacity - 1){
        pThis->SetExposureTime(pThis->indexcamera, g_exposure_high);
    }
    CameraData data;
    data.pushNum = pThis->m_pushNum;
    data.camid = pThis->indexcamera;
    data.campicid = pThis->m_campicid;
    data.frameNum = pFrameInfo->nFrameNum;
    data.time_hhmmss_zzz = getCurrentTimeString();
    data.width = pFrameInfo->nWidth;
    data.height = pFrameInfo->nHeight;
    data.data = (unsigned char*)malloc(data.width * data.height * sizeof(unsigned char));
    memcpy(data.data, pData, data.width * data.height * sizeof(unsigned char));

//    memcpy(pThis->m_datas[pThis->m_dataIndex], pData, data.width * data.height * sizeof(unsigned char));
//    data.mat_img = cv::Mat(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, pThis->m_datas[pThis->m_dataIndex]);
//    pThis->m_dataIndex = (pThis->m_dataIndex + 1) % CameraCounts;

    pThis->m_campicid++;
    if (pThis->m_campicid >= pThis->m_capacity){
        pThis->m_campicid = 0;
        pThis->m_pushNum++;
    }
    pThis->g_cameraDatas.push(data);
    pThis->g_conditionCallback.notify_one(); // 通知相机的处理线程
}
void MVSCamera::mvsImageCallBack_2(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
    MVSCamera *pThis = (MVSCamera*)pUser;
    if (LogIsShow)
        LOG_INFO("trigger one: camid=" + QString::number(pThis->indexcamera) + ", campicid=" +
QString::number(pThis->m_campicid));

    std::unique_lock<std::mutex> lock(pThis->g_mutexCallback);
    if (pThis->m_campicid == 0){
        pThis->SetExposureTime(pThis->indexcamera, g_exposure_low);
    }
    else if (pThis->m_campicid == pThis->m_capacity - 1){
        pThis->SetExposureTime(pThis->indexcamera, g_exposure_high);
    }
    CameraData data;
    data.pushNum = pThis->m_pushNum;
    data.camid = pThis->indexcamera;
    data.campicid = pThis->m_campicid;
    data.frameNum = pFrameInfo->nFrameNum;
    data.time_hhmmss_zzz = getCurrentTimeString();

    data.width = pFrameInfo->nWidth;
    data.height = pFrameInfo->nHeight;
    data.data = (unsigned char*)malloc(data.width * data.height * sizeof(unsigned char));
    memcpy(data.data, pData, data.width * data.height * sizeof(unsigned char));

//    memcpy(pThis->m_datas[pThis->m_dataIndex], pData, data.width * data.height * sizeof(unsigned char));
//    data.mat_img = cv::Mat(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, pThis->m_datas[pThis->m_dataIndex]);
//    pThis->m_dataIndex = (pThis->m_dataIndex + 1) % CameraCounts;

    pThis->m_campicid++;
    if (pThis->m_campicid >= pThis->m_capacity){
        pThis->m_campicid = 0;
        pThis->m_pushNum++;
    }
    pThis->g_cameraDatas.push(data);
    pThis->g_conditionCallback.notify_one(); // 通知相机的处理线程
}
void MVSCamera::mvsImageCallBack_3(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
    MVSCamera *pThis = (MVSCamera*)pUser;
    if (LogIsShow)
        LOG_INFO("trigger one: camid=" + QString::number(pThis->indexcamera) + ", campicid=" +
QString::number(pThis->m_campicid));

    std::unique_lock<std::mutex> lock(pThis->g_mutexCallback);
    if (pThis->m_campicid == 0){
        pThis->SetExposureTime(pThis->indexcamera, g_exposure_low);
    }
    else if (pThis->m_campicid == pThis->m_capacity - 1){
        pThis->SetExposureTime(pThis->indexcamera, g_exposure_high);
    }
    CameraData data;
    data.pushNum = pThis->m_pushNum;
    data.camid = pThis->indexcamera;
    data.campicid = pThis->m_campicid;
    data.frameNum = pFrameInfo->nFrameNum;
    data.time_hhmmss_zzz = getCurrentTimeString();

    data.width = pFrameInfo->nWidth;
    data.height = pFrameInfo->nHeight;
    data.data = (unsigned char*)malloc(data.width * data.height * sizeof(unsigned char));
    memcpy(data.data, pData, data.width * data.height * sizeof(unsigned char));

//    memcpy(pThis->m_datas[pThis->m_dataIndex], pData, data.width * data.height * sizeof(unsigned char));
//    data.mat_img = cv::Mat(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, pThis->m_datas[pThis->m_dataIndex]);
//    pThis->m_dataIndex = (pThis->m_dataIndex + 1) % CameraCounts;

    pThis->m_campicid++;
    if (pThis->m_campicid >= pThis->m_capacity){
        pThis->m_campicid = 0;
        pThis->m_pushNum++;
    }
    pThis->g_cameraDatas.push(data);
    pThis->g_conditionCallback.notify_one(); // 通知相机的处理线程
}
void MVSCamera::mvsImageCallBack_4(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
    {
        std::unique_lock<std::mutex> lock1(ModbusWorker::g_mutex_RW);
        ModbusWorker::g_type_index = 1;
        ModbusWorker::g_condition_RW.notify_one(); // 通知相机的处理线程
    }
    MVSCamera *pThis = (MVSCamera*)pUser;
    if (LogIsShow)
        LOG_INFO("trigger one: camid=" + QString::number(pThis->indexcamera) + ", campicid=" +
QString::number(pThis->m_campicid));

    std::unique_lock<std::mutex> lock(pThis->g_mutexCallback);
    if (pThis->m_campicid == 0){
        pThis->SetExposureTime(pThis->indexcamera, g_exposure_low);
    }
    else if (pThis->m_campicid == pThis->m_capacity - 1){
        pThis->SetExposureTime(pThis->indexcamera, g_exposure_high);
    }
    CameraData data;
    data.pushNum = pThis->m_pushNum;
    data.camid = pThis->indexcamera;
    data.campicid = pThis->m_campicid;
    data.frameNum = pFrameInfo->nFrameNum;
    data.time_hhmmss_zzz = getCurrentTimeString();

    data.width = pFrameInfo->nWidth;
    data.height = pFrameInfo->nHeight;
    data.data = (unsigned char*)malloc(data.width * data.height * sizeof(unsigned char));
    memcpy(data.data, pData, data.width * data.height * sizeof(unsigned char));

//    memcpy(pThis->m_datas[pThis->m_dataIndex], pData, data.width * data.height * sizeof(unsigned char));
//    data.mat_img = cv::Mat(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, pThis->m_datas[pThis->m_dataIndex]);
//    pThis->m_dataIndex = (pThis->m_dataIndex + 1) % CameraCounts;

    pThis->m_campicid++;
    if (pThis->m_campicid >= pThis->m_capacity){
        pThis->m_campicid = 0;
        pThis->m_pushNum++;
    }
    pThis->g_cameraDatas.push(data);
    pThis->g_conditionCallback.notify_one(); // 通知相机的处理线程
}
void MVSCamera::mvsImageCallBack_5(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
    MVSCamera *pThis = (MVSCamera*)pUser;
    if (LogIsShow)
        LOG_INFO("trigger one: camid=" + QString::number(pThis->indexcamera) + ", campicid=" +
QString::number(pThis->m_campicid));

    std::unique_lock<std::mutex> lock(pThis->g_mutexCallback);
    if (pThis->m_campicid == 0){
        pThis->SetExposureTime(pThis->indexcamera, g_exposure_low);
    }
    else if (pThis->m_campicid == pThis->m_capacity - 1){
        pThis->SetExposureTime(pThis->indexcamera, g_exposure_high);
    }
    CameraData data;
    data.pushNum = pThis->m_pushNum;
    data.camid = pThis->indexcamera;
    data.campicid = pThis->m_campicid;
    data.frameNum = pFrameInfo->nFrameNum;
    data.time_hhmmss_zzz = getCurrentTimeString();

    data.width = pFrameInfo->nWidth;
    data.height = pFrameInfo->nHeight;
    data.data = (unsigned char*)malloc(data.width * data.height * sizeof(unsigned char));
    memcpy(data.data, pData, data.width * data.height * sizeof(unsigned char));

//    memcpy(pThis->m_datas[pThis->m_dataIndex], pData, data.width * data.height * sizeof(unsigned char));
//    data.mat_img = cv::Mat(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC1, pThis->m_datas[pThis->m_dataIndex]);
//    pThis->m_dataIndex = (pThis->m_dataIndex + 1) % CameraCounts;

    pThis->m_campicid++;
    if (pThis->m_campicid >= pThis->m_capacity){
        pThis->m_campicid = 0;
        pThis->m_pushNum++;
    }
    pThis->g_cameraDatas.push(data);
    pThis->g_conditionCallback.notify_one(); // 通知相机的处理线程
}
*/
/////////// 取流
int MVSCamera::ReadBuffer()
{
    m_readBufferIndex = 0;
    MV_FRAME_OUT_INFO_EX stImageInfo = {0};
    memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
    for (int i = 0; i < CAMERA_NUM_MAX; i++)
    {
        pData[i] = (unsigned char *)malloc(sizeof(unsigned char) * (m_nPayloadSize));
    }

    while (m_bStartGrabbing[currentCamIndex_] == true)
    {
        int nRet = MV_CC_GetOneFrameTimeout(m_pDevHandle[currentCamIndex_], pData[m_readBufferIndex], m_nPayloadSize,
                                            &stImageInfo, 1000);
        if (nRet == MV_OK)
        {

            _sendImage_call_back(stImageInfo, pData[m_readBufferIndex], callHandle2, currentCamIndex_);
            m_readBufferIndex = (m_readBufferIndex + 1) % CAMERA_NUM_MAX;
            // free(pData);
        }
    }
    for (int i = 0; i < CAMERA_NUM_MAX; i++)
    {
        free(pData[i]);
    }
    return MV_OK;
}

// get gamma
double MVSCamera::Getgamma(double &dgamma, double &dminGamma, double &dmaxGamma)
{
    if (!getCamHandle(currentCamIndex_))
        return -1;

    MVCC_FLOATVALUE stFloatValue = {0};
    int nRet = MV_CC_GetFloatValue(m_pDevHandle[currentCamIndex_], "Gamma", &stFloatValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取gamma失败, 错误码:0x%x", nRet);
        return -1;
    }

    dgamma = stFloatValue.fCurValue;
    dminGamma = stFloatValue.fMin;
    dmaxGamma = stFloatValue.fMax;
    return 0;
}

// set gamma
bool MVSCamera::Setgamma(double dgamma)
{
    if (!getCamHandle(currentCamIndex_))
        return false;

    int nRet = MV_CC_SetFloatValue(m_pDevHandle[currentCamIndex_], "Gamma", (float)dgamma);
    if (MV_OK != nRet)
    {
        sendmessage("设置gamma失败, 错误码:0x%x", nRet);
        return false;
    }

    return true;
}
// ???????
// get Contrast
double MVSCamera::GetContrast(int64_t &dContrast, int64_t &dminContrast, int64_t &dmaxContrast)
{
    if (!getCamHandle(currentCamIndex_))
        return -1;
    MVCC_INTVALUE stFloatValue = {0};
    int Ret = MV_CC_SetBoolValue(m_pDevHandle[currentCamIndex_], "ContrastRatioEnable", true);
    int nRet = MV_CC_GetIntValue(m_pDevHandle[currentCamIndex_], "ContrastRatio", &stFloatValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取对比度失败, 错误码:0x%x", nRet);
        return -1;
    }

    dContrast = stFloatValue.nCurValue;
    dminContrast = stFloatValue.nMin;
    dmaxContrast = stFloatValue.nMax;
    return 0;
}

// ???????
// set Contrast
bool MVSCamera::SetContrast(int64_t dContrast)
{
    if (!getCamHandle(currentCamIndex_))
        return false;
    int Ret = MV_CC_SetBoolValue(m_pDevHandle[currentCamIndex_], "ContrastRatioEnable", true);
    int nRet = MV_CC_SetIntValue(m_pDevHandle[currentCamIndex_], "ContrastRatio", (int)dContrast);
    if (MV_OK != nRet)
    {
        sendmessage("设置对比度失败, 错误码:0x%x", nRet);
        return false;
    }

    return true;
}
// ??????????
// get  Heartbeat
double MVSCamera::GetHeartbeatTimeout(int64_t &dHeartbeatTimeout, int64_t &dminHeartbeatTimeout,
                                      int64_t &dmaxHeartbeatTimeout)
{
    if (!getCamHandle(currentCamIndex_))
        return -1;

    MVCC_INTVALUE stFloatValue = {0};
    int nRet = MV_CC_GetIntValue(m_pDevHandle[currentCamIndex_], "GevHeartbeatTimeout", &stFloatValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取心跳超时失败, 错误码:0x%x", nRet);
        return -1;
    }

    dHeartbeatTimeout = stFloatValue.nCurValue;
    dminHeartbeatTimeout = stFloatValue.nMin;
    dmaxHeartbeatTimeout = stFloatValue.nMax;
    return 0;
}

// set  Heartbeat
bool MVSCamera::SetHeartbeatTimeout(int64_t dHeartbeatTimeout)
{
    if (!getCamHandle(currentCamIndex_))
        return false;

    if (dHeartbeatTimeout < 500)
        dHeartbeatTimeout = 500;
    int nRet = MV_CC_SetIntValue(m_pDevHandle[currentCamIndex_], "GevHeartbeatTimeout", (int)dHeartbeatTimeout);
    if (MV_OK != nRet)
    {
        sendmessage("设置心跳超时失败, 错误码:0x%x", nRet);
        return false;
    }

    return true;
}

// get Contrast
double MVSCamera::GetDebouncer(double &dDebouncer, double &dminDebouncer, double &dmaxDebouncer)
{
    if (!getCamHandle(currentCamIndex_))
        return -1;

    MVCC_FLOATVALUE stFloatValue = {0};
    int nRet = MV_CC_GetFloatValue(m_pDevHandle[currentCamIndex_], "LineDebouncerTime", &stFloatValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取去抖失败, 错误码:0x%x", nRet);
        return -1;
    }
    dDebouncer = stFloatValue.fCurValue;
    dminDebouncer = stFloatValue.fMin;
    dmaxDebouncer = stFloatValue.fMax;
    return 0;
}

// set Contrast
bool MVSCamera::SetDebouncer(double dDebouncer)
{
    if (!getCamHandle(currentCamIndex_))
        return false;

    int nRet = MV_CC_SetFloatValue(m_pDevHandle[currentCamIndex_], "LineDebouncerTime", (float)dDebouncer);
    if (MV_OK != nRet)
    {
        sendmessage("设置去抖失败, 错误码:0x%x", nRet);
        return false;
    }

    return true;
}

int MVSCamera::getPayloadSize(int camIndex, int &payloadSize)
{
    if (!getCamHandle(camIndex))
        return -1;

    MVCC_INTVALUE stParam;
    memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    int nRet = MV_CC_GetIntValue(m_pDevHandle[camIndex], "PayloadSize", &stParam);
    if (MV_OK != nRet)
    {
        sendmessage("获取PayloadSize失败，错误码:0x%x", nRet);
        return -1;
    }
    payloadSize = stParam.nCurValue;
    return 0;
}

//// 软触发
bool MVSCamera::softTrigger(int index)
{
    if (!getCamHandle(index))
        return false;

    int ret = MV_CC_SetCommandValue(m_pDevHandle[index], "TriggerSoftware");
    if (ret != 0)
    {
        sendmessage("软触发失败, 错误码:0x%x", ret);
        return false;
    }
    m_count_software++;
    sendmessage("相机[%d] 软触发成功", index);
    return true;
}
// 触发极性  1:下降沿
bool MVSCamera::SetTriggerActivation(int mode)
{
    if (!getCamHandle(currentCamIndex_))
        return false;

    int nRet = MV_CC_SetEnumValue(m_pDevHandle[currentCamIndex_], "TriggerActivation", mode);
    if (MV_OK != nRet)
    {
        sendmessage("设置触发极性失败, 错误码:0x%x", nRet);
        return false;
    }

    return true;
}
int MVSCamera::GetTriggerActivation()
{
    if (!getCamHandle(currentCamIndex_))
        return -1;

    MVCC_ENUMVALUE stEnumValue = {0};
    int nRet = MV_CC_GetEnumValue(m_pDevHandle[currentCamIndex_], "TriggerActivation", &stEnumValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取触发极性失败, 错误码:0x%x", nRet);
        return -1;
    }

    int mode = stEnumValue.nCurValue;

    return mode;
}
// 触发模式   0：关闭  1：打开
bool MVSCamera::SetTriggerMode(int index, int mode)
{
    if (!getCamHandle(index))
        return false;

    int mode_one = (mode == 0 ? MV_TRIGGER_MODE_OFF : MV_TRIGGER_MODE_ON);
    int nRet = MV_CC_SetEnumValue(m_pDevHandle[index], "TriggerMode", mode_one);
    if (MV_OK != nRet)
    {
        sendmessage("设置触发模式失败, 错误码:0x%x", nRet);
        return false;
    }

    sendmessage("设置触发模式成功, 模式: %d");
    return true;
}
int MVSCamera::GetTriggerMode(int index)
{
    if (!getCamHandle(index))
        return -1;

    MVCC_ENUMVALUE stEnumValue = {0};
    int nRet = MV_CC_GetEnumValue(m_pDevHandle[index], "TriggerMode", &stEnumValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取触发模式失败, 错误码:0x%x", nRet);
        return -1;
    }

    int mode = stEnumValue.nCurValue;

    return mode;
}
bool MVSCamera::SetTriggerDelay(int index, double mode)
{
    if (!getCamHandle(index))
        return false;

    int nRet = MV_CC_SetFloatValue(m_pDevHandle[index], "TriggerDelay", (float)mode);
    if (MV_OK != nRet)
    {
        sendmessage("设置触发延时失败, 错误码:0x%x", nRet);
        return false;
    }

    return true;
}
int MVSCamera::GetTriggerDelay(double &dTriggerDelay, double &dminTriggerDelay, double &dmaxTriggerDelay)
{
    if (!getCamHandle(currentCamIndex_))
        return -1;

    MVCC_FLOATVALUE stFloatValue = {0};
    int nRet = MV_CC_GetFloatValue(m_pDevHandle[currentCamIndex_], "TriggerDelay", &stFloatValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取触发延时失败, 错误码:0x%x", nRet);
        return -1;
    }

    dTriggerDelay = stFloatValue.fCurValue;
    dminTriggerDelay = stFloatValue.fMin;
    dmaxTriggerDelay = stFloatValue.fMax;
    return 0;
}

///////////
int MVSCamera::setHeight(int index, unsigned int height)
{
    if (!getCamHandle(index))
        return -1;

    int tempValue = MV_CC_SetIntValue(m_pDevHandle[index], "Height", height);
    if (tempValue != 0)
    {
        sendmessage("设置高度失败，错误码:0x%x", tempValue);
        return -1;
    }
    else
    {
        return 0;
    }
}
int MVSCamera::getHeight(int index, int &height)
{
    if (!getCamHandle(index))
        return -1;

    MVCC_INTVALUE stParam;
    memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    // 获取高度
    int tempValue = MV_CC_GetIntValue(m_pDevHandle[index], "Height", &stParam);
    if (tempValue != 0)
    {
        sendmessage("获取高度失败，错误码:0x%x", tempValue);
        return -1;
    }
    height = stParam.nCurValue;

    return 0;
}

int MVSCamera::getWidth(int index, int &width)
{
    if (!getCamHandle(index))
        return -1;

    MVCC_INTVALUE stParam;
    memset(&stParam, 0, sizeof(MVCC_INTVALUE));
    // 获取高度
    int nRet = MV_CC_GetIntValue(m_pDevHandle[index], "Width", &stParam);
    if (nRet != 0)
    {
        sendmessage("获取宽度失败，错误码:0x%x", nRet);
        return nRet;
    }
    width = stParam.nCurValue;

    return 0;
}
int MVSCamera::setWidth(int index, unsigned int width)
{
    if (!getCamHandle(index))
        return -1;

    int nRet = MV_CC_SetIntValue(m_pDevHandle[index], "Width", width);
    if (nRet != 0)
    {
        sendmessage("设置宽度失败，错误码:0x%x", nRet);
        return nRet;
    }
    else
    {
        return 0;
    }
}

// 设置曝光时间
bool MVSCamera::SetExposureTime(int index, double exp)
{
    if (!getCamHandle(index))
        return false;

    int nRet = MV_CC_SetFloatValue(m_pDevHandle[index], "ExposureTime", (float)exp);
    if (MV_OK != nRet)
    {
        sendmessage("设置曝光时间失败，错误码:0x%x", nRet);
        return false;
    }

    return true;
}
int MVSCamera::GetExposureTime(int index, float *dExposureTime, float *dminExposureTime, float *dmaxExposureTime)
{
    if (!getCamHandle(index))
        return -1;

    MVCC_FLOATVALUE stFloatValue = {0};
    int nRet = MV_CC_GetFloatValue(m_pDevHandle[index], "ExposureTime", &stFloatValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取曝光时间失败, 错误码:0x%x", nRet);
        return -1;
    }

    if (dExposureTime)
        *dExposureTime = stFloatValue.fCurValue;
    if (dminExposureTime)
        *dminExposureTime = stFloatValue.fMin;
    if (dmaxExposureTime)
        *dmaxExposureTime = stFloatValue.fMax;

    return MV_OK;
}

// ch:????????
bool MVSCamera::SetGain(int index, double gain)
{
    if (!getCamHandle(index))
        return false;

    // int nRet = MV_CC_SetEnumValue(m_pDevHandle[index], "GainAuto", 0);
    int nRet = MV_CC_SetFloatValue(m_pDevHandle[index], "Gain", (float)gain);
    if (MV_OK != nRet)
    {
        sendmessage("设置增益失败, 错误码:0x%x", nRet);
        return -1;
    }
    return true;
}
float MVSCamera::GetGain(float &dGain, float &dminGain, float &dmaxGain)
{
    if (!getCamHandle(currentCamIndex_))
        return -1;

    MVCC_FLOATVALUE stFloatValue = {0};
    int nRet = MV_CC_GetFloatValue(m_pDevHandle[currentCamIndex_], "Gain", &stFloatValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取增益失败, 错误码:0x%x", nRet);
        return -1;
    }
    else
    {
        dGain = stFloatValue.fCurValue;
        dminGain = stFloatValue.fMin;
        dmaxGain = stFloatValue.fMax;
        return 0;
    }
}

//
int MVSCamera::setFrameRateEnable(int index, bool comm)
{
    if (!getCamHandle(index))
        return -1;

    int ret = MV_CC_SetBoolValue(m_pDevHandle[index], "AcquisitionFrameRateEnable", comm);
    if (ret != 0)
    {
        sendmessage("启用帧率失败, 错误码:0x%x", ret);
        return -1;
    }

    return 0;
}
// ch:???????
bool MVSCamera::SetFrameRate(int index, double frame)
{
    if (!getCamHandle(index))
        return false;

    if (0 != setFrameRateEnable(index, true))
        return false;

    auto ret = MV_CC_SetFloatValue(m_pDevHandle[index], "AcquisitionFrameRate", (float)frame);
    if (ret != MV_OK)
    {
        sendmessage("设置帧率败, 错误码:0x%x", ret);
        return false;
    }

    return true;
}
float MVSCamera::GetFrameRate(int index)
{
    if (!getCamHandle(index))
        return -1;

    MVCC_FLOATVALUE stFloatValue = {0};
    int nRet = MV_CC_GetFloatValue(m_pDevHandle[index], "ResultingFrameRate", &stFloatValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取帧率失败, 错误码:0x%x", nRet);
        return nRet;
    }
    double frame = stFloatValue.fCurValue;
    return frame;
}

// ch:????????
bool MVSCamera::SetTriggerSource(int index, int soft)
{
    // 0:Line0  1:Line1  7: Software
    if (!getCamHandle(index))
        return false;

    if (soft == 7)
    {
        int nRet = MV_CC_SetEnumValue(m_pDevHandle[index], "TriggerSource", MV_TRIGGER_SOURCE_SOFTWARE);
        if (MV_OK != nRet)
        {
            sendmessage("设置软件触发失败, 错误码:0x%x", nRet);
            return false;
        }
    }
    else if (soft == 0)
    {
        int nRet = MV_CC_SetEnumValue(m_pDevHandle[index], "TriggerSource", MV_TRIGGER_SOURCE_LINE0);
        if (MV_OK != nRet)
        {
            sendmessage("设置LINE0失败, 错误码:0x%x", nRet);
            return false;
        }
    }
    else if (soft == 1)
    {
        int nRet = MV_CC_SetEnumValue(m_pDevHandle[index], "TriggerSource", MV_TRIGGER_SOURCE_LINE1);
        if (MV_OK != nRet)
        {
            sendmessage("设置LINE1失败, 错误码:0x%x", nRet);
            return false;
        }
    }
    else if (soft == 2)
    {
        int nRet = MV_CC_SetEnumValue(m_pDevHandle[index], "TriggerSource", MV_TRIGGER_SOURCE_LINE2);
        if (MV_OK != nRet)
        {
            sendmessage("设置LINE2失败, 错误码:0x%x", nRet);
            return false;
        }
    }

    sendmessage("设置触发源成功, 触发源为: %d");
    return true;
}
int MVSCamera::GetTriggerSource(int index)
{
    // 0??Line0  1??Line1  7??Software
    if (!getCamHandle(index))
        return -1;

    MVCC_ENUMVALUE stEnumValue = {0};
    int nRet = MV_CC_GetEnumValue(m_pDevHandle[index], "TriggerSource", &stEnumValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取触发源失败, 错误码:0x%x", nRet);
        return -1;
    }
    int value = stEnumValue.nCurValue;
    if (MV_TRIGGER_SOURCE_SOFTWARE == stEnumValue.nCurValue)
    {
        return 7;
    }
    else if (MV_TRIGGER_SOURCE_LINE0 == stEnumValue.nCurValue)
    {
        return 0;
    }
    else if (MV_TRIGGER_SOURCE_LINE1 == stEnumValue.nCurValue)
    {
        return 1;
    }
    else if (MV_TRIGGER_SOURCE_LINE2 == stEnumValue.nCurValue)
    {
        return 2;
    }
    else if (MV_TRIGGER_SOURCE_LINE3 == stEnumValue.nCurValue)
    {
        return 3;
    }

    return -1;
}

// 设置OffsetX
int MVSCamera::setOffsetX(int index, unsigned int offsetX)
{
    if (!getCamHandle(index))
        return -1;

    int nRet = MV_CC_SetIntValue(m_pDevHandle[index], "OffsetX", offsetX);
    if (nRet != 0)
    {
        sendmessage("设置OffsetX失败, 错误码:0x%x", nRet);
        return -1;
    }

    return 0;
}

// 设置OffsetY
int MVSCamera::setOffsetY(int index, unsigned int offsetY)
{
    if (!getCamHandle(index))
        return -1;

    int nRet = MV_CC_SetIntValue(m_pDevHandle[index], "OffsetY", offsetY);
    if (nRet != 0)
    {
        sendmessage("设置OffsetY失败, 错误码:0x%x", nRet);
        return -1;
    }
    else
    {
        return 0;
    }
}
// Load configuration of the device
double MVSCamera::loadconfig(string path)
{
    if (!getCamHandle(currentCamIndex_))
        return -1;

    if (path == "")
    {
        sendmessage("相机配置为空");
        return -1;
    }
    if (m_bStartGrabbing[currentCamIndex_])
    {
        stopGrabbing(currentCamIndex_);

        int ret = MV_CC_FeatureLoad(m_pDevHandle[currentCamIndex_], path.c_str());
        if (MV_OK != ret)
        {
            sendmessage("加载配置失败!错误码[%d]", ret);
            return -1;
        }

        sendmessage("加载配置成功!");
        startGrabbing(currentCamIndex_);
    }
    else
    {
        int ret = MV_CC_FeatureLoad(m_pDevHandle[currentCamIndex_], path.c_str());
        if (MV_OK != ret)
        {
            sendmessage("加载配置失败!错误码[%d]", ret);
            return -1;
        }

        sendmessage("加载配置成功!");
        startGrabbing(currentCamIndex_);
    }
    return 0;
}

// Save configuration of the device
double MVSCamera::Saveconfig(string path)
{
    if (!getCamHandle(currentCamIndex_))
        return -1;

    if (path == "")
    {
        sendmessage("相机配置为空");
        return -1;
    }
    if (m_bStartGrabbing[currentCamIndex_])
    {
        stopGrabbing(currentCamIndex_);
        int ret = MV_CC_FeatureSave(m_pDevHandle[currentCamIndex_], path.c_str());
        if (0 != ret)
        {
            sendmessage("保存相机配置失败, 错误码: 0x%x", ret);
            return -1;
        }
        sendmessage("相机保存配置成功!");
        startGrabbing(currentCamIndex_);
    }
    else
    {
        int ret = MV_CC_FeatureSave(m_pDevHandle[currentCamIndex_], path.c_str());
        if (0 != ret)
        {
            sendmessage("保存相机配置失败, 错误码: 0x%x", ret);
            return -1;
        }
        sendmessage("相机保存配置成功!");
    }
    return 0;
}

// configuration to UserSet
int MVSCamera::getUserSet()
{
    if (!getCamHandle(currentCamIndex_))
        return -1;

    MVCC_ENUMVALUE stEnumValue = {0};
    int nRet = MV_CC_GetEnumValue(m_pDevHandle[currentCamIndex_], "UserSetSelector", &stEnumValue);
    if (MV_OK != nRet)
    {
        sendmessage("获取UserSetSelector失败!, 错误码: 0x%x", nRet);
        return -1;
    }

    int mode = stEnumValue.nCurValue;
    sendmessage("当前UserSetSelector: %d", mode);
    return mode;
}

// Save configuration to UserSet
double MVSCamera::SaveUserSet(int UserSetpath)
{
    if (!getCamHandle(currentCamIndex_))
        return -1;

    if (m_bStartGrabbing[currentCamIndex_])
    {
        stopGrabbing(currentCamIndex_);
        int nRet;
        nRet = MV_CC_SetEnumValue(m_pDevHandle[currentCamIndex_], "UserSetSelector", UserSetpath);
        if (nRet != 0)
        {
            sendmessage("设置UserSetSelector失败, 错误码: 0x%x", nRet);
            return -1;
        }
        nRet = MV_CC_SetEnumValue(m_pDevHandle[currentCamIndex_], "UserSetDefault", UserSetpath);
        if (nRet != 0)
        {
            sendmessage("设置UserSetDefault失败, 错误码: 0x%x", nRet);
            return -1;
        }
        nRet = MV_CC_SetCommandValue(m_pDevHandle[currentCamIndex_], "UserSetSave");
        if (nRet != 0)
        {
            sendmessage("设置UserSetSave失败, 错误码: 0x%x", nRet);
            return -1;
        }
        nRet = MV_CC_SetCommandValue(m_pDevHandle[currentCamIndex_], "UserSetLoad");
        if (nRet != 0)
        {
            sendmessage("设置UserSetLoad失败, 错误码: 0x%x", nRet);
            return -1;
        }
        //        StartCamera();
    }
    return 0;
}
