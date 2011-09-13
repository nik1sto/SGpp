/******************************************************************************
* Copyright (C) 2009 Technische Universitaet Muenchen                         *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
******************************************************************************/
// @author Alexander Heinecke (Alexander.Heinecke@mytum.de)

#ifndef OPERATIONTESTLINEARSTRETCHEDBOUNDARY_HPP
#define OPERATIONTESTLINEARSTRETCHEDBOUNDARY_HPP

#include "operation/datadriven/OperationTest.hpp"
#include "grid/GridStorage.hpp"
#include "data/DataVector.hpp"
#include "data/DataMatrix.hpp"

namespace sg
{
namespace datadriven
{

  /**
   * This class implements OperationTest for a grids with linear basis ansatzfunctions with
   * boundaries
   *
   * @version $HEAD$
   */
  class OperationTestLinearStretchedBoundary : public OperationTest
  {
  public:
    /**
     * Constructor
     *
     * @param storage the grid's base::GridStorage object
     */
    OperationTestLinearStretchedBoundary(base::GridStorage* storage) : storage(storage) {}

    /**
     * Destructor
     */
    virtual ~OperationTestLinearStretchedBoundary() {}

    virtual double test(base::DataVector& alpha, base::DataMatrix& data, base::DataVector& classes);
    virtual double testMSE(base::DataVector& alpha, base::DataMatrix& data, base::DataVector& refValues);
    virtual double testWithCharacteristicNumber(base::DataVector& alpha, base::DataMatrix& data, base::DataVector& classes, base::DataVector& charaNumbers);

  protected:
    /// Pointer to base::GridStorage object
    base::GridStorage* storage;
  };

}
}

#endif /* OPERATIONTESTLINEARSTRETCHEDBOUNDARY_HPP */
