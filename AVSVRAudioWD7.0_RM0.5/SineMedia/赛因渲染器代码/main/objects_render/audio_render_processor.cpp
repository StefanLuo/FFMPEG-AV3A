#include "audio_render_processor.hpp"
#include "render/dsp/gain_interpolator.hpp"
#include "render/decorrelate.hpp"
#include <functional>

#include <iostream>

using std::placeholders::_1;
using std::placeholders::_2;

namespace render {
namespace plugin {

inline std::vector<std::vector<float>> convertToVec(GainMatrix m) {
  std::vector<std::vector<float>> vec;
  for (int i = 0; i < m.cols(); ++i) {
    vec.push_back(
        std::vector<float>(m.col(i).data(), m.col(i).data() + m.rows()));
  }
  return vec;
}

MonitoringAudioProcessor::MonitoringAudioProcessor(
    std::size_t inputChannelCount, Layout layout, std::size_t blockSize)
    : inputChannelCount_(inputChannelCount),
      internalBlockSize_(blockSize),
      blockAdapter_(
          blockSize, inputChannelCount, layout.channels().size(),
          std::bind(&MonitoringAudioProcessor::doBlockedProcess, this, _1, _2)),
      directPathDelay_(layout.channels().size(), blockSize),
      bufferA_(blockSize, layout.channels().size()),
      bufferB_(blockSize, layout.channels().size()),
	  bufferC_(blockSize, layout.channels().size()),
      currentDirectGains_(layout.channels().size(), inputChannelCount_),
      currentDiffuseGains_(layout.channels().size(), inputChannelCount_),
      nextDirectGains_(layout.channels().size(), inputChannelCount_),
      nextDiffuseGains_(layout.channels().size(), inputChannelCount_),
      convolver_(render::designDecorrelators<float>(layout), blockSize) 

      
{
  currentDirectGains_.setZero();
  currentDiffuseGains_.setZero();
  nextDirectGains_.setZero();
  nextDiffuseGains_.setZero();

  currentSampleIndex = 512;
 
}

std::size_t MonitoringAudioProcessor::delayInSamples() const {
  return blockAdapter_.get_delay() + directPathDelay_.get_delay();
}


void MonitoringAudioProcessor::setInterp_points(dsp::SampleIndex index, const GainMatrix& direct,
    const GainMatrix& diffuse)
{
    std::vector<std::vector<float>> direct_v =
        convertToVec(direct);
    std::vector<std::vector<float>> diffuse_v =
        convertToVec(diffuse);
    interp_.interp_points.emplace_back(index, direct_v);
    interpdiffuse_.interp_points.emplace_back(index, diffuse_v);
}
void MonitoringAudioProcessor::doBlockedProcess(
    const Eigen::Ref<const Eigen::MatrixXf>& in,
    Eigen::Ref<Eigen::MatrixXf> out) {
  // in -> gain_interp_direct -> buffer_a
  // buffer_a -> delay_buffer -> out
  // in -> gain_interp_diffuse -> buffer_c
  // buffer_c -> convolvers > buffer_b
  // out += buffer_b

  render::dsp::PtrAdapterConst in_p(in.cols());
  in_p.set_eigen(in);

  render::dsp::PtrAdapter out_p(out.cols());
  out_p.set_eigen(out);

  dsp::PtrAdapter bufferA_p(bufferA_.cols());
  bufferA_p.set_eigen(bufferA_);
 
  dsp::PtrAdapter bufferC_p(bufferC_.cols());
  bufferC_p.set_eigen(bufferC_);

  // Apply gain ramp for direct path
  std::vector<std::vector<float>> currentDirectGains_v =
      convertToVec(currentDirectGains_);
  std::vector<std::vector<float>> nextDirectGains_v =
      convertToVec(nextDirectGains_);

  interp_.process(/*block_start*/currentSampleIndex, in.rows(), in_p.ptrs(), /*out_p.ptrs()*/bufferA_p.ptrs());
  currentDirectGains_ = nextDirectGains_;

  // delay direct path to align with diffuse path
  directPathDelay_.process(internalBlockSize_, bufferA_p.ptrs(), out_p.ptrs());

  // apply gain ramp for diffuse path
  std::vector<std::vector<float>> currentDiffuseGains_v =
      convertToVec(currentDiffuseGains_);
  std::vector<std::vector<float>> nextDiffuseGains_v =
      convertToVec(nextDiffuseGains_);

  interpdiffuse_.process(/*block_start*/currentSampleIndex, in.rows(), in_p.ptrs(), bufferC_p.ptrs());

  currentDiffuseGains_ = nextDiffuseGains_;

  convolver_.process(bufferC_, bufferB_);
 
  out += bufferB_;

  currentSampleIndex += 512;



  ///////////////////////////////
  ///////////////////////////


}

}  // namespace plugin
}  // namespace render
