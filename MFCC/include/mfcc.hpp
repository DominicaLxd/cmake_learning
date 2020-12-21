#pragma once

#include "AudioFFT.hpp"
#include "opencv2/opencv.hpp"
#include "iir_filter.hpp"
#include "sas_util.h"

int nSamplesPerSec = 16000;                     // sample_rate 采样率
int length_DFT = 2048;                          // n_fft 傅立叶点数
int hop_length = int(0.0125 * nSamplesPerSec);  // 200 步长                 // 200
int win_length = int(0.05 * nSamplesPerSec);    // 800 帧长
int number_filterbanks = 80;                    // Mel_bins
float preemphasis = 0.97;                       // 预加重
int max_db = 100;
int ref_db = 20;
int r = 1;                              // unknowen params
double pi = 3.14159265358979323846;

cv::Mat_<double> mel_basis;
cv::Mat_<float> hannWindow;


/*********************************************
* Short-time Fourier transform (STFT)
* 
*********************************************/
cv::Mat_<double> MagnitudeSpectrogram(const cv::Mat_<float> *wav_data, 
int n_fft = 2048, int hop_length = 0, int win_length = 0) {
    if (win_length == 0) {
        win_length = n_fft;
    }
    if (hop_length == 0) {
        hop_length = hop_length / 4;
    }

    // reflect 对称填充
    int pad_length = n_fft / 2;
    cv::Mat_<float> cv_padbuffer;
    cv::copyMakeBorder(*wav_data, cv_padbuffer, 0, 0, pad_length, pad_length, cv::BORDER_REFLECT_101);

    // 加窗 hanmming windows
    if (hannWindow.empty()) {
        hannWindow = cv::Mat_<float>(1, n_fft, 0.0f);
        int insert_cnt = 0;
        if (n_fft > win_length) {
            insert_cnt = (n_fft - win_length) / 2;
        }else {
            std::cout << "\tn_fft:" << n_fft << " > win_length:" << n_fft << std::endl;
            return cv::Mat_<double>(0, 0);
        }
        for (int k = 1; k <= win_length; k++) {
            hannWindow(0, k - 1 + insert_cnt) = float(0.5 * (1 - cos(2 * pi * k / (win_length + 1))));
        }
    }

    int size = cv_padbuffer.rows * cv_padbuffer.cols;//padbuffer.size()
    int number_feature_vectors = (size - n_fft) / hop_length + 1;
    int number_coefficients = n_fft / 2 + 1;  // linear_bins
    cv::Mat_<float> feature_vector(number_feature_vectors, number_coefficients, 0.0f);

    // FFT 初始化
    audiofft::AudioFFT fft;
    fft.init(size_t(n_fft));
    for (int i = 0; i <= size - n_fft; i += hop_length) {
        // 取一帧数据
        cv::Mat_<float> framef = cv::Mat_<float>(1, n_fft, (float *) (cv_padbuffer.data) + i).clone();
        // 加hann窗
        framef = framef.mul(hannWindow);

        // 复数: Xrf 实部, Xif 虚部
        cv::Mat_<float> Xrf(1, number_coefficients);
        cv::Mat_<float> Xif(1, number_coefficients);
        fft.fft((float *)(framef.data), (float *)(Xrf.data), (float *)(Xif.data));

        // 求模 mod
        cv::pow(Xrf, 2, Xrf);
        cv::pow(Xif, 2, Xif);
        cv::Mat_<float> cv_feature(1, number_coefficients, &(feature_vector[i / hop_length][0]));
        cv::sqrt(Xrf + Xif, cv_feature);
    }
    cv::Mat_<float> cv_mag;
    cv::transpose(feature_vector, cv_mag);
    cv::Mat_<double> mag;

    //CV_64FC1   64F代表每一个像素点元素占64位浮点数，通道数为1
    //CV_64FC3   64F代表每一个像素点元素占64点×3个浮点数，通道数为4
    cv_mag.convertTo(mag, CV_64FC1);
    return mag;
}


/*********************************************
 * log_mel
 * function:    input wav，output log-mel features
 * paramaters:  @ifile_data        wav
 *              @nSamples_per_sec  samplerate
 * return: cv::Mat_<double>   log-mel features
*********************************************/
cv::Mat_<double> log_mel(std::vector<float> &ifile_data, int nSamples_per_sec) {
    if (nSamples_per_sec != nSamplesPerSec) {
        std::cout << R"(the "nSamples_per_sec" is not 16000.)" << std::endl;
        return cv::Mat_<double>(0, 0);
    }

    /*
    // pre-emphasis 预加重 //高通滤波
    int ifile_length = int(ifile_data.size());
    cv::Mat_<float> d1(1, ifile_length - 1, (float *) (ifile_data.data()) + 1);
    cv::Mat_<float> d2(1, ifile_length - 1, (float *) (ifile_data.data()));
    cv::Mat_<float> cv_emphasis_data;
    cv::hconcat(cv::Mat_<float>::zeros(1, 1), d1 - d2 * preemphasis, cv_emphasis_data);
    */
    cv::Mat_<float> cv_emphasis_data(ifile_data);
    // magnitude spectrogram 幅度谱图
    auto mag = MagnitudeSpectrogram(&cv_emphasis_data, length_DFT, hop_length, win_length);

}
