#include "multichannel_convolver.hpp"
#include <memory>
#include <iostream>

namespace render {
namespace plugin {
MultichannelConvolver::MultichannelConvolver(
    std::vector<std::vector<float>> filters, std::size_t blockSize)
    : blockSize_(blockSize) {
  auto context =
      dsp::block_convolver::Context(blockSize, render::get_fft_kiss<float>());

  for (const auto& filterVector : filters) {
    dsp::block_convolver::Filter filter(context, filterVector.size(),
                                        filterVector.data());
    convolvers_.push_back(
        std::make_unique<dsp::block_convolver::BlockConvolver>(context,
                                                               filter));
  }
}

void MultichannelConvolver::process(const Eigen::Ref<const Eigen::MatrixXf>& in,
                                    Eigen::Ref<Eigen::MatrixXf> out) {
  if (in.cols() != out.cols()) {

      std::cout << "1MultichannelConvolver" << std::endl;
    throw std::invalid_argument(
        "Input and output must have the same number of channels");
  }
  if (in.rows() != out.rows()) {
      std::cout << "2MultichannelConvolver" << std::endl;
    throw std::invalid_argument(
        "Input and output must have the same number of samples");
  }
  if (in.cols() != convolvers_.size()) {
      std::cout << "3MultichannelConvolver" << convolvers_.size() <<std::endl;
    throw std::invalid_argument(
        "Input channel count must match the number of filters/convolvers");
  }
  if (in.rows() != blockSize_) {
      std::cout << "4MultichannelConvolver" << std::endl;
    throw std::invalid_argument(
        "Input sample count must match the convolver block size");
  }

  for (std::size_t n = 0; n < convolvers_.size(); ++n) {
    convolvers_[n]->process(in.col(n).data(), out.col(n).data());
  }
}

}  // namespace plugin
}  // namespace rendeer
