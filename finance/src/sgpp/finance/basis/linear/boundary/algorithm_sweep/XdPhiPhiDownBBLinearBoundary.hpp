// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#ifndef XDPHIPHIDOWNBBLINEARBOUNDARY_HPP
#define XDPHIPHIDOWNBBLINEARBOUNDARY_HPP

#include <sgpp/base/grid/GridStorage.hpp>
#include <sgpp/base/datatypes/DataVector.hpp>

#include <sgpp/finance/basis/linear/noboundary/algorithm_sweep/XdPhiPhiDownBBLinear.hpp>

#include <sgpp/globaldef.hpp>


namespace SGPP {
  namespace finance {



    /**
     * Implementation of sweep operator (): 1D Down for
     * Bilinearform \f$\int_{x} x \frac{\partial \phi(x)}{x} \phi(x) dx\f$
     * on linear boundary grids
     */
    class XdPhiPhiDownBBLinearBoundary : public XdPhiPhiDownBBLinear {
      public:
        /**
         * Constructor
         *
         * @param storage the grid's SGPP::base::GridStorage object
         */
        XdPhiPhiDownBBLinearBoundary(SGPP::base::GridStorage* storage);

        /**
         * Destructor
         */
        virtual ~XdPhiPhiDownBBLinearBoundary();

        /**
         * This operations performs the calculation of down in the direction of dimension <i>dim</i>
         *
         * For level zero it's assumed, that both ansatz-functions do exist: 0,0 and 0,1
         * If one is missing this code might produce some bad errors (segmentation fault, wrong calculation
         * result)
         * So please assure that both functions do exist!
         *
         * On level zero the getfixDirechletBoundaries of the storage object evaluated
         *
         * @param source SGPP::base::DataVector that contains the gridpoint's coefficients (values from the vector of the laplace operation)
         * @param result SGPP::base::DataVector that contains the result of the down operation
         * @param index a iterator object of the grid
         * @param dim current fixed dimension of the 'execution direction'
         */
        virtual void operator()(SGPP::base::DataVector& source, SGPP::base::DataVector& result, grid_iterator& index, size_t dim);
    };

    // namespace detail

  } // namespace SGPP
}

#endif /* XDPHIPHIDOWNBBLINEARBOUNDARY_HPP */