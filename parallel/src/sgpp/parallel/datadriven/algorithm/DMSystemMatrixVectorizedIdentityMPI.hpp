/* ****************************************************************************
* Copyright (C) 2010 Technische Universitaet Muenchen                         *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
**************************************************************************** */
// @author Alexander Heinecke (Alexander.Heinecke@mytum.de)
// @author Roman Karlstetter (karlstetter@mytum.de)

#ifndef DMSYSTEMMATRIXVECTORIZEDIDENTITYMPI_HPP
#define DMSYSTEMMATRIXVECTORIZEDIDENTITYMPI_HPP

#include <sgpp/base/datatypes/DataVector.hpp>
#include <sgpp/base/grid/Grid.hpp>

#include <sgpp/datadriven/algorithm/DMSystemMatrixBase.hpp>

#include <sgpp/parallel/datadriven/operation/OperationMultipleEvalVectorized.hpp>
#include <sgpp/parallel/tools/TypesParallel.hpp>

#include <string>

namespace sg {
  namespace parallel {

    /**
     * Class that implements the virtual class sg::base::OperationMatrix for the
     * application of classification for the Systemmatrix
     *
     * The Identity matrix is used as regularization operator.
     *
     * For the Operation B's mult and mutlTransposed functions
     * vectorized formulations are used.
     */
    class DMSystemMatrixVectorizedIdentityMPI : public sg::datadriven::DMSystemMatrixBase {
      private:
        /// vectorization mode
        VectorizationType vecMode_;
        /// Number of orignal training instances
        size_t numTrainingInstances_;
        /// Number of patched and used training instances
        size_t numPatchedTrainingInstances_;
        /// OperationB for calculating the data matrix
        sg::parallel::OperationMultipleEvalVectorized* B_;

        double waitting_time;
      public:
        /**
         * Std-Constructor
         *
         * @param SparseGrid reference to the sparse grid
         * @param trainData reference to sg::base::DataMatrix that contains the training data
         * @param lambda the lambda, the regression parameter
         * @param vecMode vectorization mode
         */
        DMSystemMatrixVectorizedIdentityMPI(sg::base::Grid& SparseGrid, sg::base::DataMatrix& trainData, double lambda, VectorizationType vecMode);

        /**
         * Std-Destructor
         */
        virtual ~DMSystemMatrixVectorizedIdentityMPI();

        virtual void mult(sg::base::DataVector& alpha, sg::base::DataVector& result);

        virtual void generateb(sg::base::DataVector& classes, sg::base::DataVector& b);

        virtual void rebuildLevelAndIndex();

      private:
        /// how to distribute storage array
        int* _mpi_grid_sizes;
        int* _mpi_grid_offsets;

        /// reference to grid. needed to get new grid size after it changes
        sg::base::Grid& m_grid;

        /// how to distribute dataset
        int* _mpi_data_sizes;
        int* _mpi_data_offsets;

        /**
         * Wrapper function that handles communication after calculation and time measurement
         */
        void multVec(base::DataVector& alpha, base::DataVector& result);

        /**
         * Wrapper function that handles communication after calculation and time measurement
         */
        void multTransposeVec(base::DataVector& source, base::DataVector& result);
    };

  }
}

#endif /* DMSYSTEMMATRIXVECTORIZEDIDENTITYMPI_HPP */