/* ****************************************************************************
* Copyright (C) 2013 Technische Universitaet Muenchen                         *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
**************************************************************************** */
// @author Alexander Heinecke (Alexander.Heinecke@mytum.de)

#ifndef HEATEQUATIONPARABOLICPDESOLVERSYSTEMVECTORIZEDMPI_HPP
#define HEATEQUATIONPARABOLICPDESOLVERSYSTEMVECTORIZEDMPI_HPP

#include <sgpp/parallel/pde/operation/OperationParabolicPDESolverSystemDirichletCombined.hpp>
#include <sgpp/parallel/pde/operation/OperationParabolicPDEMatrixCombined.hpp>

#include <sgpp/base/datatypes/DataVector.hpp>
#include <sgpp/base/grid/Grid.hpp>

namespace sg {
  namespace parallel {

    /**
     * This class implements the ParabolicPDESolverSystem for the
     * Heat Equation using vectorized and iterative operators
     */
    class HeatEquationParabolicPDESolverSystemVectorizedMPI : public sg::parallel::OperationParabolicPDESolverSystemDirichletCombined {
      private:
        /// the heat coefficient
        double a;
        /// the Laplace Operation, on boundary grid
        sg::base::OperationMatrix* OpLaplaceBound;
        /// the LTwoDotProduct Operation (Mass Matrix), on boundary grid
        sg::base::OperationMatrix* OpLTwoBound;
        /// the Laplace Operation, on Inner grid
        sg::base::OperationMatrix* OpLaplaceInner;
        /// the LTwoDotProduct Operation (Mass Matrix), on Inner grid
        sg::base::OperationMatrix* OpLTwoInner;
        /// the combination of the LTwoDotProduct Operation (Mass Matrix) and the Laplace Operation, on Inner grid
        sg::parallel::OperationParabolicPDEMatrixCombined* OpLTwoDotLaplaceInner;
        /// the combination of the LTwoDotProduct Operation (Mass Matrix) and the Laplace Operation, on Bound grid
        sg::parallel::OperationParabolicPDEMatrixCombined* OpLTwoDotLaplaceBound;

        virtual void applyMassMatrixComplete(sg::base::DataVector& alpha, sg::base::DataVector& result);

        virtual void applyLOperatorComplete(sg::base::DataVector& alpha, sg::base::DataVector& result);

        virtual void applyMassMatrixInner(sg::base::DataVector& alpha, sg::base::DataVector& result);

        virtual void applyLOperatorInner(sg::base::DataVector& alpha, sg::base::DataVector& result);

        virtual void applyMassMatrixLOperatorInner(sg::base::DataVector& alpha, sg::base::DataVector& result);

        virtual void applyMassMatrixLOperatorBound(sg::base::DataVector& alpha, sg::base::DataVector& result);

        virtual void setTimestepCoefficientInner(double timestep_coefficient);

        virtual void setTimestepCoefficientBound(double timestep_coefficient);

      public:
        /**
         * Std-Constructor
         *
         * @param SparseGrid reference to the sparse grid
         * @param alpha the sparse grid's coefficients
         * @param a the heat coefficient
         * @param TimestepSize the size of one timestep used in the ODE Solver
         * @param OperationMode specifies in which solver this matrix is used, valid values are:
         *                ImEul for implicit Euler, CrNic for Crank Nicolson solver
         */
        HeatEquationParabolicPDESolverSystemVectorizedMPI(sg::base::Grid& SparseGrid, sg::base::DataVector& alpha, double a, double TimestepSize, std::string OperationMode = "ImEul");

        /**
         * Std-Destructor
         */
        virtual ~HeatEquationParabolicPDESolverSystemVectorizedMPI();

        virtual void finishTimestep();

        virtual void coarsenAndRefine(bool isLastTimestep = false);

        void startTimestep();
    };

  }
}

#endif /* HEATEQUATIONPARABOLICPDESOLVERSYSTEMVECTORIZEDMPI_HPP */