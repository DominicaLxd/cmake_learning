#include <iostream>
#include <fstream>
#include "opencv2/opencv.hpp"
//#include "include/mfcc.hpp"
//#include "include/sas_util.h"
#include "wavreader/wavreader.hpp"
#include "include/AudioFFT.hpp"

int nSamplesPerSec = 16000;                     //采样率(每秒样本数) //Sample rate.(keda, thchs30, aishell)
int length_DFT = 2048;                          //傅里叶点数 //fft points (samples)
int hop_length = int(0.05 * nSamplesPerSec);    //步长 //下一帧取数据相对于这一帧的右偏移量
int win_length = int(0.1 * nSamplesPerSec);     //帧长 //假设16000采样率，则取取0.1s时间的数据
int number_filterbanks = 80;                    //过滤器数量 //Number of Mel banks to generate
float preemphasis = 0.97;                       //预加重（高通滤波器比例值）
int min_db = -100;
int ref_db = 20;
int r = 1;                                      //librosa里的r=1，暂未深入分析其作用
double pi = 3.14159265358979323846;

cv::Mat_<double> mel_basis;
cv::Mat_<float> hannWindow;

//"""Convert Hz to Mels"""
double hz_to_mel(double frequencies, bool htk = false) {
    if (htk) {
        return 2595.0 * log10(1.0 + frequencies / 700.0);
    }
    // Fill in the linear part
    double f_min = 0.0;
    double f_sp = 200.0 / 3;
    double mels = (frequencies - f_min) / f_sp;
    // Fill in the log-scale part
    double min_log_hz = 1000.0;                         // beginning of log region (Hz)
    double min_log_mel = (min_log_hz - f_min) / f_sp;   // same (Mels)
    double logstep = log(6.4) / 27.0;              // step size for log region

    // 对照Python平台的librosa库，移植
    //如果是多维数列
//    if (frequencies.ndim) {
//        // If we have array data, vectorize
//        log_t = (frequencies >= min_log_hz)
//        mels[log_t] = min_log_mel + np.log(frequencies[log_t] / min_log_hz) / logstep
//    } else
    if (frequencies >= min_log_hz) {
        // If we have scalar data, heck directly
        mels = min_log_mel + log(frequencies / min_log_hz) / logstep;
    }
    return mels;
}

//"""Convert mel bin numbers to frequencies"""
cv::Mat_<double> mel_to_hz(cv::Mat_<double> mels, bool htk = false) {
//    if (htk) {
//        return //python://700.0 * (10.0**(mels / 2595.0) - 1.0);
//    }
    // Fill in the linear scale
    double f_min = 0.0;
    double f_sp = 200.0 / 3;
    cv::Mat_<double> freqs = mels * f_sp + f_min;
    // And now the nonlinear scale
    double min_log_hz = 1000.0;                         // beginning of log region (Hz)
    double min_log_mel = (min_log_hz - f_min) / f_sp;   // same (Mels)
    double logstep = log(6.4) / 27.0;              // step size for log region
    // 对照Python平台的librosa库，移植
    //if (mels.ndim) {
    // If we have vector data, vectorize
    cv::Mat_<bool> log_t = (mels >= min_log_mel);
    for (int i = 0; i < log_t.cols; i++) {
        if (log_t(0, i)) {
            freqs(0, i) = cv::exp((mels(0, i) - min_log_mel) * logstep) * min_log_hz;
        }
    }
    //}
    return freqs;
}

// 生成等差数列，类似np.linspace
cv::Mat_<double> cvlinspace(double min_, double max_, int length) {
    auto cvmat = cv::Mat_<double>(1, length);
    for (int i = 0; i < length; i++) {
        cvmat(0, i) = ((max_ - min_) / (length - 1) * i) + min_;
    }
    return cvmat;
}

cv::Mat_<double> mel_spectrogram_create(int nps, int n_fft, int n_mels) {
    // nps -> samplerate
    double f_max = nps / 2.0;
    double f_min = 0;
    int n_fft_2 = 1 + n_fft / 2;

    // Initialize the weight 
    auto weights = cv::Mat_<double>(n_mels, n_fft_2, 0.0);
    // Center freqs of each FFT bin
    auto fftfreqs = cvlinspace(f_min, f_max, n_fft_2);

    // Center freqs of mel bands 
    double min_mel = hz_to_mel(f_min, false);
    double max_mel = hz_to_mel(f_max, false);

    auto mels = cvlinspace(min_mel, max_mel, n_mels + 2);
    auto mel_f = mel_to_hz(mels, false);

    //auto fdiff_ = nc::diff(mel_f_); //沿着指定轴计算第N维的离散差值(后一个元素减去前一个元素)
    cv::Mat_<double> d1(1, mel_f.cols * mel_f.rows - 1, (double *) (mel_f.data) + 1);
    cv::Mat_<double> d2(1, mel_f.cols * mel_f.rows - 1, (double *) (mel_f.data));
    cv::Mat_<double> fdiff = d1 - d2;

    //auto ramps = nc::subtract.outer(mel_f, fftfreqs); //nc没有subtract.outer
    //nc::NdArray<double> ramps = nc::zeros<double>(mel_f.cols, fftfreqs.cols);
    auto ramps = cv::Mat_<double>(mel_f.cols, fftfreqs.cols);
    for (int i = 0; i < mel_f.cols; i++) {
        for (int j = 0; j < fftfreqs.cols; j++) {
            ramps(i, j) = mel_f(0, i) - fftfreqs(0, j);
        }
    }

    for (int i = 0; i < n_mels; i++) {
        // lower and upper slopes for all bins
        //auto ramps_1 = nc::NdArray<double>(1, ramps.cols);
        auto ramps_1 = cv::Mat_<double>(1, ramps.cols);
        for (int j = 0; j < ramps.cols; j++) {
            ramps_1(0, j) = ramps(i, j);
        }
        //auto ramps_2 = nc::NdArray<double>(1, ramps.cols);
        auto ramps_2 = cv::Mat_<double>(1, ramps.cols);
        for (int j = 0; j < ramps.cols; j++) {
            ramps_2(0, j) = ramps(i + 2, j);
        }
        cv::Mat_<double> lower = ramps_1 * -1 / fdiff(0, i);
        cv::Mat_<double> upper = ramps_2 / fdiff(0, i + 1);
        // .. then intersect them with each other and zero
        //auto weights_1 = nc::maximum(nc::zeros<double>(1, ramps.cols), nc::minimum(lower, upper));
        cv::Mat c1 = lower;//(cv::Mat_<double>(1,5) << 1,2,-3,4,-5);
        cv::Mat c2 = upper;
        cv::Mat weights_1 = cv::Mat_<double>(1, lower.cols);
        cv::min(c1, c2, weights_1);
        cv::max(weights_1, 0, weights_1);
        for (int j = 0; j < n_fft_2; j++) {
            weights(i, j) = weights_1.at<double_t>(0, j);
        }
    }

    // Slaney-style mel is scaled to be approx constant energy per channel
    auto enorm = cv::Mat_<double>(1, n_mels);
    for (int j = 0; j < n_mels; j++) {
        enorm(0, j) = 2.0 / (mel_f(0, j + 2) - mel_f(0, j));
    }
    for (int j = 0; j < n_mels; j++) {
        for (int k = 0; k < n_fft_2; k++) {
            weights(j, k) *= enorm(0, j);
        }
    }
    return weights;
}


cv::Mat_<double> MagnitudeSpectrogram(const cv::Mat_<float> *emphasis_data, int n_fft = 2048, int hop_length = 0,
                                      int win_length = 0){
    if (win_length == 0) {
        win_length = n_fft;
    }
    if (hop_length == 0) {
        hop_length = win_length / 4;
    }

    // reflect对称填充,padding emphasis_data
    int pad_lenght = n_fft / 2;
    // 使用opencv里的copyMakeBorder来完成reflect填充
    cv::Mat_<float> cv_padbuffer;
    cv::copyMakeBorder(*emphasis_data, cv_padbuffer, 0, 0, pad_lenght, pad_lenght, cv::BORDER_REFLECT_101);

    // windowing加窗：将每一帧乘以汉宁窗，以增加帧左端和右端的连续性。
    // 生成一个1600长度的hannWindow，并居中到2048长度的
    if (hannWindow.empty()) {
        hannWindow = cv::Mat_<float>(1, n_fft, 0.0f);
        int insert_cnt = 0;
        if (n_fft > win_length) {
            insert_cnt = (n_fft - win_length) / 2;
        }
        else {
            std::cerr << "\tn_fft:" << n_fft << " > win_length:" << n_fft << std::endl;
            return cv::Mat_<double>(0, 0);
        }
        for (int k = 1; k <= win_length; k++) {
            hannWindow(0, k - 1 + insert_cnt) = float(0.5 * (1 - cos(2 * pi * k / (win_length + 1))));
        }
    }
    int size = cv_padbuffer.rows * cv_padbuffer.cols;//padbuffer.size()
    int number_feature_vectors = (size - n_fft) / hop_length + 1;  // 帧长
    int number_coefficients = n_fft / 2 + 1;  // linear_bin
    cv::Mat_<float> feature_vector(number_feature_vectors, number_coefficients, 0.0f);  //最终spec特征维度

    audiofft::AudioFFT fft; //将FFT初始化放在循环外，可达到最优速度
    fft.init(size_t(n_fft));

    for (int i = 0; i <= size - n_fft; i += hop_length) {
        //每次取hop_length 长的数据
        cv::Mat_<float> framef = cv::Mat_<float>(1, n_fft, (float *) (cv_padbuffer.data) + i).clone();
        framef = framef.mul(hannWindow);

        // stft
        // 复数：Xrf实数，Xif虚数。
        cv::Mat_<float> Xrf(1, number_coefficients);
        cv::Mat_<float> Xif(1, number_coefficients);
        fft.fft((float *) (framef.data), (float *) (Xrf.data), (float *) (Xif.data));

        // 求模
        cv::pow(Xrf, 2, Xrf);
        cv::pow(Xif, 2, Xif);
        cv::Mat_<float> cv_feature(1, number_coefficients, &(feature_vector[i / hop_length][0]));
        cv::sqrt(Xrf + Xif, cv_feature);
    }
    cv::Mat_<float> cv_mag;
    cv::transpose(feature_vector, cv_mag);
    cv::Mat_<double> mag;
    cv_mag.convertTo(mag, CV_64FC1);
    return mag;
}

cv::Mat_<double> log_mel(std::vector<float> &ifile_data, int nSamples_per_sec) {
    if (nSamples_per_sec != nSamplesPerSec) {
        std::cout << R"(the "nSamples_per_sec" is not 16000.)" << std::endl;
        return cv::Mat_<double>(0, 0);
    }

    // pre-emphasis 预加重 //高通滤波
    int ifile_length = int(ifile_data.size() / 4);
    cv::Mat_<float> d1(1, ifile_length - 1, (float *) (ifile_data.data()) + 1);
    cv::Mat_<float> d2(1, ifile_length - 1, (float *) (ifile_data.data()));
    cv::Mat_<float> cv_emphasis_data;
    cv::hconcat(cv::Mat_<float>::zeros(1, 1), d1 - d2 * preemphasis, cv_emphasis_data);

    auto mag = MagnitudeSpectrogram(&cv_emphasis_data, length_DFT, hop_length, win_length);
    mag = cv::abs(mag);  // spec power == 1

    if (mel_basis.empty()) {
        mel_basis = mel_spectrogram_create(nSamplesPerSec, length_DFT, number_filterbanks);
    }

    // dot mel_basis
    cv::Mat cv_mel = mel_basis * mag;

    // power2db
    cv::log(cv::max(cv_mel, 1e-5), cv_mel); // np.log(np.max(1e-5, amp))
    cv_mel = cv_mel / 2.3025850929940459 * 20;

    // - ref_level_db
    cv_mel = cv_mel - ref_db;  // ref_level_db;
    // norm  mel = np.clip((mel - min_db) / hp.min_db, 0, 1)
    cv_mel = (cv_mel - min_db) / min_db;
    //cv_mel = cv::max(cv::min(cv_mel, 1.0) ,0.);
    return cv_mel; // mel_spec
}


int main(int argc, char ** argv) {
    std::cout << "in main" << std::endl;
    std::vector<float> table = readWAV("/Users/dominica/Downloads/learning_cpp/cmake_learning/MFCC/wavreader/city_16k.wav");    std::cout << table[0] << std::endl;

    // 运算mel特征向量 //11ms
    //auto stime = NowTime<milliseconds>();
    auto mel = log_mel(table, 16000);
    std::cout << mel << std::endl;
    //auto mag = log_mel(&table, length_DFT, hop_length, win_length);
    //std::cout << mag << std::endl;
    //exit(0);
    //auto mel = pcen(ifile_data, 16000);
    //auto etime = NowTime<milliseconds>();
    //std::cout << etime - stime << std::endl;
   

    return 0;
}