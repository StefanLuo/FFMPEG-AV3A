#include <iomanip>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <Eigen/Core>
#include <algorithm>
#include <sstream>

#include "render/bs2051.hpp"
#include "render/decorrelate.hpp"
#include "render/layout.hpp"
#include "render/dsp/block_convolver.hpp"
#include "render/fft.hpp"
#include "render/dsp/delay_buffer.hpp"
#include "render/dsp/gain_interpolator.hpp"
#include "render/dsp/ptr_adapter.hpp"
#include "render/render.hpp"
#include "render/Bw64/bw64.hpp"

#include "kissfft/kissfft.hh"

#include "adm/parse.hpp"
#include "adm/common_definitions.hpp"
#include "adm/private/xml_parser.hpp"
#include "adm/document.hpp"
#include "adm/elements.hpp"
#include "adm/route_tracer.hpp"
#include "adm/elements/audio_block_format_objects.hpp"
#include "adm/elements/speaker_position.hpp"

#include "audio_render_processor.hpp"


using namespace bw64;
using namespace render;
using namespace adm;
using namespace render::dsp;

std::string VERSION = "0.7.0";
const int BLOCK_SIZE = 512;


namespace render {
	namespace plugin {

		template <>
		struct BufferTraits<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>> {
			using Buffer = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>;
			using SampleType = float;
			static Eigen::Index channelCount(const Buffer& b) { return b.cols(); };
			static Eigen::Index size(const Buffer& b) { return b.rows(); }
			static const SampleType* getChannel(const Buffer& b, std::size_t n) {
				return b.col(n).data();
			}
			static SampleType* getChannel(Buffer& b, std::size_t n) {
				return b.col(n).data();
			}
		};
	}
}

DirectSpeakersTypeMetadata tmWithLabels(std::vector<std::string> labels)
{
	DirectSpeakersTypeMetadata tm;
	tm.speakerLabels = labels;
	return tm;
}

int main(int argc, char** argv)
{
	if (argc == 2)
	{
		if (memcmp(argv[1], "-v", 2) == 0)
		{
			std::cout << "adm render version " << VERSION <<std::endl;
			exit(1);
		}
	}
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

	std::cout << "chunkIds:" << std::endl;
	for (auto& chunk : inFile->chunks()) {
		std::cout << " - " << '\'' << utils::fourCCToStr(chunk.id) << '\''
			<< std::endl;
	}

	if (inFile->hasChunk(utils::fourCC("chna"))) {
		if (auto chnaChunk = inFile->chnaChunk()) {
			std::cout << "ChnaChunk:" << std::endl;
			std::cout << " - numTracks: " << chnaChunk->numTracks() << std::endl;
			std::cout << " - numUids: " << chnaChunk->numUids() << std::endl;
			std::cout << " - audioIds:" << std::endl;
			for (auto audioId : chnaChunk->audioIds()) {
				std::cout << "   - ";
				std::cout << audioId.trackIndex() << ", " << audioId.uid() << ", "
					<< audioId.trackRef() << ", " << audioId.packRef()
					<< std::endl;
			}
		}
	}

	auto axmlChunk = inFile->axmlChunk();  // get axml chunk
	auto chnaChunk = inFile->chnaChunk();  // get chna chunk 

	if (inFile->hasChunk(utils::fourCC("chna")))
	{
		if (chnaChunk)
		{
			if (axmlChunk)
			{
				std::stringstream axmlStringstream;
				axmlChunk->write(axmlStringstream);

				auto parsedDocument = parseXml(axmlStringstream, adm::xml::ParserOptions::recursive_node_search);
				auto document = parsedDocument->deepCopy();
				auto chnaIds = chnaChunk->audioIds();
				
				for (auto id : chnaIds)
				{
					auto audioPackFormat = document->lookup(parseAudioPackFormatId(id.packRef()));
					auto audioObjectFormat = document->lookup(parseAudioObjectId(id.packRef().replace(0, 7, "AO_")));
					auto audioType = audioPackFormat->get<AudioPackFormatId>().get<TypeDescriptor>();					

					if (audioType == TypeDefinition::OBJECTS)
					{
						// make the gain calculator
						Layout layout = getLayout(argv[3]);//.withoutLfe();
						GainCalculatorObjects gc(layout);

						auto startTime = audioObjectFormat->get<Start>().get();
						int startPosion = inFile->sampleRate() * startTime.count() / 1000000000;
						inFile->seek(startPosion);

						int out_channels = layout.channels().size();
						int in_channels = inFile->channels();

						std::cout << "out_channels:" << out_channels << std::endl;
						std::cout << "in_channels:" << in_channels << std::endl;

						std::vector<std::string> channelNames = layout.channelNames();

						// make the input data; just left of centre
						ObjectsTypeMetadata otm;

						// calculate the direct and diffuse gains
						std::vector<float> directGains(out_channels);
						std::vector<float> diffuseGains(out_channels);
						auto outFile = writeFile(argv[2], out_channels, inFile->sampleRate(), inFile->bitDepth());

						auto audioChannelFormat = document->lookup(parseAudioChannelFormatId(id.packRef().replace(0, 2, "AC")));
						auto audioBlockFormats = audioChannelFormat->getElements<AudioBlockFormatObjects>();
						int  blocks = audioBlockFormats.size();
						std::cout << "blocks:" << blocks << std::endl;

						int blockSize_1 = 512;
						in_channels = in_channels * 2;
						render::plugin::MonitoringAudioProcessor processor(in_channels, layout, blockSize_1);

						int blockL = 0;
						int blockB = 0;
						for (int j = 0; j < blocks; ++j)
						{
							
							auto rTime = audioBlockFormats[j].get<Rtime>().get();
							auto duration = audioBlockFormats[j].get<Duration>().get();
							auto gain = audioBlockFormats[j].get<Gain>().get();
							auto importance = audioBlockFormats[j].get<Importance>().get();

							bool isCartesian = audioBlockFormats[j].get<Cartesian>().get();
							auto diffuse = audioBlockFormats[j].get<Diffuse>().get();
							auto channelLock = audioBlockFormats[j].get<adm::ChannelLock>().get<adm::ChannelLockFlag>();
							auto divergence = audioBlockFormats[j].get<adm::ObjectDivergence>().get<adm::Divergence>().get();
							auto jumpPositionFlag = audioBlockFormats[j].get<adm::JumpPosition>().get<adm::JumpPositionFlag>();
							auto interPolationLength = audioBlockFormats[j].get<adm::JumpPosition>().get<adm::InterpolationLength>().get();
							auto screenRef = audioBlockFormats[j].get<adm::ScreenRef>();

							auto width = audioBlockFormats[j].get<Width>().get();
							auto height = audioBlockFormats[j].get<Height>().get();
							auto depth = audioBlockFormats[j].get<Depth>().get();

							otm.cartesian = isCartesian;
							if (isCartesian)
							{
								auto x = audioBlockFormats[j].get<adm::CartesianPosition>().get<X>().get();
								auto y = audioBlockFormats[j].get<adm::CartesianPosition>().get<Y>().get();
								auto z = audioBlockFormats[j].get<adm::CartesianPosition>().get<Z>().get();
								otm.position = render::CartesianPosition(x, y, z);
							}
							else
							{
								auto polarPosition = audioBlockFormats[j].get<SphericalPosition>();
								auto azimuth = polarPosition.get<Azimuth>().get();
								auto elevation = polarPosition.get<Elevation>().get();
								auto distance = polarPosition.get<Distance>().get();
								otm.position = PolarPosition(azimuth, elevation, distance);
							}

							otm.depth = depth;
							otm.width = width;
							otm.height = height;
							otm.diffuse = diffuse;
							otm.gain = gain;

							gc.calculate(otm, directGains, diffuseGains);

							blockL = inFile->sampleRate() * duration.count() / 1000000000;

							

							blockL = blockL + blockB;
                           // std::cout << "blockLength:" << blockL << std::endl;
							render::plugin::GainMatrix gainDirect = Eigen::MatrixXf::Zero(out_channels, in_channels);
							render::plugin::GainMatrix gainDiffuse = Eigen::MatrixXf::Zero(out_channels, in_channels);

							gainDirect = Eigen::Map<Eigen::MatrixXf>(&directGains[0], out_channels, in_channels);
							gainDiffuse = Eigen::Map<Eigen::MatrixXf>(&diffuseGains[0], out_channels, in_channels);

							processor.setInterp_points(blockL, gainDirect, gainDiffuse);

							blockB = blockL;
						}

						for (int i = 0; i < blocks; ++i)
						{
							auto rTime = audioBlockFormats[i].get<Rtime>().get();
							auto duration = audioBlockFormats[i].get<Duration>().get();
							auto gain = audioBlockFormats[i].get<Gain>().get();
							auto importance = audioBlockFormats[i].get<Importance>().get();

							bool isCartesian = audioBlockFormats[i].get<Cartesian>().get();
							auto diffuse = audioBlockFormats[i].get<Diffuse>().get();
							auto channelLock = audioBlockFormats[i].get<adm::ChannelLock>().get<adm::ChannelLockFlag>();
							auto divergence = audioBlockFormats[i].get<adm::ObjectDivergence>().get<adm::Divergence>().get();
							auto jumpPositionFlag = audioBlockFormats[i].get<adm::JumpPosition>().get<adm::JumpPositionFlag>();
							auto interPolationLength = audioBlockFormats[i].get<adm::JumpPosition>().get<adm::InterpolationLength>().get();
							auto screenRef = audioBlockFormats[i].get<adm::ScreenRef>();

							auto width = audioBlockFormats[i].get<Width>().get();
							auto height = audioBlockFormats[i].get<Height>().get();
							auto depth = audioBlockFormats[i].get<Depth>().get();

							otm.cartesian = isCartesian;
							if (isCartesian)
							{
								auto x = audioBlockFormats[i].get<adm::CartesianPosition>().get<X>().get();
								auto y = audioBlockFormats[i].get<adm::CartesianPosition>().get<Y>().get();
								auto z = audioBlockFormats[i].get<adm::CartesianPosition>().get<Z>().get();
								otm.position = render::CartesianPosition(x, y, z);
							}
							else
							{
								auto polarPosition = audioBlockFormats[i].get<SphericalPosition>();
								auto azimuth = polarPosition.get<Azimuth>().get();
								auto elevation = polarPosition.get<Elevation>().get();
								auto distance = polarPosition.get<Distance>().get();
								otm.position = PolarPosition(azimuth, elevation, distance);
							}

							otm.depth =  depth;
							otm.width = width;
							otm.height =  height;
							otm.diffuse = diffuse;
							otm.gain = gain;

							gc.calculate(otm, directGains, diffuseGains);

							int blockLength = inFile->sampleRate() * duration.count() / 1000000000;
							
							if (blockLength == 0)
							{
								continue;
							}
		

							//processor.delayInSamples();

							if (!inFile->eof())
							{								
								std::size_t blockSize = blockSize_1;//blockLength;

								for (int m = 0; m <= blockLength / blockSize; m++)
								{
									// render::plugin::MonitoringAudioProcessor processor(in_channels, layout, blockSize);
									std::vector<float> in_buffer((/*processor.delayInSamples() + */blockSize)*in_channels);
									std::vector<float> out_buffer((/*processor.delayInSamples() +*/ blockSize)*out_channels);

									int readlen = blockSize;
									if (m == blockLength / blockSize)
									{
										readlen = blockLength - m * blockSize;
									}
									if (readlen == 0)
									{
										break;
									}
									auto readFrames = inFile->read(&in_buffer[0], readlen/*blockSize*/);


									using Buffer = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>;

									Buffer in(/*processor.delayInSamples() +*/ blockSize, in_channels);
									in.setZero();
									in = Eigen::Map<Eigen::MatrixXf>(&in_buffer[0], /*processor.delayInSamples() +*/ blockSize, in_channels);



									Buffer out(/*processor.delayInSamples() +*/ blockSize, out_channels);
									out.setZero();

									render::plugin::GainMatrix gainDirect = Eigen::MatrixXf::Zero(out_channels, in_channels);
									render::plugin::GainMatrix gainDiffuse = Eigen::MatrixXf::Zero(out_channels, in_channels);

									gainDirect = Eigen::Map<Eigen::MatrixXf>(&directGains[0], out_channels, in_channels);
									gainDiffuse = Eigen::Map<Eigen::MatrixXf>(&diffuseGains[0], out_channels, in_channels);


									processor.process(in, out, gainDirect, gainDiffuse);

									for (int j = 0; j < blockSize; j++)
									{
										for (int k = 0; k < out_channels; k++)
										{
											out_buffer[j * out_channels + k] = out(j, k);
										}
									}


									outFile->write(&out_buffer[0], /*blockSize*/readlen);
								}
							}
						}
						goto stoped;
					}
					if (audioType == TypeDefinition::DIRECT_SPEAKERS)
					{
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
						goto stoped;
					}
					if (audioType == TypeDefinition::HOA)
					{
						Layout layout = getLayout(argv[3]);
						GainCalculatorHOA gc(layout);
						HOATypeMetadata tm;
						tm.orders = { 0, 1, 1, 1 };
						tm.degrees = { 0, -1, 0, 1 };

						int out_channel = layout.channels().size();
						int in_channel = inFile->channels();

						auto audioChannelFormat = document->lookup(parseAudioChannelFormatId(id.packRef().replace(0, 2, "AC")));
						auto audioBlockFormats = audioChannelFormat->getElements<AudioBlockFormatObjects>();

						std::vector<std::string> channelNames = layout.channelNames();
						std::vector<std::vector<double>> hoaGains(4);
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
						goto stoped;
					}
					if (audioType == TypeDefinition::MATRIX)
					{

					}
					if (audioType == TypeDefinition::BINAURAL)
					{

					}
				}
			}
		}
	}
stoped:
	return 0;
}
