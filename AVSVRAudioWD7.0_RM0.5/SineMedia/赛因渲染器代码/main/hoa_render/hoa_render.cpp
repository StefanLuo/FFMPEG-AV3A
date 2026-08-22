#include <iomanip>
#include <iostream>
#include <vector>

#include "render/render.hpp"
#include "render/Bw64/bw64.hpp"

using namespace bw64;
using namespace render;

const unsigned int BLOCK_SIZE = 4096;

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
  
  auto axmlChunk = inFile->axmlChunk();  // get axml chunk
  auto chnaChunk = inFile->chnaChunk();  // get chna chunk  
  
  Layout layout = getLayout(argv[3]);
  GainCalculatorHOA gc(layout);

  int out_channel = layout.channels().size();
  int in_channel = inFile->channels();

  HOATypeMetadata tm;

  if (in_channel == 4)
  {
	  tm.orders = { 0, 1,  1, 1 };
	  tm.degrees = { 0, -1, 0, 1 };
  }
  if (in_channel == 9)
  {
	  tm.orders = { 0, 1,  1, 1, 1,  1, 1, 1, 1 };
	  tm.degrees = { 0, -1, 0, 1, 0, -1, 0, 1 ,1 };
  }

  if (in_channel == 16)
  {
	  tm.orders = { 0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 1,  1, 1, 1, 1 };
	  tm.degrees = { 0, -1, 0, 1, 0, -1, 0, 1 ,1 ,1, 1, 1,  1, 1, 1, 1 };
  }
  
  if (in_channel == 25)
  {
	  tm.orders = { 0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 1,  1, 1, 1, 1 ,0, 1,  1, 1, 1,  1, 1, 1, 1 };
	  tm.degrees = { 0, -1, 0, 1, 0, -1, 0, 1 ,1 ,1, 1, 1,  1, 1, 1, 1,0, -1, 0, 1, 0, -1, 0, 1 ,1 };
  }
  if (in_channel == 36)
  {
	  tm.orders = { 0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 1,  1, 1, 1, 1 ,0, 1,  1, 1, 1,  1, 1, 1, 1 ,0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1 };
	  tm.degrees = { 0, -1, 0, 1, 0, -1, 0, 1 ,1 ,1, 1, 1,  1, 1, 1, 1,0, -1, 0, 1, 0, -1, 0, 1 ,1,0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1 };
  }
  if (in_channel == 49)
  {
	  tm.orders = { 0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 1,  1, 1, 1, 1 ,0, 1,  1, 1, 1,  1, 1, 1, 1 ,0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1 ,0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 1,  1 };
	  tm.degrees = { 0, -1, 0, 1, 0, -1, 0, 1 ,1 ,1, 1, 1,  1, 1, 1, 1,0, -1, 0, 1, 0, -1, 0, 1 ,1,0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 1,  1 };
  }

  if (in_channel == 64)
  {
	  tm.orders = { 0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 1,  1, 1, 1, 1 ,0, 1,  1, 1, 1,  1, 1, 1, 1 ,0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1 ,0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 1,  1,0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 1,  1, 1, 1 };
	  tm.degrees = { 0, -1, 0, 1, 0, -1, 0, 1 ,1 ,1, 1, 1,  1, 1, 1, 1,0, -1, 0, 1, 0, -1, 0, 1 ,1,0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 1,  1, 0, 1,  1, 1, 1,  1, 1, 1, 1 ,1, 1, 1,  1, 1, 1 };
  }

 

  std::vector<std::string> channelNames = layout.channelNames();
  std::vector<std::vector<double>> hoaGains(in_channel);
  for (auto& col : hoaGains) col.resize(out_channel);
  for (int i = 0; i < out_channel; i++)
  {
	  gc.calculate(tm, hoaGains);
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
				  sum = sum + in_buffer[i * in_channel + j] * hoaGains[j][n];
			  }
			  out_buffer[i * out_channel + n] = sum;
		  }
	  }
	  outFile->write(&out_buffer[0], readFrames);
  }

  return 0;
}
