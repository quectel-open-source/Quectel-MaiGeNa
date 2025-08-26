#pragma once
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include <cstdint>
#pragma execution_character_set("utf-8")
#include "CameraParams.h"
#include <QDebug>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <pthread.h>
#include <string>
#include <vector>

#include "commonZack.h"

using namespace std;
using namespace cv;

/**
 * @brief 安全格式化字符串到缓冲区（类似 snprintf 的封装）
 * @param format 格式化字符串（如 ">> Error: %#x"）
 * @param args   可变参数（对应 format 中的占位符）
 * @return 格式化后的字符串
 * @throws std::runtime_error 如果格式化失败或缓冲区不足
 */

template <typename... Args> std::string formatStr(const char *format, Args... args)
{
    char buf[256]; // 可根据需求调整缓冲区大小
    int written = snprintf(buf, sizeof(buf), format, args...);
    if (written < 0)
    {
        throw std::runtime_error("Formatting failed (snprintf error)");
    }
    else if (static_cast<size_t>(written) >= sizeof(buf))
    {
        throw std::runtime_error("Buffer too small for formatting");
    }

    return std::string(buf);
}

#define CAMERA_NUM_MAX 12 // 支持的最大相机数量
#define CallBack_BufferCounts 20
// typedef void (*CallBack)(string s, void *);
// typedef void (*CallBack2)(MV_FRAME_OUT_INFO_EX s, unsigned char *, void *, int);
using CallBack = std::function<void(string s, void *)>;
using CallBack2 = std::function<void(MV_FRAME_OUT_INFO_EX s, unsigned char *, void *, int)>;

struct CameraData
{
    int camid; /**< 相机ID 1 2 3 4  */
    int pushNum;
    int frameNum;
    int width;
    int height;
    MvGvspPixelType pixelType;
    unsigned char *data = nullptr;
    QDateTime time;
};

struct FrameData
{
    int camid; /**< 相机ID 1 2 3 4  */
    int pushNum;
    int frameNum;
    int width;
    int height;
    MvGvspPixelType pixelType;
    unsigned char *data = nullptr;
    std::int64_t timestamp; // 时间戳, 单位为毫秒
};

class MVSCamera
{
  public:
    MVSCamera();
    ~MVSCamera();

  public:
    static bool enumDevice(std::vector<std::vector<std::string>> &);
    static MV_CC_DEVICE_INFO_LIST stDevList_;

  public:
  int connectCamera(int camIndex);
  int closeCamera(int camIndex);
  bool isConnected(int camIndex);
  int startGrabbing(int camIndex);
  int stopGrabbing(int camIndex);
  bool isGrabbing(int camIndex)
  {
    return m_bStartGrabbing[camIndex];
  }
  void onFrame(MV_FRAME_OUT_INFO_EX *pFrameInfo, unsigned char *pData);
  void onError(int msgType);
  
  void setFrameCallback(std::function<void(FrameData)> callback)
  {
      frameCallback_ = callback;
  }
  void setErrorCallback(std::function<void(int nMsgType)> errorCallback)
  {
    errorCallback_ = errorCallback;
  }
  private:
    std::function<void(FrameData)> frameCallback_;
    std::function<void(int nMsgType)> errorCallback_;

  public:
    int ReadBuffer();
    // ????????????????
    void sendmsg(CallBack call_back, void *);
    // ???????????????
    void sendImage(CallBack2 call_back, void *);
    ///
    void ReconnectDevice(unsigned int nMsgType);
    void initData(int width, int height);
    int StartCamera_2(int run_grab = 0, int trigger_type = 0);
    int ReadBuffer_2();

    int setBestPacketSize();
    static std::string getCurrentTimeString();

    void recvCameraInfo(unsigned char *data, int ww, int hh, int frameNum);
    bool waitForData(CameraData &data, int timeoutMs = 1000);

    bool m_isDataReady = false;
    CameraData m_dataList[ArrayLength];
    std::mutex m_mutexCallback;
    std::condition_variable m_conditionCallback;
    int m_count_software = 0;
    int m_width;
    int m_height;

    int getPayloadSize(int index, int &payloadSize);
    int getWidth(int index, int &width);
    int getHeight(int index, int &height);
    int setWidth(int index, unsigned int width);
    int setHeight(int index, unsigned int height);

    bool softTrigger(int index);
    bool SetTriggerMode(int index, int mode);
    int GetTriggerMode(int index);
    bool SetTriggerDelay(int index, double mode);
    int GetTriggerDelay(double &dTriggerDelay, double &dminTriggerDelay, double &dmaxTriggerDelay);
    double Getgamma(double &dgamma, double &dminGamma, double &dmaxGamma);
    bool Setgamma(double dgamma);
    bool SetExposureTime(int index, double exp);
    int GetExposureTime(int index, float *dExposureTime = nullptr, float *dminExposureTime = nullptr,
                        float *dmaxExposureTime = nullptr);
    double GetContrast(int64_t &dContrast, int64_t &dminContrast, int64_t &dmaxContrast);
    bool SetContrast(int64_t dContrast);
    bool SetGain(int index, double gain);
    float GetGain(float &dGain, float &dminGain, float &dmaxGain);
    int setFrameRateEnable(int index, bool comm);
    bool SetFrameRate(int index, double frame);
    float GetFrameRate(int index);
    double GetHeartbeatTimeout(int64_t &dHeartbeatTimeout, int64_t &dminHeartbeatTimeout,
                               int64_t &dmaxHeartbeatTimeout);
    bool SetHeartbeatTimeout(int64_t dHeartbeatTimeout);
    bool SetTriggerSource(int index, int soft);
    int GetTriggerSource(int index);
    int setOffsetX(int index, unsigned int offsetX);
    int setOffsetY(int index, unsigned int offsetY);
    double GetDebouncer(double &dDebouncer, double &dminDebouncer, double &dmaxDebouncer);
    bool SetDebouncer(double dDebouncer);
    bool SetTriggerActivation(int mode);
    int GetTriggerActivation();
    double loadconfig(string path);
    double Saveconfig(string path);
    int getUserSet();
    double SaveUserSet(int UserSetpath);

  public:
    CallBack _my_call_back = nullptr;
    CallBack2 _sendImage_call_back = nullptr;
    void *callHandle1;
    void *callHandle2;

    unsigned char *pData[CAMERA_NUM_MAX];
    int m_readBufferIndex = 0;
    int currentCamIndex_;

    void *m_pDevHandle[CAMERA_NUM_MAX];
    unsigned char *m_pImageBuffer[CAMERA_NUM_MAX];
    unsigned int m_nImageBufferSize[CAMERA_NUM_MAX];
    bool m_bThreadState;
    bool m_bStartGrabbing[CAMERA_NUM_MAX];
    MV_FRAME_OUT_INFO_EX m_stImageInfo[CAMERA_NUM_MAX];
    int m_nPayloadSize = 0;
    int m_errorCnts = 0;

  private:
    template <typename... Args> void sendmessage(const char *format, Args... args)
    {
        _my_call_back(formatStr(format, args...), callHandle1);
    }
    template <typename...> void sendmessage(const char *format)
    {
        _my_call_back(std::string(format), callHandle1);
    }

    inline void *getCamHandle(int index)
    {
        // validate
        if (index < 0)
        {
            sendmessage("无效相机索引: %d", index);
            return nullptr;
        }
        auto camHandle = m_pDevHandle[index];
        if (!camHandle)
        {
            sendmessage("无效相机句柄: %d", index);
            return nullptr;
        }
        return camHandle;
    }
};
