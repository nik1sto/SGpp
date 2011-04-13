/******************************************************************************
* Copyright (C) 2009 Technische Universitaet Muenchen                         *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
******************************************************************************/

#include "grid/common/DirichletUpdateVector.hpp"
#include "solver/ode/AdamsBashforth.hpp"
#include "operation/common/OperationEval.hpp"
#include "tools/common/GridPrinter.hpp"
#include "exception/solver_exception.hpp"

#include <iostream>
#include <string>
#include <sstream>

namespace sg
{

AdamsBashforth::AdamsBashforth(size_t imax, double timestepSize, ScreenOutput* screen) : ODESolver(imax, timestepSize), myScreen(screen)
{
	this->residuum = 0.0;

}

AdamsBashforth::~AdamsBashforth()
{
}

void AdamsBashforth::solve(SLESolver& LinearSystemSolver, OperationParabolicPDESolverSystem& System, bool bIdentifyLastStep, bool verbose)
{
	size_t allIter = 0;
    DataVector* rhs;

	for (size_t i = 0; i < this->nMaxIterations; i++)
	{
		if(i > 0)
			System.setODESolver("AdBas");
		else
			System.setODESolver("ExEul");

		// generate right hand side
		rhs = System.generateRHS();


		// solve the system of the current timestep
		LinearSystemSolver.solve(System, *System.getGridCoefficientsForCG(), *rhs, true, false, -1.0);

	    allIter += LinearSystemSolver.getNumberIterations();
	    if (verbose == true)
	    {
	    	if (myScreen == NULL)
	    	{
	    		std::cout << "Final residuum " << LinearSystemSolver.residuum << "; with " << LinearSystemSolver.getNumberIterations() << " Iterations (Total Iter.: " << allIter << ")" << std::endl;
	    	}
	    }
	    if (myScreen != NULL)
    	{
    		std::stringstream soutput;
    		soutput << "Final residuum " << LinearSystemSolver.residuum << "; with " << LinearSystemSolver.getNumberIterations() << " Iterations (Total Iter.: " << allIter << ")";

    		if (i < this->nMaxIterations-1)
    		{
    			myScreen->update((size_t)(((double)(i+1)*100.0)/((double)this->nMaxIterations)), soutput.str());
    		}
    		else
    		{
    			myScreen->update(100, soutput.str());
    		}
    	}
	    if (bIdentifyLastStep == false)
	    {
			System.finishTimestep(false);
	    }
	    else
	    {
			if (i < (this->nMaxIterations-1))
			{
				System.finishTimestep(false);
			}
			else
			{
				System.finishTimestep(true);
			}
	    }
	    System.saveAlpha();

	}

	// write some empty lines to console
    if (myScreen != NULL)
	{
    	myScreen->writeEmptyLines(2);
	}

    this->nIterations = allIter;
}

}