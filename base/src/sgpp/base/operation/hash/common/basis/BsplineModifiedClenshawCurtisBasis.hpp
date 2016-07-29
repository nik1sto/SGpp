// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#ifndef BSPLINE_MODIFIED_CLENSHAW_CURTIS_BASE_HPP
#define BSPLINE_MODIFIED_CLENSHAW_CURTIS_BASE_HPP

#include <sgpp/base/operation/hash/common/basis/Basis.hpp>
#include <sgpp/base/operation/hash/common/basis/BsplineBasis.hpp>
#include <sgpp/base/tools/ClenshawCurtisTable.hpp>
#include <sgpp/base/tools/GaussLegendreQuadRule1D.hpp>
#include <sgpp/base/exception/operation_exception.hpp>

#include <sgpp/globaldef.hpp>

#include <cmath>
#include <vector>

namespace sgpp {
namespace base {

/**
 * B-spline basis on Clenshaw-Curtis grids.
 */
template<class LT, class IT>
class BsplineModifiedClenshawCurtisBasis : public Basis<LT, IT> {
public:
  /**
   * Default constructor.
   */
  BsplineModifiedClenshawCurtisBasis()
    : BsplineModifiedClenshawCurtisBasis(0) {
  }

  /**
   * Constructor.
   *
   * @param degree        B-spline degree, must be odd
   *                      (if it's even, degree - 1 is used)
   */
  explicit BsplineModifiedClenshawCurtisBasis(size_t degree)
    : degree(degree),
      xi(std::vector<double>(degree + 2, 0.0)),
      clenshawCurtisTable(ClenshawCurtisTable::getInstance()) {
    if (degree < 1) {
      this->degree = 1;
    } else if (degree % 2 == 0) {
      this->degree = degree - 1;
    }
  }

  /**
   * Destructor.
   */
  ~BsplineModifiedClenshawCurtisBasis() override {
  }

  /**
   * @param l     level of the grid point
   * @param i     index of the grid point
   * @return      i-th Clenshaw-Curtis grid point with level l
   */
  inline double clenshawCurtisPoint(LT l, IT i) const {
    const IT hInv = static_cast<IT>(1) << l;
    return clenshawCurtisPoint(l, i, hInv);
  }

  /**
   * @param l     level of the grid point
   * @param ni    neagtive index -i of the grid point
   * @return      (-ni)-th Clenshaw-Curtis grid point with level l
   */
  inline double clenshawCurtisPointNegativeIndex(LT l, IT ni) const {
    const IT hInv = static_cast<IT>(1) << l;
    return clenshawCurtisPointNegativeIndex(l, ni, hInv);
  }

  /**
   * @param l     level of basis function
   * @param i     index of basis function
   * @param x     evaluation point
   * @return      value of modified Clenshaw-Curtis
   *              B-spline basis function
   */
  inline double eval(LT l, IT i, double x) override {
    if (l == 1) {
      return 1.0;
    }

    const IT hInv = static_cast<IT>(1) << l;

    if (i == 1) {
      return modifiedBSpline(l, hInv, x, degree);
    } else if (i == hInv - 1) {
      return modifiedBSpline(l, hInv, 1.0 - x, degree);
    } else {
      constructKnots(l, i, hInv);
      return nonUniformBSpline(x, degree, 0);
    }
  }

  /**
   * @param l     level of basis function
   * @param i     index of basis function
   * @param x     evaluation point
   * @return      value of derivative of modified Clenshaw-Curtis
   *              B-spline basis function
   */
  inline double evalDx(LT l, IT i, double x) {
    if (l == 1) {
      return 0.0;
    }

    const IT hInv = static_cast<IT>(1) << l;

    if (i == 1) {
      return modifiedBSplineDx(l, hInv, x, degree);
    } else if (i == hInv - 1) {
      return -modifiedBSplineDx(l, hInv, 1.0 - x, degree);
    } else {
      constructKnots(l, i, hInv);
      return nonUniformBSplineDx(x, degree, 0);
    }
  }

  /**
   * @param l     level of basis function
   * @param i     index of basis function
   * @param x     evaluation point
   * @return      value of 2nd derivative of modified Clenshaw-Curtis
   *              B-spline basis function
   */
  inline double evalDxDx(LT l, IT i, double x) {
    if (l == 1) {
      return 0.0;
    }

    const IT hInv = static_cast<IT>(1) << l;

    if (i == 1) {
      return modifiedBSplineDxDx(l, hInv, x, degree);
    } else if (i == hInv - 1) {
      return modifiedBSplineDxDx(l, hInv, 1.0 - x, degree);
    } else {
      constructKnots(l, i, hInv);
      return nonUniformBSplineDxDx(x, degree, 0);
    }
  }

  /**
   * @return      B-spline degree
   */
  inline size_t getDegree() const {
    return degree;
  }

    /**
   * @param l     level of basis function
   * @param i     index of basis function
   * @return      integreal of the basis function
   */
  double getIntegral(LT l, IT i) {
    const IT hInv = static_cast<IT>(1) << l;
    const double hInvDbl = static_cast<double>(hInv);
    size_t degree = getDegree();
    if(l == 1){
      return 1.0;
    }
    size_t erster_abschnitt = std::max(0, -static_cast<int>(i-(degree+1)/2));
    size_t letzter_abschnitt = std::min(degree, (1 << l) + (degree+1)/2 - i - 1 );
    // size_t quadLevel = (degree + 1)/2;
    size_t quadLevel = 5;
    if(!integrationInitialized){
      sgpp::base::GaussLegendreQuadRule1D gauss;
      gauss.getLevelPointsAndWeightsNormalized(quadLevel, coordinates, weights);
      integrationInitialized = true;
    }
    if ( !(i == i || i== hInv - 1) ) {
      constructKnots(l, i, hInv);
    }
    double res = 0.0;
    for(size_t j = erster_abschnitt; j <= letzter_abschnitt; j++){
      double h = xi[j + 1] - xi[j];
      double temp_res = 0.0;
      for (size_t c = 0; c < quadLevel; c++){
        double x = (h * coordinates[c]) + xi[j];
        if (i == 1) {
          temp_res += weights[c]*modifiedBSpline(l, hInv, x, degree);
        } else if (i == hInv - 1) {
          temp_res += weights[c]*modifiedBSpline(l, hInv, 1-x, degree);
        } else {
          temp_res += weights[c]*nonUniformBSpline(x, degree, 0);
        }
        // temp_res += weights[c]*eval(l, i, x);
      }
      res += h * temp_res;
    }
    return res;
  }
  
 protected:
  /// degree of the B-spline
  size_t degree;
  /// temporary helper vector of fixed size p+2 containing B-spline knots
  std::vector<double> xi;
  /// reference to the Clenshaw-Curtis cache table
  ClenshawCurtisTable& clenshawCurtisTable;
  DataVector coordinates;
  DataVector weights;
  bool integrationInitialized = false;

  /**
   * @param l     level of the grid point
   * @param i     index of the grid point
   * @param hInv  2^l
   * @return      i-th Clenshaw-Curtis grid point with level l
   */
  inline double clenshawCurtisPoint(LT l, IT i, IT hInv) const {
    if (l == 1) {
      return static_cast<double>(i) / 2.0;
    } else if (i == 0) {
      // = x_{l,1} - (x_{l,2} - x_{l,1})
      return 2.0 * clenshawCurtisTable.getPoint(l, 1, hInv) -
             clenshawCurtisTable.getPoint(l, 2, hInv);
    } else if (i >= hInv) {
      const double x1 = clenshawCurtisTable.getPoint(l, 1, hInv);
      const double x2 = clenshawCurtisTable.getPoint(l, 2, hInv);
      const double hBoundary = x2 - x1;

      return (1.0 - x1) + hBoundary * static_cast<double>(i - hInv + 1);
    } else {
      return clenshawCurtisTable.getPoint(l, i, hInv);
    }
  }

  /**
   * @param l     level of the grid point
   * @param ni    negative index -i of the grid point
   * @param hInv  2^l
   * @return      (-ni)-th Clenshaw-Curtis grid point with level l
   */
  inline double clenshawCurtisPointNegativeIndex(LT l, IT ni, IT hInv) const {
    const double x1 = clenshawCurtisTable.getPoint(l, 1, hInv);
    const double x2 = clenshawCurtisTable.getPoint(l, 2, hInv);
    const double hBoundary = x2 - x1;

    return x1 - hBoundary * static_cast<double>(ni + 1);
  }

  /**
   * Construct the (p+2) Clenshaw-Curtis knots of a
   * B-spline basis function and save them in xi.
   *
   * @param l     level of basis function
   * @param i     index of basis function
   * @param hInv  2^l
   */
  inline void constructKnots(LT l, IT i, IT hInv) {
    const IT degreePlusOneHalved = static_cast<IT>(degree + 1) / 2;

    for (IT k = 0; k < degree + 2; k++) {
      if (i + k >= degreePlusOneHalved) {
        xi[k] = clenshawCurtisPoint(
                  l, i + k - degreePlusOneHalved, hInv);
      } else {
        xi[k] = clenshawCurtisPointNegativeIndex(
                  l, degreePlusOneHalved - i - k, hInv);
      }
    }
  }

  /**
   * Construct the (p+2) Clenshaw-Curtis knots of a
   * B-spline basis function and save them in xi.
   *
   * @param l     level of basis function
   * @param ni    negative index -i of basis function
   * @param hInv  2^l
   */
  inline void constructKnotsNegativeIndex(LT l, IT ni, IT hInv) {
    const IT degreePlusOneHalved = static_cast<IT>(degree + 1) / 2;

    for (IT k = 0; k < degree + 2; k++) {
      if (k >= degreePlusOneHalved + ni) {
        xi[k] = clenshawCurtisPoint(l, k - ni - degreePlusOneHalved, hInv);
      } else {
        xi[k] = clenshawCurtisPointNegativeIndex(l,
                degreePlusOneHalved + ni - k,
                hInv);
      }
    }
  }

  /**
   * @param x     evaluation point
   * @param p     B-spline degree
   * @param k     index of B-spline in the knot sequence
   * @return      value of non-uniform B-spline with knots
   *              \f$\{\xi_k, ... \xi_{k+p+1}\}\f$
   */
  inline double nonUniformBSpline(double x, size_t p, size_t k) const {
    if (p == 0) {
      // characteristic function of [xi[k], xi[k+1])
      return (((x >= xi[k]) && (x < xi[k + 1])) ? 1.0 : 0.0);
    } else if ((x < xi[k]) || (x >= xi[k + p + 1])) {
      // out of support
      return 0.0;
    } else {
      // Cox-de-Boor recursion
      return (x - xi[k]) / (xi[k + p] - xi[k])
             * nonUniformBSpline(x, p - 1, k)
             + (1.0 - (x - xi[k + 1]) / (xi[k + p + 1] - xi[k + 1]))
             * nonUniformBSpline(x, p - 1, k + 1);
    }
  }

  /**
   * @param x     evaluation point
   * @param p     B-spline degree
   * @param k     index of B-spline in the knot sequence
   * @return      value of derivative of non-uniform B-spline with knots
   *              \f$\{\xi_k, ... \xi_{k+p+1}\}\f$
   */
  inline double nonUniformBSplineDx(double x, size_t p, size_t k) const {if (p == 0) {return 0.0;
    } else if ((x < xi[k]) || (x >= xi[k + p + 1])) {
      return 0.0;
    } else {
      const double pDbl = static_cast<double>(p);

      return pDbl / (xi[k + p] - xi[k]) * nonUniformBSpline(x, p - 1, k)
             - pDbl / (xi[k + p + 1] - xi[k + 1])
             * nonUniformBSpline(x, p - 1, k + 1);
    }
  }

  /**
   * @param x     evaluation point
   * @param p     B-spline degree
   * @param k     index of B-spline in the knot sequence
   * @return      value of 2nd derivative of non-uniform B-spline
   *              with knots \f$\{\xi_k, ... \xi_{k+p+1}\}\f$
   */
  inline double nonUniformBSplineDxDx(
    double x, size_t p, size_t k) const {
    if (p <= 1) {
      return 0.0;
    } else if ((x < xi[k]) || (x >= xi[k + p + 1])) {
      return 0.0;
    } else {
      const double pDbl = static_cast<double>(p);
      const double alphaKP = pDbl / (xi[k + p] - xi[k]);
      const double alphaKp1P = pDbl / (xi[k + p + 1] - xi[k + 1]);
      const double alphaKPm1 = (pDbl - 1.0) / (xi[k + p - 1] - xi[k]);
      const double alphaKp1Pm1 = (pDbl - 1.0) / (xi[k + p] - xi[k + 1]);
      const double alphaKp2Pm1 = (pDbl - 1.0) /
                                  (xi[k + p + 1] - xi[k + 2]);

      return alphaKP * alphaKPm1 * nonUniformBSpline(x, p - 2, k)
             - (alphaKP + alphaKp1P) * alphaKp1Pm1
             * nonUniformBSpline(x, p - 2, k + 1)
             + alphaKp1P * alphaKp2Pm1 *
             nonUniformBSpline(x, p - 2, k + 2);
    }
  }

  /**
   * @param l     level of basis function
   * @param hInv  2^l
   * @param x     evaluation point
   * @param p     B-spline degree
   * @return      value of modified
   *              Clenshaw-Curtis B-spline (e.g. index == 1)
   */
  inline double modifiedBSpline(LT l, IT hInv, double x, size_t p) {
    double y = 0.0;
    constructKnots(l, 1, hInv);
    y += 1.0 * nonUniformBSpline(x, degree, 0);
    constructKnots(l, 0, hInv);
    y += 2.0 * nonUniformBSpline(x, degree, 0);

    // the upper summation bound is defined to be ceil((p + 1) / 2.0),
    // which is the same as (p + 2) / 2 written in C
    for (IT k = 2; k <= (p + 2) / 2; k++) {
      constructKnotsNegativeIndex(l, k - 1, hInv);
      y += static_cast<double>(k + 1) * nonUniformBSpline(x, degree, 0);
    }

    return y;
  }

  /**
   * @param l     level of basis function
   * @param hInv  2^l
   * @param x     evaluation point
   * @param p     B-spline degree
   * @return      value of derivative of modified
   *              Clenshaw-Curtis B-spline (e.g. index == 1)
   */
  inline double modifiedBSplineDx(LT l, IT hInv, double x, size_t p) {
    double y = 0.0;
    constructKnots(l, 1, hInv);
    y += 1.0 * nonUniformBSplineDx(x, degree, 0);
    constructKnots(l, 0, hInv);
    y += 2.0 * nonUniformBSplineDx(x, degree, 0);

    // the upper summation bound is defined to be ceil((p + 1) / 2.0),
    // which is the same as (p + 2) / 2 written in C
    for (IT k = 2; k <= (p + 2) / 2; k++) {
      constructKnotsNegativeIndex(l, k - 1, hInv);
      y += static_cast<double>(k + 1) * nonUniformBSplineDx(x, degree, 0);
    }

    return y;
  }

  /**
   * @param l     level of basis function
   * @param hInv  2^l
   * @param x     evaluation point
   * @param p     B-spline degree
   * @return      value of 2nd derivative of modified
   *              Clenshaw-Curtis B-spline (e.g. index == 1)
   */
  inline double modifiedBSplineDxDx(LT l, IT hInv, double x, size_t p) {
    double y = 0.0;
    constructKnots(l, 1, hInv);
    y += 1.0 * nonUniformBSplineDxDx(x, degree, 0);
    constructKnots(l, 0, hInv);
    y += 2.0 * nonUniformBSplineDxDx(x, degree, 0);

    // the upper summation bound is defined to be ceil((p + 1) / 2.0),
    // which is the same as (p + 2) / 2 written in C
    for (IT k = 2; k <= (p + 2) / 2; k++) {
      constructKnotsNegativeIndex(l, k - 1, hInv);
      y += static_cast<double>(k + 1) * nonUniformBSplineDxDx(x, degree, 0);
    }

    return y;
  }

};

// default type-def (unsigned int for level and index)
typedef BsplineModifiedClenshawCurtisBasis<unsigned int, unsigned int>
SBsplineModifiedClenshawCurtisBase;

}  // namespace base
}  // namespace sgpp

#endif /* BSPLINE_MODIFIED_CLENSHAW_CURTIS_BASE_HPP */
