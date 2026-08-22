#pragma once
#include <boost/optional.hpp>
#include <map>
#include <memory>
#include <regex>
#include "../common/point_source_panner.hpp"
#include "../common/screen_edge_lock.hpp"
#include "render/helpers/output_gains.hpp"
#include "render/layout.hpp"
#include "render/metadata.hpp"
#include "render/warnings.hpp"

namespace render {

  class GainCalculatorDirectSpeakersImpl {
   public:
    GainCalculatorDirectSpeakersImpl(
        const Layout& layout,
        std::map<std::string, std::string> additionalSubstitutions = {});

    void calculate(const DirectSpeakersTypeMetadata& metadata,
                   OutputGains& direct,
                   const WarningCB& warning_cb = default_warning_cb);

    template <typename T>
    void calculate(const DirectSpeakersTypeMetadata& metadata,
                   std::vector<T>& direct,
                   const WarningCB& warning_cb = default_warning_cb) {
      OutputGainsT<T> direct_wrapped(direct);
      calculate(metadata, direct_wrapped, warning_cb);
    }

   private:
    /** @brief 从ADM扬声器标签获取bs.2051扬声器标签。
     *
     * 解析 URNs, 处理LFE信道的替代标记方法。
     */
    std::string _nominalSpeakerLabel(const std::string& label);
    /** @brief 获取最接近给定位置的候选演扬声器的索引。
     *
     * 如果有多个扬声器被视为同样地接近，则无法决定最接近的扬声器。
     *
     * @param position  目标位置参数
     * @param candidates 拟考虑的扬声器子集参数
     * @param tol  定义“最近”的公差的参数.
     *
     * @returns  在距离目标位置最近的扬声器中为扬声器编制索引，
     * 如果无法唯一定义此类扬声器，则为“无”。
     */
    boost::optional<int> _closestChannelIndex(const SpeakerPosition& position,
                                              std::vector<bool> candidates,
                                              double tol);

    /** @brief 确定type_metadata是否为LFE通道，
     * 如果speakerLabel和frequency元素之间存在差异，则发出警告。
     */
    bool _isLfeChannel(const DirectSpeakersTypeMetadata& metadata,
                       const WarningCB& warning_cb);

    SpeakerPosition _applyScreenEdgeLock(SpeakerPosition position);
    boost::optional<int> _findChannelWithinBounds(
        const SpeakerPosition& position, bool isLfe, double tol);
    std::vector<std::pair<int, double>> _findCandidates(
        const PolarSpeakerPosition& position, bool isLfe, double tol);
    std::vector<std::pair<int, double>> _findCandidates(
        const CartesianSpeakerPosition& position, bool isLfe, double tol);

    Layout _layout;
    std::shared_ptr<PointSourcePanner> _pointSourcePanner;
    ScreenEdgeLockHandler _screenEdgeLockHandler;
    int _nChannels;
    std::vector<std::string> _channelNames;
    Eigen::VectorXd _azimuths;
    Eigen::VectorXd _elevations;
    Eigen::VectorXd _distances;
    Eigen::MatrixXd _positions;
    Eigen::Array<bool, Eigen::Dynamic, 1> _isLfe;
    std::map<std::string, std::string> _substitutions;
    const std::regex SPEAKER_URN_REGEX =
        std::regex("^urn:itu:bs:2051:[0-9]+:speaker:(.*)$");
  };

}  // namespace render
