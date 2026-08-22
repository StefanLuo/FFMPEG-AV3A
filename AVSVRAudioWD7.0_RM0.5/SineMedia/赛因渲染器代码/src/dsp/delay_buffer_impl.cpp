#include <Eigen/Core>
#include <boost/make_unique.hpp>
#include "render/dsp/delay_buffer.hpp"
#include "render/exceptions.hpp"

#include<iostream>

namespace render {
  namespace dsp {

    /// A multi-channel delay buffer, templated over the type of sample.
    class DelayBufferImpl {
     public:
      /// @param nchannels number of input and output channels
      /// @param nsamples length of the delay
      DelayBufferImpl(size_t nchannels, size_t nsamples)
          : delaymem(Eigen::MatrixXf::Zero(nsamples, nchannels)) {}

      /// Process an arbitrary number of samples. \p input and \p output have
      /// \c nchannels channels and \p nsamples samples.
      void process(size_t nsamples, const float *const *input,
                   float *const *output) {
        Eigen::Index nchannels = delaymem.cols();
        Eigen::Index delay = delaymem.rows();

      //  std::cout << "directprocess inchan:" << nchannels << std::endl;

      //  std::cout << "directprocess delay:" << delay << std::endl;

      //  std::cout << "directprocess nsamples:" << nsamples << std::endl;


        for (Eigen::Index channel = 0; channel < nchannels; channel++) {
          for (Eigen::Index sample = 0; sample < nsamples + delay; sample++) {
            // transfer from:
            //    [delaymem; input]
            // to:
            //    [output; delaymem]
            float value = sample < delay ? delaymem(sample, channel)
                                         : input[channel][sample - delay];

            
            if (sample < nsamples)
            {
               /* if (value > 0)
                {
                    std::cout << "directprocess val:" << value << std::endl;
                }*/
                output[channel][sample] = value;
            }
            else
            {
               /* if (value > 0)
                {
                    std::cout << "directprocess delay:" << value << std::endl;
                }*/
                delaymem(sample - nsamples, channel) = value;
            }
          }
        }
      }

      /// Get the delay in samples.
      int get_delay() const { return (int)delaymem.rows(); }

     private:
      Eigen::MatrixXf delaymem;
    };

    DelayBuffer::DelayBuffer(size_t nchannels, size_t nsamples)
        : impl(boost::make_unique<DelayBufferImpl>(nchannels, nsamples)) {}

    void DelayBuffer::process(size_t nsamples, const float *const *input,
                              float *const *output) {
      impl->process(nsamples, input, output);
    }

    int DelayBuffer::get_delay() const { return impl->get_delay(); }

    DelayBuffer::~DelayBuffer() = default;

  }  // namespace dsp
}  // namespace render
