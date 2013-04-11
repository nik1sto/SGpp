/* ****************************************************************************
* Copyright (C) 2010 Technische Universitaet Muenchen                         *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
**************************************************************************** */
// @author Alexander Heinecke (Alexander.Heinecke@mytum.de)
// @author Roman Karlstetter (karlstetter@mytum.de)

#ifndef DMSYSTEMMATRIXSPVECTORIZEDIDENTITYMPI_HPP
#define DMSYSTEMMATRIXSPVECTORIZEDIDENTITYMPI_HPP

#include "base/datatypes/DataVectorSP.hpp"
#include "base/grid/Grid.hpp"

#include "datadriven/algorithm/DMSystemMatrixBaseSP.hpp"

#include "parallel/datadriven/operation/OperationMultipleEvalVectorizedSP.hpp"
#include "parallel/tools/TypesParallel.hpp"

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
     *
     * In this class single precision DataVectors are used.
     */
    class DMSystemMatrixSPVectorizedIdentityMPI : public sg::datadriven::DMSystemMatrixBaseSP {
      private:
        /// vectorization mode
        VectorizationType vecMode_;
        /// Number of original training instances
        size_t numTrainingInstances_;
        /// Number of patched and used training instances
        size_t numPatchedTrainingInstances_;
        /// OperationB for calculating the data matrix
        sg::parallel::OperationMultipleEvalVectorizedSP* B_;

      public:
        /**
         * Std-Constructor
         *
         * @param SparseGrid reference to the sparse grid
         * @param trainData reference to sg::base::DataMatrix that contains the training data
         * @param lambda the lambda, the regression parameter
         * @param vecMode vectorization mode, possible values are SSE, AVX, OCL, ArBB
         */
        DMSystemMatrixSPVectorizedIdentityMPI(sg::base::Grid& SparseGrid, sg::base::DataMatrixSP& trainData, float lambda, VectorizationType vecMode);

        /**
         * Std-Destructor
         */
        virtual ~DMSystemMatrixSPVectorizedIdentityMPI();

        virtual void mult(sg::base::DataVectorSP& alpha, sg::base::DataVectorSP& result);

        virtual void generateb(sg::base::DataVectorSP& classes, sg::base::DataVectorSP& b);

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
        void multVec(base::DataVectorSP& alpha, base::DataVectorSP& result);

        /**
         * Wrapper function that handles communication after calculation and time measurement
         */
        void multTransposeVec(base::DataVectorSP& source, base::DataVectorSP& result);

        /**
         * calculates the distribution for the current MPI setting for a domain of
         * size totalSize and stores the result into the arrays sizes and offsets. These
         * arrays must have a size equal to the number of MPI processes currently running.
         *
         * @param totalSize size of domain to distribute
         * @param sizes output array to store resulting distribution sizes (array size must match the number of MPI processes)
         * @param offsets output array to store resulting distribution offsets (array size must match the number of MPI processes)
         * @param blocksize blocksize
         *
         */
        void calcDistribution(size_t totalSize, int* sizes, int* offsets, size_t blocksize);

    };

  }
}

#endif /* DMSYSTEMMATRIXSPVECTORIZEDIDENTITYMPI_HPP */
