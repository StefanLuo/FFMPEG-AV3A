#include "hoa.hpp"
#include <iostream>
#include "render/helpers/assert.hpp"
#include "resources.hpp"


Eigen::Matrix<double, Eigen::Dynamic, 3> render::hoa::load_points() {
  std::stringstream points_file;

  render_assert(getEmbeddedFile("Quadrature.dat", points_file),
             "could not find embedded file Quadrature.dat, this is "
             "probably a compilation error");
  size_t len = 5200;

  Eigen::Matrix<double, Eigen::Dynamic, 3> points(len, 3);
  for (size_t i = 0; i < len; i++) {
    double phi, theta;
    points_file >> phi >> theta;
    points(i, 0) = std::sin(theta) * std::cos(phi);
    points(i, 1) = std::sin(theta) * std::sin(phi);
    points(i, 2) = std::cos(theta);
  }

  render_assert(points_file.good(),
             "failure when reading HOA points from embedded file");

  return points;
}
