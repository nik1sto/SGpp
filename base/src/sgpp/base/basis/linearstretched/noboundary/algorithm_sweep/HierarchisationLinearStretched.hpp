/* ****************************************************************************
* Copyright (C) 2011 Technische Universitaet Muenchen                         *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
**************************************************************************** */
// @author Alexander Heinecke (Alexander.Heinecke@mytum.de), Sarpkan Selcuk (Sarpkan.Selcuk@mytum.de)


#ifndef HIERARCHISATIONLINEARSTRETCHED_HPP
#define HIERARCHISATIONLINEARSTRETCHED_HPP

#include <sgpp/base/grid/GridStorage.hpp>
#include <sgpp/base/datatypes/DataVector.hpp>

namespace sg {
  namespace base {



    /**
     * Class that implements the hierarchisation on a linear sparse grid. Therefore
     * the ()operator has to be implement in order to use the sweep algorithm for
     * the grid traversal
     */
    class HierarchisationLinearStretched {
      protected:
        typedef GridStorage::grid_iterator grid_iterator;

        /// the grid object
        GridStorage* storage;
        /// the stretching object
        Stretching* stretch;

      public:
        /**
         * Constructor, must be bind to a grid
         *
         * @param storage the grid storage object of the the grid, on which the hierarchisation should be executed
         */
        HierarchisationLinearStretched(GridStorage* storage);

        /**
         * Destructor
         */
        virtual ~HierarchisationLinearStretched();

        /**
         * Implements operator() needed by the sweep class during the grid traversal. This function
         * is applied to the whole grid.
         *
         * @param source this DataVector holds the node base coefficients of the function that should be applied to the sparse grid
         * @param result this DataVector holds the linear base coefficients of the sparse grid's ansatz-functions
         * @param index a iterator object of the grid
         * @param dim current fixed dimension of the 'execution direction'
         */
        virtual void operator()(DataVector& source, DataVector& result, grid_iterator& index, size_t dim);

      protected:

        /**
         * Recursive hierarchisation algorithm, this algorithms works in-place -> source should be equal to result
         *
         * @todo add graphical explanation here
         *
         * @param source this DataVector holds the node base coefficients of the function that should be applied to the sparse grid
         * @param result this DataVector holds the linear base coefficients of the sparse grid's ansatz-functions
         * @param index a iterator object of the grid
         * @param dim current fixed dimension of the 'execution direction'
         * @param fl left value of the current region regarded in this step of the recursion
         * @param fr right value of the current region regarded in this step of the recursion
         */
        void rec(DataVector& source, DataVector& result, grid_iterator& index, size_t dim, double fl, double fr);
    };

    // namespace detail

  } // namespace sg
}

#endif /* HIERARCHISATIONLINEARSTRETCHED_HPP */