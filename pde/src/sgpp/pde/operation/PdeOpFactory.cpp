/******************************************************************************
* Copyright (C) 2009-2014 Technische Universitaet Muenchen                    *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
******************************************************************************/
// @author Valeriy Khakhutskyy (khakhutv@in.tum.de), Dirk Pflueger (pflueged@in.tum.de)

#include <sgpp/pde/operation/PdeOpFactory.hpp>

#include <cstring>

#include <sgpp/base/exception/factory_exception.hpp>

#include <sgpp/base/grid/type/PrewaveletGrid.hpp>

#include <sgpp/pde/basis/linear/noboundary/operation/OperationLaplaceLinear.hpp>
#include <sgpp/pde/basis/linear/boundary/operation/OperationLaplaceLinearBoundary.hpp>
#include <sgpp/pde/basis/modlinear/operation/OperationLaplaceModLinear.hpp>
#include <sgpp/pde/basis/prewavelet/operation/OperationLaplacePrewavelet.hpp>
#include <sgpp/pde/basis/linearstretched/noboundary/operation/OperationLaplaceLinearStretched.hpp>
#include <sgpp/pde/basis/linearstretched/boundary/operation/OperationLaplaceLinearStretchedBoundary.hpp>

#include <sgpp/pde/basis/linear/noboundary/operation/OperationLTwoDotProductLinear.hpp>
#include <sgpp/pde/basis/linear/noboundary/operation/OperationMatrixLTwoDotExplicitLinear.hpp>
#include <sgpp/pde/basis/linear/boundary/operation/OperationLTwoDotProductLinearBoundary.hpp>
#include <sgpp/pde/basis/linear/boundary/operation/OperationMatrixLTwoDotExplicitLinearBoundary.hpp>
#include <sgpp/pde/basis/linearstretched/noboundary/operation/OperationLTwoDotProductLinearStretched.hpp>
#include <sgpp/pde/basis/linearstretched/boundary/operation/OperationLTwoDotProductLinearStretchedBoundary.hpp>
#include <sgpp/pde/basis/periodic/operation/OperationMatrixLTwoDotExplicitPeriodic.hpp>
#include <sgpp/pde/basis/periodic/operation/OperationMatrixLTwoDotPeriodic.hpp>

namespace sg {

  namespace op_factory {

    base::OperationMatrix* createOperationLaplace(base::Grid& grid) {

      if (strcmp(grid.getType(), "linear") == 0) {
        return new pde::OperationLaplaceLinear(grid.getStorage());
      } else if (strcmp(grid.getType(), "linearBoundary") == 0 || strcmp(grid.getType(), "linearTrapezoidBoundary") == 0) {
        return new pde::OperationLaplaceLinearBoundary(grid.getStorage());
      } else if (strcmp(grid.getType(), "modlinear") == 0 ) {
        return new pde::OperationLaplaceModLinear(grid.getStorage());
      } else if (strcmp(grid.getType(), "prewavelet") == 0 ) {
        return new pde::OperationLaplacePrewavelet(grid.getStorage(),
               ((base::PrewaveletGrid*) &grid)->getShadowStorage());
      } else if (strcmp(grid.getType(), "linearStretched") == 0 ) {
        return new pde::OperationLaplaceLinearStretched(grid.getStorage());
      } else if (strcmp(grid.getType(), "linearStretchedTrapezoidBoundary") == 0 ) {
        return new pde::OperationLaplaceLinearStretchedBoundary(grid.getStorage());
      } else {
        throw base::factory_exception("OperationLaplace is not implemented for this grid type.");
      }
    }

    base::OperationMatrix* createOperationLaplace(base::Grid& grid, sg::base::DataVector& coef) {

      if (strcmp(grid.getType(), "linear") == 0) {
        return new pde::OperationLaplaceLinear(grid.getStorage(), coef);
      } else if (strcmp(grid.getType(), "linearBoundary") == 0 || strcmp(grid.getType(), "linearTrapezoidBoundary") == 0) {
        return new pde::OperationLaplaceLinearBoundary(grid.getStorage(), coef);
      } else {
        throw base::factory_exception("OperationLaplace (with coefficients) is not implemented for this grid type.");
      }
    }

    base::OperationMatrix* createOperationLTwoDotProduct(base::Grid& grid) {

      if (strcmp(grid.getType(), "linear") == 0) {
        return new pde::OperationLTwoDotProductLinear(grid.getStorage());
      } else if (strcmp(grid.getType(), "linearBoundary") == 0 || strcmp(grid.getType(), "linearTrapezoidBoundary") == 0) {
        return new pde::OperationLTwoDotProductLinearBoundary(grid.getStorage());
      } else if (strcmp(grid.getType(), "linearStretched") == 0) {
        return new pde::OperationLTwoDotProductLinearStretched(grid.getStorage());
      } else if (strcmp(grid.getType(), "linearStretchedTrapezoidBoundary") == 0) {
        return new pde::OperationLTwoDotProductLinearStretchedBoundary(grid.getStorage());
      } else if (strcmp(grid.getType(), "periodic") == 0) {
        return new pde::OperationMatrixLTwoDotPeriodic(grid.getStorage());
      } else
        throw base::factory_exception("OperationLTwoDotProduct is not implemented for this grid type.");
    }

    base::OperationMatrix* createOperationLTwoDotExplicit(base::Grid& grid) {
      if (strcmp(grid.getType(), "linear") == 0) {
        return new pde::OperationMatrixLTwoDotExplicitLinear(&grid);
      } else if (strcmp(grid.getType(), "linearBoundary") == 0) {
        return new pde::OperationMatrixLTwoDotExplicitLinearBoundary(&grid);
      } else if (strcmp(grid.getType(), "periodic") == 0) {
        return new pde::OperationMatrixLTwoDotExplicitPeriodic(&grid);
      } else
        throw base::factory_exception("OperationLTwoDotExplicit is not implemented for this grid type.");
    }

    base::OperationMatrix* createOperationLTwoDotExplicit(base::DataMatrix* m, base::Grid& grid) {
      if (strcmp(grid.getType(), "linear") == 0) {
        return new pde::OperationMatrixLTwoDotExplicitLinear(m, &grid);
      } else if (strcmp(grid.getType(), "linearBoundary") == 0) {
        return new pde::OperationMatrixLTwoDotExplicitLinearBoundary(m, &grid);
      } else if (strcmp(grid.getType(), "periodic") == 0) {
    	return new pde::OperationMatrixLTwoDotExplicitPeriodic(m, &grid);
      } else
        throw base::factory_exception("OperationLTwoDotExplicit is not implemented for this grid type.");
    }

  }
}
