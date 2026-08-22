#pragma once
#include <boost/optional.hpp>
#include <string>
#include <vector>
#include "common_types.hpp"
#include "export.hpp"
#include "screen.hpp"

namespace render {

  /** @brief 用名称、实际和标称位置、允许的方位角和仰角范围以及lfe标志表示通道。
   */
  class RENDER_EXPORT Channel {
   public:
    Channel() = default;
    /**
     * @param name 通道命名
     * @param polarPosition  真实扬声器位置参数（球形坐标）
     * @param polarPositionNominal 标称扬声器位置参数，默认为球形坐标位置
     * @param azimuthRange
     *     方位角范围（度）参数；允许范围解释为从方位角[0]开始，
     *     逆时针移动到方位角[1]；默认为polar_nominal_position的方位角
     * @param elevationRange
     *     仰角范围（度）参数；允许的范围被解释为从（elevationRange.first）开始，
     *     向上移动到（elevationRange.second）默认为polar_nominal_position的仰角。
     * @param isLfe 指示LFE通道的标志参数
     */
    Channel(
        const std::string& name, PolarPosition polarPosition,
        boost::optional<PolarPosition> polarPositionNominal = boost::none,
        boost::optional<std::pair<double, double>> azimuthRange = boost::none,
        boost::optional<std::pair<double, double>> elevationRange = boost::none,
        bool isLfe = false);

    std::string name() const;
    PolarPosition polarPosition() const;
    PolarPosition polarPositionNominal() const;
    std::pair<double, double> azimuthRange() const;
    std::pair<double, double> elevationRange() const;
    bool isLfe() const;

    void name(const std::string& name);
    void polarPosition(PolarPosition polarPosition);
    void polarPositionNominal(
        const boost::optional<PolarPosition>& polarPositionNominal);
    void azimuthRange(
        const boost::optional<std::pair<double, double>>& azimuthRange);
    void elevationRange(
        const boost::optional<std::pair<double, double>>& elevationRange);
    void isLfe(bool isLfe);

    void checkPosition(std::function<void(const std::string&)> callback) const;

   private:
    std::string _name;
    PolarPosition _polarPosition;
    boost::optional<PolarPosition> _polarPositionNominal;
    boost::optional<std::pair<double, double>> _azimuthRange;
    boost::optional<std::pair<double, double>> _elevationRange;
    bool _isLfe;
  };

  /** @brief 扬声器布局的表示，带有名称和通道列表。
   */
  class RENDER_EXPORT Layout {
   public:
    Layout(std::string name = "",
           std::vector<Channel> channels = std::vector<Channel>(),
           boost::optional<Screen> screen = boost::none);

    std::string name() const;
    std::vector<Channel>& channels();
    std::vector<Channel> channels() const;
    boost::optional<Screen> screen() const;

    void name(std::string name);
    void screen(boost::optional<Screen> screen);
    Layout withoutLfe() const;
    std::vector<bool> isLfe() const;
    std::vector<std::string> channelNames() const;
    void checkPositions(std::function<void(const std::string&)> callback) const;
    Channel channelWithName(const std::string& name) const;
    boost::optional<int> indexForName(const std::string& name) const;
    std::vector<PolarPosition> positions() const;
    std::vector<PolarPosition> nominalPositions() const;

   private:
    std::string _name;
    std::vector<Channel> _channels;
    boost::optional<Screen> _screen;
  };

}  // namespace render
