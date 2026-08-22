#include <iomanip>
#include <iostream>
#include <vector>
#include <string>

#include "render/render.hpp"
#include "render/Bw64/bw64.hpp"

using namespace bw64;
using namespace render;

const unsigned int BLOCK_SIZE = 4096;
DirectSpeakersTypeMetadata tmWithLabels(std::vector<std::string> labels) 
{
	DirectSpeakersTypeMetadata tm;
	tm.speakerLabels = labels;
	return tm;
}

int main(int argc, char **argv) 
{
  if (argc != 4) 
  {
    std::cout << "usage: " << argv[0] << " [INFILE] [OUTFILE] [LAYOUTFILE]" << std::endl;
    exit(1);
  }
  
  auto inFile = readFile(argv[1]);  
  std::cout << "Format:" << std::endl;
  std::cout << " - formatTag: " << inFile->formatTag() << std::endl;
  std::cout << " - channels: " << inFile->channels() << std::endl;
  std::cout << " - sampleRate: " << inFile->sampleRate() << std::endl;
  std::cout << " - bitDepth: " << inFile->bitDepth() << std::endl;
  std::cout << " - numerOfFrames: " << inFile->numberOfFrames() << std::endl;    
    
  Layout layout = getLayout(argv[3]);
  GainCalculatorDirectSpeakers gc(layout);

  int out_channel = layout.channels().size();
  int in_channel = inFile->channels();

  std::vector<std::string> channelNames = layout.channelNames();
  std::vector<float> directGains[64];
  for (int i = 0; i < out_channel; i++)
  {
	  directGains[i].resize(out_channel);
	  gc.calculate(tmWithLabels(std::vector<std::string>{channelNames[i]}), directGains[i]);
  }

  auto outFile = writeFile(argv[2], out_channel, inFile->sampleRate(), inFile->bitDepth());
  std::vector<float> in_buffer(BLOCK_SIZE * in_channel);
  std::vector<float> out_buffer(BLOCK_SIZE * out_channel);

  while (!inFile->eof())
  {
	  auto readFrames = inFile->read(&in_buffer[0], BLOCK_SIZE);
	  for (int i = 0; i < BLOCK_SIZE; i++)
	  {
		  for (int n = 0; n < out_channel; n++)
		  {
			  double sum = 0;
			  for (int j = 0; j < in_channel; j++)
			  {
				  sum = sum + in_buffer[i * in_channel + j] * directGains[j][n];
			  }
			  out_buffer[i * out_channel + n] = sum;
		  }
	  }
	  outFile->write(&out_buffer[0], readFrames);
  }

  return 0;
}