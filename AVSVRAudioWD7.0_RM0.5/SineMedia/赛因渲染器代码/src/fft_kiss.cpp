#include <complex>
#include <cstddef>
#include <memory>
#include <vector>
#include "render/export.hpp"
#include "render/fft.hpp"
#include "render/helpers/assert.hpp"
#include "kissfft/kissfft.hh"

namespace render {

  //   FFT接口KISS的实现方式
  //
  //
  // - kiffsst对象包含计划缓冲区和工作缓冲区，它们必须保存在FFTWorkBuf实现中，
  //   以使其线程安全。这些对象还存储在plan对象中，
  //   以便至少可以将它们复制到FFTWorkBuf中，以节省初始化时间。
  //   
  // - KISS只支持实数到复数实数的转换，而不支持复数到实数的转换，
  //   因此transform_reverse会执行完整的转换，
  //   并来回复制到FFTWorkBuf中存储的临时缓冲区。
  //
  // - KISS transform_real packs the real N/2 component into the imaginary part
  // - KISS transform_real将实数N/2分量打包到第一个分量的虚部——这是解开的包，
  //   可使所有FFT实现都可以使用相同的格式。
  //   
  //
  // - KISS transform_real仅支持偶数输入长度。这是接口的一部分，
  //   这是接口的一部分，是一个合理的做法。

  template <typename Real>
  class WorkBufKiss : public FFTWorkBuf {
   public:
    WorkBufKiss(size_t n_fft, kissfft<Real> plan_forward,
                kissfft<Real> plan_reverse)
        : plan_forward(plan_forward),
          plan_reverse(plan_reverse),
          reverse_tmp_input(n_fft),
          reverse_tmp_output(n_fft) {}

   public:
    kissfft<Real> plan_forward;
    kissfft<Real> plan_reverse;
    std::vector<std::complex<Real>> reverse_tmp_input;
    std::vector<std::complex<Real>> reverse_tmp_output;
  };

  template <typename Real>
  class FFTPlanKiss : public FFTPlan<Real> {
   public:
    using Complex = typename FFTPlan<Real>::Complex;

    FFTPlanKiss(size_t n_fft)
        : n_fft(n_fft),
          plan_forward(n_fft / 2, false),
          plan_reverse(n_fft, true) {}

    void transform_forward(Real *input, Complex *output,
                           FFTWorkBuf &workbuf) const override {
      WorkBufKiss<Real> &workbuf_kiss =
          dynamic_cast<WorkBufKiss<Real> &>(workbuf);
      workbuf_kiss.plan_forward.transform_real(input, output);

      // kiss transform_real将n/2+1实数项压缩为第0个虚项；然后放在适当的地方。
      output[n_fft / 2] = output[0].imag();
      output[0].imag(0.0);
    }

    void transform_reverse(Complex *input, Real *output,
                           FFTWorkBuf &workbuf) const override {
      WorkBufKiss<Real> &workbuf_kiss =
          dynamic_cast<WorkBufKiss<Real> &>(workbuf);

      for (size_t i = 0; i < n_fft; i++)
        workbuf_kiss.reverse_tmp_input[i] =
            i < n_fft / 2 + 1 ? input[i] : std::conj(input[n_fft - i]);

      workbuf_kiss.plan_reverse.transform(
          workbuf_kiss.reverse_tmp_input.data(),
          workbuf_kiss.reverse_tmp_output.data());

      for (size_t i = 0; i < n_fft; i++)
        output[i] = workbuf_kiss.reverse_tmp_output[i].real();
    }

    std::unique_ptr<FFTWorkBuf> alloc_workbuf() const override {
      return std::unique_ptr<FFTWorkBuf>(
          new WorkBufKiss<Real>(n_fft, plan_forward, plan_reverse));
    }

   private:
    size_t n_fft;
    kissfft<Real> plan_forward;
    kissfft<Real> plan_reverse;
  };

  template <typename Real>
  class FFTKiss : public FFTImpl<Real> {
   public:
    std::shared_ptr<FFTPlan<Real>> plan(size_t n_fft) const override {
      render_assert(n_fft % 2 == 0, "n_fft must be even");
      return std::make_shared<FFTPlanKiss<Real>>(n_fft);
    }
  };

  template <typename Real>
  FFTImpl<Real> &get_fft_kiss() {
    static FFTKiss<Real> fft;
    return fft;
  }

  template FFTImpl<float> RENDER_EXPORT &get_fft_kiss<float>();
  template FFTImpl<double> RENDER_EXPORT &get_fft_kiss<double>();

}  // namespace render
