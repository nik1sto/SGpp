/* ****************************************************************************
* Copyright (C) 2014 Technische Universitaet Muenchen                         *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
**************************************************************************** */
// @author Julian Valentin (julian.valentin@stud.mathematik.uni-stuttgart.de)

#ifndef SGPP_OPT_HPP
#define SGPP_OPT_HPP

#include "opt/basis/bspline/boundary/operation/OperationMultipleHierarchisationBsplineBoundary.hpp"
#include "opt/basis/bspline/clenshawcurtis/operation/OperationMultipleHierarchisationBsplineClenshawCurtis.hpp"
#include "opt/basis/bspline/modified/operation/OperationMultipleHierarchisationModBspline.hpp"
#include "opt/basis/bspline/noboundary/operation/OperationMultipleHierarchisationBspline.hpp"
#include "opt/basis/linear/boundary/operation/OperationMultipleHierarchisationLinearBoundary.hpp"
#include "opt/basis/linear/clenshawcurtis/operation/OperationMultipleHierarchisationLinearClenshawCurtis.hpp"
#include "opt/basis/linear/modified/operation/OperationMultipleHierarchisationModLinear.hpp"
#include "opt/basis/linear/noboundary/operation/OperationMultipleHierarchisationLinear.hpp"
#include "opt/basis/wavelet/boundary/operation/OperationMultipleHierarchisationWaveletBoundary.hpp"
#include "opt/basis/wavelet/modified/operation/OperationMultipleHierarchisationModWavelet.hpp"
#include "opt/basis/wavelet/noboundary/operation/OperationMultipleHierarchisationWavelet.hpp"

#include "opt/function/Interpolant.hpp"
#include "opt/function/InterpolantGradient.hpp"
#include "opt/function/InterpolantHessian.hpp"
#include "opt/function/Objective.hpp"
#include "opt/function/ObjectiveGradient.hpp"
#include "opt/function/ObjectiveHessian.hpp"
#include "opt/function/test/Ackley.hpp"
#include "opt/function/test/Beale.hpp"
#include "opt/function/test/Branin.hpp"
#include "opt/function/test/Easom.hpp"
#include "opt/function/test/Eggholder.hpp"
#include "opt/function/test/GoldsteinPrice.hpp"
#include "opt/function/test/Griewank.hpp"
#include "opt/function/test/Hartman3.hpp"
#include "opt/function/test/Hartman6.hpp"
#include "opt/function/test/Himmelblau.hpp"
#include "opt/function/test/HoelderTable.hpp"
#include "opt/function/test/Michalewicz.hpp"
#include "opt/function/test/Mladineo.hpp"
#include "opt/function/test/Rastrigin.hpp"
#include "opt/function/test/Rosenbrock.hpp"
#include "opt/function/test/SHCB.hpp"
#include "opt/function/test/Schwefel.hpp"
#include "opt/function/test/Sphere.hpp"
#include "opt/function/test/Test.hpp"

#include "opt/gridgen/HashRefinementMultiple.hpp"
#include "opt/gridgen/IterativeGridGenerator.hpp"
#include "opt/gridgen/IterativeGridGeneratorLinearSurplus.hpp"
#include "opt/gridgen/IterativeGridGeneratorRitterNovak.hpp"

#include "opt/operation/OpFactory.hpp"
#include "opt/operation/OperationMultipleHierarchisation.hpp"

#include "opt/optimizer/DifferentialEvolution.hpp"
#include "opt/optimizer/GradientMethod.hpp"
#include "opt/optimizer/LineSearchArmijo.hpp"
#include "opt/optimizer/NelderMead.hpp"
#include "opt/optimizer/Newton.hpp"
#include "opt/optimizer/NLCG.hpp"
#include "opt/optimizer/Optimizer.hpp"
#include "opt/optimizer/RandomSearch.hpp"

#include "opt/sle/solver/Armadillo.hpp"
#include "opt/sle/solver/Auto.hpp"
#include "opt/sle/solver/BiCGStab.hpp"
#include "opt/sle/solver/Eigen.hpp"
#include "opt/sle/solver/Gmmpp.hpp"
#include "opt/sle/solver/Solver.hpp"
#include "opt/sle/solver/UMFPACK.hpp"
#include "opt/sle/system/Cloneable.hpp"
#include "opt/sle/system/Full.hpp"
#include "opt/sle/system/Hierarchisation.hpp"
#include "opt/sle/system/System.hpp"

#include "opt/tools/MutexType.hpp"
#include "opt/tools/Permuter.hpp"
#include "opt/tools/Printer.hpp"
#include "opt/tools/RNG.hpp"
#include "opt/tools/ScopedLock.hpp"
#include "opt/tools/SmartPointer.hpp"

#endif
