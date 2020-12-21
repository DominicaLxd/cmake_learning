#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <assert.h>
#include <memory>
#include <fstream>
#include "opencv2/opencv.hpp"

using namespace std;
using namespace std::chrono;

class parambase
{
public:
    string name;
    string help;
    string strval;
    parambase(){}
    virtual  ~parambase(){}
    virtual bool set(const char* value) {};
};

/**
 * �������
 */
class EnginePar
{
public:
    static int cs_timeout; //�кŷ�����ɵĳ�ʱʱ��(Ĭ���������һ�νкŴ�����һ�����,Ĭ��ֵ5����)
    static int cs_detecthandsup_time; //�кź���������ֵ�ʱ��(Ĭ��10s)
    static int cs_detecthandsup_interval ; //�кź���������ֵ�ʱ����(Ĭ��1��1��)
    static int cs_detectsmile_interval; //�кź�΢Ц����ʱ����(Ĭ��1��1��)
    static int cs_detectspeech_interval;//�кź���������ʱ����(Ĭ��20��)
    static int cs_detectpose_interval;  //�кź���̬����ʱ����(Ĭ��5��1��)
    static int detectpose_interval;     //�ǽк��ڼ���̬����ʱ����(Ĭ��5��1��)
    static int detectsmile_interval;    //�ǽк��ڼ�΢Ц����ʱ����(Ĭ��1��1��)
    static int detectappearance_interval; //��װ�����
    static float action_turnpen_thrd;   //ת����ֵ
    static float action_turnchair_thrd; //ת����ֵ
    static float action_record_time;    //����¼��ʱ��
    static float sit_supporthead_thrd;  //��ͷ��ֵ
    static float sit_layondesk_thrd;    //ſ����ֵ
    static float sit_relyingonchair_thrd;//������ֵ
    static string log_path;
    static string log_level;
    static string temp_path;
	static bool set(const char* key, const char* val);
	static bool haskey(const char* key);
	static const char* getvalue(const char* key);
};
/**
 * ����ͷ��������
 */
enum VideoScene
{
    SCENE_counter,    // ��̨
    SCENE_financial,   // ����
    SCENE_lobby,       // ����
    SCENE_hall             // ����
};
/**
 * ����ͷ��������
 */
class VideoPar
{
private:
    vector<shared_ptr<parambase>> params;
public:
    VideoScene scene;            //����: 1��̨, 2����, 3����, 4����(��װ���)
    bool audio_enable ;          //��Ƶ���� 1��,0��
    int audio_channels ;         //��Ƶͨ���� 0,1,2,4,6
    int audio_sample_rate ;      // ������ 44100, 48000, 96000, 192000
    bool video_enable ;          // ��Ƶ���� 1��,0��
    //int video_analyse_rate ;   //��Ƶ��������: ����>0,ÿ�����֡��
    bool video_sample_keyframe;  //ֻ����ؼ�֡
    bool video_record;           //����¼����Ƶ 1��,0��
    int video_record_duration;   //��Ƶ¼��ʱ��,Ĭ��10s
    int video_record_reviewtime; //��Ƶ¼�ƻ���ʱ��,Ĭ��5s
    int face_minsize;            //��С������С
    VideoPar();
    //~VideoPar();
    bool set(const char* key, const char* val);
    static bool haskey(const char* key);
};

template<class T>
inline int64_t NowTime()
{
	return std::chrono::time_point_cast<T>(std::chrono::system_clock::now()).time_since_epoch().count();
}

/**--------------------------------- ������models����ģ�����õ��ķ��� ---------------------------------**/

inline bool detectFileExist(char *file_path) {
    std::ifstream _ifstream;
    _ifstream.open(file_path, std::ios::in);
    if (!_ifstream) {
        return false;
    }
    _ifstream.close();
    return true;
}

// ����任��������xy������ת
inline cv::Mat_<double> rotate_point(cv::Mat_<double> xy, double angle) {
    cv::Mat rotate_matrix = (cv::Mat_<double>(2, 2) << cos(angle), -sin(angle), sin(angle), cos(angle));
    cv::transpose(rotate_matrix, rotate_matrix);
    auto rotate_xy = xy * rotate_matrix;
    return rotate_xy;
}

// �����Ƿ��ڿ���
inline bool check_point_in_rect(cv::Point point, cv::Rect rect) {
    if ((rect.x < point.x && point.x < rect.x + rect.width) &&
        (rect.y < point.y && point.y < rect.y + rect.height)) {
        return true;//��rect�ڲ�
    } else {
        return false;//��rect���ϻ��ⲿ
    }
}

