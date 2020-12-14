#include <iostream> 
#include "static/Hello.h"
#include "gist/Gist.h"
#include "gist/AudioFile.h"

AudioFile<float> loadAudioFile(const std::string &path) {
        //---------------------------------------------------------------
    std::cout << "**********************" << std::endl;
    std::cout << "Running Example: Load Audio File and Print Summary" << std::endl;
    std::cout << "**********************" << std::endl << std::endl;

    const std::string filePath = path;

    AudioFile<float> a;
    bool loadedOK = a.load(filePath);

    assert(loadedOK);
        std::cout << "Bit Depth: " << a.getBitDepth() << std::endl;
    std::cout << "Sample Rate: " << a.getSampleRate() << std::endl;
    std::cout << "Num Channels: " << a.getNumChannels() << std::endl;
    std::cout << "Length in Seconds: " << a.getLengthInSeconds() << std::endl;
    std::cout << std::endl;
    return a;

}

int main(int argc, char * argv[]) {
    Hello hi;
    hi.print();

    AudioFile<float> a = loadAudioFile(argv[1]);
    std::cout << "wav load end" << std::endl;

    int sampleRate = 44100;    
    int frameSize = 512;
    Gist<float> gist (frameSize, sampleRate);
    const std::vector<float>& melSpec = gist.getMelFrequencySpectrum();

    return 0;
}
