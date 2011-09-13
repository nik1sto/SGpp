/******************************************************************************
* Copyright (C) 2009 Technische Universitaet Muenchen                         *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
******************************************************************************/
// @author Dirk Pflueger (pflueged@in.tum.de)

#include "basis/modbspline/operation/datadriven/OperationTestModBspline.hpp"

#include "exception/operation_exception.hpp"
#include "algorithm/datadriven/test_dataset.hpp"

#include "data/DataVector.hpp"

namespace sg
{
namespace datadriven
{

  double OperationTestModBspline::test(sg::base::DataVector& alpha, sg::base::DataMatrix& data, sg::base::DataVector& classes)
  {
    return test_dataset(this->storage, base, alpha, data, classes);
  }

  double OperationTestModBspline::testMSE(sg::base::DataVector& alpha, sg::base::DataMatrix& data, sg::base::DataVector& refValues)
  {
    return test_dataset_mse(this->storage, base, alpha, data, refValues);
  }

  double OperationTestModBspline::testWithCharacteristicNumber(sg::base::DataVector& alpha, sg::base::DataMatrix& data, sg::base::DataVector& classes, sg::base::DataVector& charaNumbers)
  {
    return test_datasetWithCharacteristicNumber(this->storage, base, alpha, data, classes, charaNumbers);
  }

}
}
