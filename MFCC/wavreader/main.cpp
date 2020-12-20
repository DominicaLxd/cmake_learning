#include <iostream>
#include <vector>
#include "wavreader.hpp"

int main(int argc, char *argv[]){
//	std::vector<float> table = readWAV("/Users/dominica/Downloads/learning_cpp/CycleGAN/test.wav");
	std::vector<float> table = readWAV("city_16k.wav");
    std::ofstream fout("wav-self.txt");
	for(auto const& x : table)
		fout << x << '\n';
	return 0;
}
