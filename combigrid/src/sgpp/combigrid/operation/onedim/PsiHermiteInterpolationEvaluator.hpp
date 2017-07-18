// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#ifndef COMBIGRID_SRC_SGPP_COMBIGRID_OPERATION_ONEDIM_PSIHERMITEINTERPOLATIONEVALUATOR_HPP_
#define COMBIGRID_SRC_SGPP_COMBIGRID_OPERATION_ONEDIM_PSIHERMITEINTERPOLATIONEVALUATOR_HPP_

#include <sgpp/combigrid/algebraic/FloatScalarVector.hpp>
#include <sgpp/combigrid/definitions.hpp>
#include <sgpp/combigrid/operation/onedim/AbstractLinearEvaluator.hpp>

#include <vector>

namespace sgpp {
namespace combigrid {

/**
 * This evaluator does linear interpolation on the given grid points. If the evaluation point is
 * outside the grid points, it will just use the function value of the nearest grid point.
 */
class PsiHermiteInterpolationEvaluator : public AbstractLinearEvaluator<FloatScalarVector> {
  double evaluationPoint;
  std::vector<FloatScalarVector> basisCoefficients;
  std::vector<double> xValues;

  void computeBasisCoefficients();

 public:
  PsiHermiteInterpolationEvaluator();
  virtual ~PsiHermiteInterpolationEvaluator();
  PsiHermiteInterpolationEvaluator(PsiHermiteInterpolationEvaluator const &other);

  virtual std::vector<FloatScalarVector> getBasisCoefficients() { return basisCoefficients; }

  virtual void setGridPoints(std::vector<double> const &newXValues);
  virtual std::shared_ptr<AbstractLinearEvaluator<FloatScalarVector>> cloneLinear();
  virtual bool needsOrderedPoints();
  virtual bool needsParameter();
  virtual void setParameter(FloatScalarVector const &param);
  virtual double evalUniform(double x);
};

} /* namespace combigrid */
} /* namespace sgpp*/

#endif /* COMBIGRID_SRC_SGPP_COMBIGRID_OPERATION_ONEDIM_PSIHERMITEINTERPOLATIONEVALUATOR_HPP_ */
