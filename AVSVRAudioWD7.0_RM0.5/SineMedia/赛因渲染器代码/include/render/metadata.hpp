#pragma once
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <string>
#include "common_types.hpp"
#include "export.hpp"
#include "screen.hpp"

namespace render {

  // typeDefinition==DirectSpeakers

  struct RENDER_EXPORT ScreenEdgeLock {
    /// screenEdgeLock属性位于带有coordinate="azimuth"
    ///  或coordinate="X"的position元素之上
    boost::optional<std::string> horizontal;
    /// screenEdgeLock属性位于带有coordinate="elevation"
    /// 或coordinate="Z"）的position元素之上
    boost::optional<std::string> vertical;
  };

  struct RENDER_EXPORT PolarSpeakerPosition {
    PolarSpeakerPosition(double az = 0.0, double el = 0.0, double dist = 1.0)
        : azimuth(az), elevation(el), distance(dist){};
    double azimuth;
    boost::optional<double> azimuthMin;
    boost::optional<double> azimuthMax;
    double elevation;
    boost::optional<double> elevationMin;
    boost::optional<double> elevationMax;
    double distance;
    boost::optional<double> distanceMin;
    boost::optional<double> distanceMax;
    ScreenEdgeLock screenEdgeLock;
  };

  struct RENDER_EXPORT CartesianSpeakerPosition {
    CartesianSpeakerPosition(double X = 0.0, double Y = 1.0, double Z = 0.0)
        : X(X), Y(Y), Z(Z){};
    double X;
    boost::optional<double> XMin;
    boost::optional<double> XMax;
    double Y;
    boost::optional<double> YMin;
    boost::optional<double> YMax;
    double Z;
    boost::optional<double> ZMin;
    boost::optional<double> ZMax;
    ScreenEdgeLock screenEdgeLock;
  };

  using SpeakerPosition =
      boost::variant<PolarSpeakerPosition, CartesianSpeakerPosition>;

  struct RENDER_EXPORT ChannelFrequency {
    boost::optional<double> lowPass = boost::none;
    boost::optional<double> highPass = boost::none;
  };

  struct RENDER_EXPORT DirectSpeakersTypeMetadata {
    /// 描述此audioBlockFormat中的speakerLabel标记的内容，是按AXML中给出的顺序排列
    std::vector<std::string> speakerLabels = {};
    /// 描述位置元素的内容
    SpeakerPosition position = PolarSpeakerPosition();
    /// audioChannelFormat中包含的frequency信息
    ChannelFrequency channelFrequency = {};
    /// 直接引用此通道的audioPackFormat的audioPackFormatID
    /// channel, 例如'AP_00010002'
    boost::optional<std::string> audioPackFormatID = boost::none;
  };

  // typeDefinition==Objects

  struct RENDER_EXPORT ChannelLock {
    ChannelLock(bool flag = false,
                boost::optional<double> maxDistance = boost::none)
        : flag(flag), maxDistance(maxDistance){};
    bool flag;
    boost::optional<double> maxDistance;
  };

  struct RENDER_EXPORT PolarObjectDivergence {
    PolarObjectDivergence(double divergence = 0.0, double azimuthRange = 45.0)
        : divergence(divergence), azimuthRange(azimuthRange){};
    double divergence;
    double azimuthRange;
  };
  struct RENDER_EXPORT CartesianObjectDivergence {
    CartesianObjectDivergence(double divergence = 0.0,
                              double positionRange = 0.0)
        : divergence(divergence), positionRange(positionRange){};
    double divergence;
    double positionRange;
  };

  using ObjectDivergence =
      boost::variant<PolarObjectDivergence, CartesianObjectDivergence>;

  struct RENDER_EXPORT PolarExclusionZone {
    float minAzimuth;
    float maxAzimuth;
    float minElevation;
    float maxElevation;
    float minDistance;
    float maxDistance;
    std::string label;
  };

  struct RENDER_EXPORT CartesianExclusionZone {
    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;
    std::string label;
  };

  using ExclusionZone =
      boost::variant<PolarExclusionZone, CartesianExclusionZone>;

  struct RENDER_EXPORT ZoneExclusion {
    std::vector<ExclusionZone> zones;
  };

  struct RENDER_EXPORT ObjectsTypeMetadata {
    Position position = {};
    double width = 0.0;
    double height = 0.0;
    double depth = 0.0;
    /// “笛卡尔”标识的值；与position、objectDivergence
    /// 和zoneExclusion中使用的类型相同，否则将出现警告。
   
    bool cartesian = false;
    double gain = 1.0;
    double diffuse = 0.0;
    ChannelLock channelLock = {};
    ObjectDivergence objectDivergence = {};
    ZoneExclusion zoneExclusion = {};
    bool screenRef = false;
    /// 正在渲染的“AudioProgram”的“AudioProgrammerReferenceScreen”元素的屏幕规格
    
    Screen referenceScreen = getDefaultScreen();
  };

  // typeDefinition==HOA

  /// 在HOA audioPackFormat中表示所有AudioChannelFormats。
  ///
  /// \ref orders和\ref degrees必须与正在渲染的通道具有相同的长度和顺序，\ref orders and \ref degrees must be the same length and must be in the
  /// 以便第i个输入通道具有orders[i]`和degrees[i]。
  ///
  
  struct RENDER_EXPORT HOATypeMetadata {
    /// 每个通道的“audioBlockFormat”元素中的“order”元素的值
    
    std::vector<int> orders;
    /// 每个频道的“audioBlockFormat”元素中的“degree”元素的值
    
    std::vector<int> degrees;
    std::string normalization = std::string("SN3D");
    double nfcRefDist = 0.0;
    bool screenRef = false;
    /// 正在渲染的“AudioProgram”的“AudioProgrammerReferenceScreen”元素的屏幕规格
    Screen referenceScreen = getDefaultScreen();
  };

}  // namespace render
