/******************************************************************************
* Copyright (C) 2009 Technische Universitaet Muenchen                         *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
******************************************************************************/
// @author Alexander Heinecke (Alexander.Heinecke@mytum.de)

#include "algorithm/pde/BlackScholesParabolicPDESolverSystem.hpp"
#include "algorithm/pde/BlackScholesParabolicPDESolverSystemEuropean.hpp"
#include "algorithm/pde/BlackScholesParabolicPDESolverSystemEuropeanParallelOMP.hpp"
#include "application/pde/BlackScholesSolver.hpp"
#include "solver/ode/Euler.hpp"
#include "solver/ode/CrankNicolson.hpp"
#include "solver/ode/AdamsBashforth.hpp"
#include "solver/ode/VarTimestep.hpp"
#include "solver/ode/StepsizeControlH.hpp"
#include "solver/ode/StepsizeControlBDF.hpp"
#include "solver/ode/StepsizeControlEJ.hpp"
#include "solver/sle/BiCGStab.hpp"
#include "grid/Grid.hpp"
#include "exception/application_exception.hpp"
#include <cstdlib>
#include <sstream>
#include <cmath>
#include <fstream>
#include <iomanip>

namespace sg
{

BlackScholesSolver::BlackScholesSolver(bool useLogTransform, std::string OptionType) : ParabolicPDESolver()
{
	this->bStochasticDataAlloc = false;
	this->bGridConstructed = false;
	this->myScreen = NULL;
	this->useCoarsen = false;
	this->coarsenThreshold = 0.0;
	this->adaptSolveMode = "none";
	this->refineMode = "classic";
	this->numCoarsenPoints = -1;
	this->useLogTransform = useLogTransform;
	this->refineMaxLevel = 0;
	this->nNeededIterations = 0;
	this->dNeededTime = 0.0;
	this->staInnerGridSize = 0;
	this->finInnerGridSize = 0;
	this->avgInnerGridSize = 0;
	if (OptionType == "all" || OptionType == "European")
	{
		this->tOptionType = OptionType;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::BlackScholesSolver : An unsupported option type has been chosen! all & European are supported!");
	}
}

BlackScholesSolver::~BlackScholesSolver()
{
	if (this->bStochasticDataAlloc)
	{
		delete this->mus;
		delete this->sigmas;
		delete this->rhos;
	}
	if (this->myScreen != NULL)
	{
		delete this->myScreen;
	}
}

void BlackScholesSolver::getGridNormalDistribution(DataVector& alpha, std::vector<double>& norm_mu, std::vector<double>& norm_sigma)
{
	if (this->bGridConstructed)
	{
		double tmp;
		double value;
		StdNormalDistribution myNormDistr;

		for (size_t i = 0; i < this->myGrid->getStorage()->size(); i++)
		{
			std::string coords = this->myGridStorage->get(i)->getCoordsStringBB(*(this->myBoundingBox));
			std::stringstream coordsStream(coords);

			value = 1.0;
			for (size_t j = 0; j < this->dim; j++)
			{
				coordsStream >> tmp;

				if (this->useLogTransform == false)
				{
					value *= myNormDistr.getDensity(tmp, norm_mu[j], norm_sigma[j]);
				}
				else
				{
					value *= myNormDistr.getDensity(exp(tmp), norm_mu[j], norm_sigma[j]);
				}
			}

			alpha[i] = value;
		}
	}
	else
	{
		throw new application_exception("BlackScholesSolver::getGridNormalDistribution : The grid wasn't initialized before!");
	}
}

void BlackScholesSolver::constructGrid(BoundingBox& BoundingBox, size_t level)
{
	this->dim = BoundingBox.getDimensions();
	this->levels = level;

	this->myGrid = new LinearTrapezoidBoundaryGrid(BoundingBox);

	GridGenerator* myGenerator = this->myGrid->createGridGenerator();
	myGenerator->regular(this->levels);
	delete myGenerator;

	this->myBoundingBox = this->myGrid->getBoundingBox();
	this->myGridStorage = this->myGrid->getStorage();

	//std::string serGrid;
	//myGrid->serialize(serGrid);
	//std::cout << serGrid << std::endl;

	this->bGridConstructed = true;
}

void BlackScholesSolver::refineInitialGridWithPayoff(DataVector& alpha, double strike, std::string payoffType, double dStrikeDistance)
{
	size_t nRefinements = 0;

	if (this->useLogTransform == false)
	{
		if (this->bGridConstructed)
		{

			DataVector refineVector(alpha.getSize());

			if (payoffType == "std_euro_call" || payoffType == "std_euro_put")
			{
				double tmp;
				double* dblFuncValues = new double[dim];
				double dDistance = 0.0;

				for (size_t i = 0; i < this->myGrid->getStorage()->size(); i++)
				{
					std::string coords = this->myGridStorage->get(i)->getCoordsStringBB(*(this->myBoundingBox));
					std::stringstream coordsStream(coords);

					for (size_t j = 0; j < this->dim; j++)
					{
						coordsStream >> tmp;

						dblFuncValues[j] = tmp;
					}

					tmp = 0.0;
					for (size_t j = 0; j < this->dim; j++)
					{
						tmp += dblFuncValues[j];
					}

					if (payoffType == "std_euro_call")
					{
						dDistance = fabs(((tmp/static_cast<double>(this->dim))-strike));
					}
					if (payoffType == "std_euro_put")
					{
						dDistance = fabs((strike-(tmp/static_cast<double>(this->dim))));
					}

					if (dDistance <= dStrikeDistance)
					{
						refineVector[i] = dDistance;
						nRefinements++;
					}
					else
					{
						refineVector[i] = 0.0;
					}
				}

				delete[] dblFuncValues;

				SurplusRefinementFunctor* myRefineFunc = new SurplusRefinementFunctor(&refineVector, nRefinements, 0.0);

				this->myGrid->createGridGenerator()->refine(myRefineFunc);

				delete myRefineFunc;

				alpha.resize(this->myGridStorage->size());

				// reinit the grid with the payoff function
				initGridWithPayoff(alpha, strike, payoffType);
			}
			else
			{
				throw new application_exception("BlackScholesSolver::refineInitialGridWithPayoff : An unsupported payoffType was specified!");
			}
		}
		else
		{
			throw new application_exception("BlackScholesSolver::refineInitialGridWithPayoff : The grid wasn't initialized before!");
		}
	}
}

void BlackScholesSolver::refineInitialGridWithPayoffToMaxLevel(DataVector& alpha, double strike, std::string payoffType, double dStrikeDistance, size_t maxLevel)
{
	size_t nRefinements = 0;

	if (this->useLogTransform == false)
	{
		if (this->bGridConstructed)
		{

			DataVector refineVector(alpha.getSize());

			if (payoffType == "std_euro_call" || payoffType == "std_euro_put")
			{
				double tmp;
				double* dblFuncValues = new double[dim];
				double dDistance = 0.0;

				for (size_t i = 0; i < this->myGrid->getStorage()->size(); i++)
				{
					std::string coords = this->myGridStorage->get(i)->getCoordsStringBB(*this->myBoundingBox);
					std::stringstream coordsStream(coords);

					for (size_t j = 0; j < this->dim; j++)
					{
						coordsStream >> tmp;

						dblFuncValues[j] = tmp;
					}

					tmp = 0.0;
					for (size_t j = 0; j < this->dim; j++)
					{
						tmp += dblFuncValues[j];
					}

					if (payoffType == "std_euro_call")
					{
						dDistance = fabs(((tmp/static_cast<double>(this->dim))-strike));
					}
					if (payoffType == "std_euro_put")
					{
						dDistance = fabs((strike-(tmp/static_cast<double>(this->dim))));
					}

					if (dDistance <= dStrikeDistance)
					{
						refineVector[i] = dDistance;
						nRefinements++;
					}
					else
					{
						refineVector[i] = 0.0;
					}
				}

				delete[] dblFuncValues;

				SurplusRefinementFunctor* myRefineFunc = new SurplusRefinementFunctor(&refineVector, nRefinements, 0.0);

				this->myGrid->createGridGenerator()->refineMaxLevel(myRefineFunc, maxLevel);

				delete myRefineFunc;

				alpha.resize(this->myGridStorage->size());

				// reinit the grid with the payoff function
				initGridWithPayoff(alpha, strike, payoffType);
			}
			else
			{
				throw new application_exception("BlackScholesSolver::refineInitialGridWithPayoffToMaxLevel : An unsupported payoffType was specified!");
			}
		}
		else
		{
			throw new application_exception("BlackScholesSolver::refineInitialGridWithPayoffToMaxLevel : The grid wasn't initialized before!");
		}
	}
}

void BlackScholesSolver::setStochasticData(DataVector& mus, DataVector& sigmas, DataMatrix& rhos, double r)
{
	this->mus = new DataVector(mus);
	this->sigmas = new DataVector(sigmas);
	this->rhos = new DataMatrix(rhos);
	this->r = r;

	bStochasticDataAlloc = true;
}

void BlackScholesSolver::solveExplicitEuler(size_t numTimesteps, double timestepsize, size_t maxCGIterations, double epsilonCG, DataVector& alpha, bool verbose, bool generateAnimation, size_t numEvalsAnimation)
{
	if (this->bGridConstructed && this->bStochasticDataAlloc)
	{
		Euler* myEuler = new Euler("ExEul", numTimesteps, timestepsize, generateAnimation, numEvalsAnimation, myScreen);
		BiCGStab* myCG = new BiCGStab(maxCGIterations, epsilonCG);
		OperationParabolicPDESolverSystem* myBSSystem = NULL;

		if (this->tOptionType == "European")
		{
#ifdef _OPENMP
			myBSSystem = new BlackScholesParabolicPDESolverSystemEuropeanParallelOMP(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "ExEul", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
#else
			myBSSystem = new BlackScholesParabolicPDESolverSystemEuropean(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "ExEul", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
#endif
		}
		else
		{
			myBSSystem = new BlackScholesParabolicPDESolverSystem(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "ExEul", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
		}

		SGppStopwatch* myStopwatch = new SGppStopwatch();
		this->staInnerGridSize = getNumberInnerGridPoints();

		std::cout << "Using Explicit Euler to solve " << numTimesteps << " timesteps:" << std::endl;
		myStopwatch->start();
		myEuler->solve(*myCG, *myBSSystem, true, verbose);
		this->dNeededTime = myStopwatch->stop();

		std::cout << std::endl << "Final Grid size: " << getNumberGridPoints() << std::endl;
		std::cout << "Final Grid size (inner): " << getNumberInnerGridPoints() << std::endl << std::endl << std::endl;

		std::cout << "Average Grid size: " << static_cast<double>(myBSSystem->getSumGridPointsComplete())/static_cast<double>(numTimesteps) << std::endl;
		std::cout << "Average Grid size (Inner): " << static_cast<double>(myBSSystem->getSumGridPointsInner())/static_cast<double>(numTimesteps) << std::endl << std::endl << std::endl;

		if (this->myScreen != NULL)
		{
			std::cout << "Time to solve: " << this->dNeededTime << " seconds" << std::endl;
			this->myScreen->writeEmptyLines(2);
		}

		this->finInnerGridSize = getNumberInnerGridPoints();
		this->avgInnerGridSize = static_cast<size_t>((static_cast<double>(myBSSystem->getSumGridPointsInner())/static_cast<double>(numTimesteps))+0.5);
		this->nNeededIterations = myEuler->getNumberIterations();

		delete myBSSystem;
		delete myCG;
		delete myEuler;
		delete myStopwatch;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::solveExplicitEuler : A grid wasn't constructed before or stochastic parameters weren't set!");
	}
}

void BlackScholesSolver::solveImplicitEuler(size_t numTimesteps, double timestepsize, size_t maxCGIterations, double epsilonCG, DataVector& alpha, bool verbose, bool generateAnimation, size_t numEvalsAnimation)
{
	if (this->bGridConstructed && this->bStochasticDataAlloc)
	{
		Euler* myEuler = new Euler("ImEul", numTimesteps, timestepsize, generateAnimation, numEvalsAnimation, myScreen);
		BiCGStab* myCG = new BiCGStab(maxCGIterations, epsilonCG);
		OperationParabolicPDESolverSystem* myBSSystem = NULL;

		if (this->tOptionType == "European")
		{
#ifdef _OPENMP
			myBSSystem = new BlackScholesParabolicPDESolverSystemEuropeanParallelOMP(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "ImEul", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
#else
			myBSSystem = new BlackScholesParabolicPDESolverSystemEuropean(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "ImEul", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
#endif
		}
		else
		{
			myBSSystem = new BlackScholesParabolicPDESolverSystem(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "ImEul", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
		}

		SGppStopwatch* myStopwatch = new SGppStopwatch();
		this->staInnerGridSize = getNumberInnerGridPoints();

		std::cout << "Using Implicit Euler to solve " << numTimesteps << " timesteps:" << std::endl;
		myStopwatch->start();
		myEuler->solve(*myCG, *myBSSystem, true, verbose);
		this->dNeededTime = myStopwatch->stop();

		std::cout << std::endl << "Final Grid size: " << getNumberGridPoints() << std::endl;
		std::cout << "Final Grid size (inner): " << getNumberInnerGridPoints() << std::endl << std::endl << std::endl;

		std::cout << "Average Grid size: " << static_cast<double>(myBSSystem->getSumGridPointsComplete())/static_cast<double>(numTimesteps) << std::endl;
		std::cout << "Average Grid size (Inner): " << static_cast<double>(myBSSystem->getSumGridPointsInner())/static_cast<double>(numTimesteps) << std::endl << std::endl << std::endl;

		if (this->myScreen != NULL)
		{
			std::cout << "Time to solve: " << this->dNeededTime << " seconds" << std::endl;
			this->myScreen->writeEmptyLines(2);
		}

		this->finInnerGridSize = getNumberInnerGridPoints();
		this->avgInnerGridSize = static_cast<size_t>((static_cast<double>(myBSSystem->getSumGridPointsInner())/static_cast<double>(numTimesteps))+0.5);
		this->nNeededIterations = myEuler->getNumberIterations();

		delete myBSSystem;
		delete myCG;
		delete myEuler;
		delete myStopwatch;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::solveImplicitEuler : A grid wasn't constructed before or stochastic parameters weren't set!");
	}
}

void BlackScholesSolver::solveCrankNicolson(size_t numTimesteps, double timestepsize, size_t maxCGIterations, double epsilonCG, DataVector& alpha, size_t NumImEul)
{
	if (this->bGridConstructed && this->bStochasticDataAlloc)
	{
		BiCGStab* myCG = new BiCGStab(maxCGIterations, epsilonCG);
		OperationParabolicPDESolverSystem* myBSSystem = NULL;

		if (this->tOptionType == "European")
		{
#ifdef _OPENMP
			myBSSystem = new BlackScholesParabolicPDESolverSystemEuropeanParallelOMP(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "CrNic", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
#else
			myBSSystem = new BlackScholesParabolicPDESolverSystemEuropean(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "CrNic", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
#endif
		}
		else
		{
			myBSSystem = new BlackScholesParabolicPDESolverSystem(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "CrNic", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
		}

		SGppStopwatch* myStopwatch = new SGppStopwatch();
		this->staInnerGridSize = getNumberInnerGridPoints();

		size_t numCNSteps;
		size_t numIESteps;

		numCNSteps = numTimesteps;
		if (numTimesteps > NumImEul)
		{
			numCNSteps = numTimesteps - NumImEul;
		}
		numIESteps = NumImEul;

		Euler* myEuler = new Euler("ImEul", numIESteps, timestepsize, false, 0, this->myScreen);
		CrankNicolson* myCN = new CrankNicolson(numCNSteps, timestepsize, this->myScreen);

		myStopwatch->start();
		if (numIESteps > 0)
		{
			std::cout << "Using Implicit Euler to solve " << numIESteps << " timesteps:" << std::endl;
			myBSSystem->setODESolver("ImEul");
			myEuler->solve(*myCG, *myBSSystem, false, false);
		}
		myBSSystem->setODESolver("CrNic");
		std::cout << "Using Crank Nicolson to solve " << numCNSteps << " timesteps:" << std::endl << std::endl << std::endl << std::endl;
		myCN->solve(*myCG, *myBSSystem, true, false);
		this->dNeededTime = myStopwatch->stop();

		std::cout << std::endl << "Final Grid size: " << getNumberGridPoints() << std::endl;
		std::cout << "Final Grid size (inner): " << getNumberInnerGridPoints() << std::endl << std::endl << std::endl;

		std::cout << "Average Grid size: " << static_cast<double>(myBSSystem->getSumGridPointsComplete())/static_cast<double>(numTimesteps) << std::endl;
		std::cout << "Average Grid size (Inner): " << static_cast<double>(myBSSystem->getSumGridPointsInner())/static_cast<double>(numTimesteps) << std::endl << std::endl << std::endl;

		if (this->myScreen != NULL)
		{
			std::cout << "Time to solve: " << this->dNeededTime << " seconds" << std::endl;
			this->myScreen->writeEmptyLines(2);
		}

		this->finInnerGridSize = getNumberInnerGridPoints();
		this->avgInnerGridSize = static_cast<size_t>((static_cast<double>(myBSSystem->getSumGridPointsInner())/static_cast<double>(numTimesteps))+0.5);
		this->nNeededIterations = myEuler->getNumberIterations() + myCN->getNumberIterations();

		delete myBSSystem;
		delete myCG;
		delete myCN;
		delete myEuler;
		delete myStopwatch;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::solveCrankNicolson : A grid wasn't constructed before or stochastic parameters weren't set!");
	}
}


void BlackScholesSolver::solveAdamsBashforth(size_t numTimesteps, double timestepsize, size_t maxCGIterations, double epsilonCG, DataVector& alpha, bool verbose)
{
	ODESolver* myODESolver = new AdamsBashforth(numTimesteps, timestepsize, myScreen);
	BlackScholesSolver::solveX(numTimesteps, timestepsize, maxCGIterations, epsilonCG, alpha, verbose, myODESolver, "AdBas");
	delete myODESolver;
}

void BlackScholesSolver::solveSCAC(size_t numTimesteps, double timestepsize, double epsilon, size_t maxCGIterations, double epsilonCG, DataVector& alpha, bool verbose)
{
	ODESolver* myODESolver = new VarTimestep(numTimesteps, timestepsize, epsilon, myScreen);
	BlackScholesSolver::solveX(numTimesteps, timestepsize, maxCGIterations, epsilonCG, alpha, verbose, myODESolver, "CrNic");
	delete myODESolver;
}

void BlackScholesSolver::solveSCH(size_t numTimesteps, double timestepsize, double epsilon, size_t maxCGIterations, double epsilonCG, DataVector& alpha, bool verbose)
{
	ODESolver* myODESolver = new StepsizeControlH(numTimesteps, timestepsize, epsilon, myScreen);
	BlackScholesSolver::solveX(numTimesteps, timestepsize, maxCGIterations, epsilonCG, alpha, verbose, myODESolver, "CrNic");
	delete myODESolver;
}

void BlackScholesSolver::solveSCBDF(size_t numTimesteps, double timestepsize, double epsilon, size_t maxCGIterations, double epsilonCG, DataVector& alpha, bool verbose)
{
	ODESolver* myODESolver = new StepsizeControlBDF(numTimesteps, timestepsize, epsilon, myScreen);
	BlackScholesSolver::solveX(numTimesteps, timestepsize, maxCGIterations, epsilonCG, alpha, verbose, myODESolver, "SCBDF");
	delete myODESolver;
}

void BlackScholesSolver::solveSCEJ(size_t numTimesteps, double timestepsize, double epsilon, double myAlpha, size_t maxCGIterations, double epsilonCG, DataVector& alpha, bool verbose)
{
	ODESolver* myODESolver = new StepsizeControlEJ(numTimesteps, timestepsize, epsilon, myAlpha,  myScreen);
	BlackScholesSolver::solveX(numTimesteps, timestepsize, maxCGIterations, epsilonCG, alpha, verbose, myODESolver, "SCEJ");
	delete myODESolver;
}

void BlackScholesSolver::solveX(size_t numTimesteps, double timestepsize, size_t maxCGIterations, double epsilonCG, DataVector& alpha, bool verbose, void *myODESolverV, std::string Solver)
{
	ODESolver *myODESolver = (ODESolver *)myODESolverV;
	if (this->bGridConstructed && this->bStochasticDataAlloc)
	{		BiCGStab* myCG = new BiCGStab(maxCGIterations, epsilonCG);
		OperationParabolicPDESolverSystem* myBSSystem = NULL;

		if (this->tOptionType == "European")
		{
#ifdef _OPENMP
			myBSSystem = new BlackScholesParabolicPDESolverSystemEuropeanParallelOMP(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, Solver, this->useLogTransform, false, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
#else
			myBSSystem = new BlackScholesParabolicPDESolverSystemEuropean(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, Solver, this->useLogTransform, false, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
#endif
		}
		else
		{
			myBSSystem = new BlackScholesParabolicPDESolverSystem(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, Solver, this->useLogTransform, false, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
		}

		SGppStopwatch* myStopwatch = new SGppStopwatch();
		this->staInnerGridSize = getNumberInnerGridPoints();

		myStopwatch->start();
		myODESolver->solve(*myCG, *myBSSystem, false, verbose);
		this->dNeededTime = myStopwatch->stop();

		if (this->myScreen != NULL)
		{
			std::cout << "Time to solve: " << this->dNeededTime << " seconds" << std::endl;
			this->myScreen->writeEmptyLines(2);
		}

		this->finInnerGridSize = getNumberInnerGridPoints();
		this->avgInnerGridSize = static_cast<size_t>((static_cast<double>(myBSSystem->getSumGridPointsInner())/static_cast<double>(numTimesteps))+0.5);
		this->nNeededIterations = myODESolver->getNumberIterations();

		delete myBSSystem;
		delete myCG;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::solveX : A grid wasn't constructed before or stochastic parameters weren't set!");
	}
}


void BlackScholesSolver::initGridWithPayoff(DataVector& alpha, double strike, std::string payoffType)
{
	if (this->useLogTransform)
	{
		initLogTransformedGridWithPayoff(alpha, strike, payoffType);
	}
	else
	{
		initCartesianGridWithPayoff(alpha, strike, payoffType);
	}
}

double BlackScholesSolver::get1DEuroCallPayoffValue(double assetValue, double strike)
{
	if (assetValue <= strike)
	{
		return 0.0;
	}
	else
	{
		return assetValue - strike;
	}
}

void BlackScholesSolver::solve1DAnalytic(std::vector< std::pair<double, double> >& premiums, double maxStock, double StockInc, double strike, double t, bool isCall)
{
	if (bStochasticDataAlloc)
	{
		double stock = 0.0;
		double vola = this->sigmas->get(0);
		StdNormalDistribution* myStdNDis = new StdNormalDistribution();

		for (stock = 0.0; stock <= maxStock; stock += StockInc)
		{
			double dOne = (log((stock/strike)) + ((this->r + (vola*vola*0.5))*(t)))/(vola*sqrt(t));
			double dTwo = dOne - (vola*sqrt(t));
			double prem;
			if (isCall)
			{
				prem = (stock*myStdNDis->getCumulativeDensity(dOne)) - (strike*myStdNDis->getCumulativeDensity(dTwo)*(exp((-1.0)*this->r*t)));
			}
			else
			{
				prem = (strike*myStdNDis->getCumulativeDensity(dTwo*(-1.0))*(exp((-1.0)*this->r*t))) - (stock*myStdNDis->getCumulativeDensity(dOne*(-1.0)));
			}
			premiums.push_back(std::make_pair(stock, prem));
		}

		delete myStdNDis;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::solve1DAnalytic : Stochastic parameters weren't set!");
	}
}

void BlackScholesSolver::print1DAnalytic(std::vector< std::pair<double, double> >& premiums, std::string tfilename)
{
	typedef std::vector< std::pair<double, double> > printVector;
	std::ofstream fileout;

	fileout.open(tfilename.c_str());
	for(printVector::iterator iter = premiums.begin(); iter != premiums.end(); iter++)
	{
		fileout << iter->first << " " << iter->second << " " << std::endl;
	}
	fileout.close();
}

std::vector<size_t> BlackScholesSolver::getAlgorithmicDimensions()
{
	return this->myGrid->getAlgorithmicDimensions();
}

void BlackScholesSolver::setAlgorithmicDimensions(std::vector<size_t> newAlgoDims)
{
	if (tOptionType == "all")
	{
		this->myGrid->setAlgorithmicDimensions(newAlgoDims);
	}
	else
	{
		throw new application_exception("BlackScholesSolver::setAlgorithmicDimensions : Set algorithmic dimensions is only supported when choosing option type all!");
	}
}

void BlackScholesSolver::initScreen()
{
	this->myScreen = new ScreenOutput();
	this->myScreen->writeTitle("SGpp - Black Scholes Solver, 2.0.0", "TUM (C) 2009-2010, by Alexander Heinecke");
	this->myScreen->writeStartSolve("Multidimensional Black Scholes Solver");
}

void BlackScholesSolver::setEnableCoarseningData(std::string adaptSolveMode, std::string refineMode, size_t refineMaxLevel, int numCoarsenPoints, double coarsenThreshold, double refineThreshold)
{
	this->useCoarsen = true;
	this->coarsenThreshold = coarsenThreshold;
	this->refineThreshold = refineThreshold;
	this->refineMaxLevel = refineMaxLevel;
	this->adaptSolveMode = adaptSolveMode;
	this->refineMode = refineMode;
	this->numCoarsenPoints = numCoarsenPoints;
}

void BlackScholesSolver::printPayoffInterpolationError2D(DataVector& alpha, std::string tFilename, size_t numTestpoints, double strike)
{
	if (this->useLogTransform == false)
	{
		if (this->bGridConstructed)
		{
			if (this->myGrid->getStorage()->getBoundingBox()->getDimensions() == 2)
			{
				if (numTestpoints < 2)
					numTestpoints = 2;

				double dInc = (2.0*strike)/static_cast<double>(numTestpoints-1);

				double dX = 0.0;
				double dY = 2*strike;

				std::ofstream file;
				file.open(tFilename.c_str());

				OperationEval* myEval = this->myGrid->createOperationEval();

				for (size_t i = 0; i < numTestpoints; i++)
				{
					std::vector<double> point;

					point.push_back(dX);
					point.push_back(dY);

					double result = myEval->eval(alpha, point);

					file << std::scientific << std::setprecision( 16 ) << dX << " " << dY << " " << result << std::endl;

					dX += dInc;
					dY -= dInc;
				}

				delete myEval;

				file.close();
			}
		}
		else
		{
			throw new application_exception("BlackScholesSolver::getPayoffInterpolationError : A grid wasn't constructed before!");
		}
	}
}

size_t BlackScholesSolver::getGridPointsAtMoney(std::string payoffType, double strike, double eps)
{
	size_t nPoints = 0;

	if (this->useLogTransform == false)
	{
		if (this->bGridConstructed)
		{
			for (size_t i = 0; i < this->myGrid->getStorage()->size(); i++)
			{
				bool isAtMoney = true;
				DataVector coords(this->dim);
				this->myGridStorage->get(i)->getCoordsBB(coords, *this->myBoundingBox);

				if (payoffType == "std_euro_call" || payoffType == "std_euro_put")
				{
					for (size_t d = 0; d < this->dim; d++)
					{
						if ( ((coords.sum()/static_cast<double>(this->dim)) < (strike-eps)) || ((coords.sum()/static_cast<double>(this->dim)) > (strike+eps)) )
						{
							isAtMoney = false;
						}

					}
				}
				else
				{
					throw new application_exception("BlackScholesSolver::getGridPointsAtMoney : An unknown payoff-type was specified!");
				}

				if (isAtMoney == true)
				{
					nPoints++;
				}
			}
		}
		else
		{
			throw new application_exception("BlackScholesSolver::getGridPointsAtMoney : A grid wasn't constructed before!");
		}
	}

	return nPoints;
}

void BlackScholesSolver::initCartesianGridWithPayoff(DataVector& alpha, double strike, std::string payoffType)
{
	double tmp;

	if (this->bGridConstructed)
	{
		for (size_t i = 0; i < this->myGrid->getStorage()->size(); i++)
		{
			std::string coords = this->myGridStorage->get(i)->getCoordsStringBB(*this->myBoundingBox);
			std::stringstream coordsStream(coords);
			double* dblFuncValues = new double[dim];

			for (size_t j = 0; j < this->dim; j++)
			{
				coordsStream >> tmp;

				dblFuncValues[j] = tmp;
			}

			if (payoffType == "std_euro_call")
			{
				tmp = 0.0;
				for (size_t j = 0; j < dim; j++)
				{
					tmp += dblFuncValues[j];
				}
				alpha[i] = std::max<double>(((tmp/static_cast<double>(dim))-strike), 0.0);
			}
			else if (payoffType == "std_euro_put")
			{
				tmp = 0.0;
				for (size_t j = 0; j < dim; j++)
				{
					tmp += dblFuncValues[j];
				}
				alpha[i] = std::max<double>(strike-((tmp/static_cast<double>(dim))), 0.0);
			}
			else
			{
				throw new application_exception("BlackScholesSolver::initCartesianGridWithPayoff : An unknown payoff-type was specified!");
			}

			delete[] dblFuncValues;
		}

		OperationHierarchisation* myHierarchisation = this->myGrid->createOperationHierarchisation();
		myHierarchisation->doHierarchisation(alpha);
		delete myHierarchisation;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::initCartesianGridWithPayoff : A grid wasn't constructed before!");
	}
}

void BlackScholesSolver::initLogTransformedGridWithPayoff(DataVector& alpha, double strike, std::string payoffType)
{
	double tmp;

	if (this->bGridConstructed)
	{
		for (size_t i = 0; i < this->myGrid->getStorage()->size(); i++)
		{
			std::string coords = this->myGridStorage->get(i)->getCoordsStringBB(*this->myBoundingBox);
			std::stringstream coordsStream(coords);
			double* dblFuncValues = new double[dim];

			for (size_t j = 0; j < this->dim; j++)
			{
				coordsStream >> tmp;

				dblFuncValues[j] = tmp;
			}

			if (payoffType == "std_euro_call")
			{
				tmp = 0.0;
				for (size_t j = 0; j < dim; j++)
				{
					tmp += exp(dblFuncValues[j]);
				}
				alpha[i] = std::max<double>(((tmp/static_cast<double>(dim))-strike), 0.0);
			}
			else if (payoffType == "std_euro_put")
			{
				tmp = 0.0;
				for (size_t j = 0; j < dim; j++)
				{
					tmp += exp(dblFuncValues[j]);
				}
				alpha[i] = std::max<double>(strike-((tmp/static_cast<double>(dim))), 0.0);
			}
			else
			{
				throw new application_exception("BlackScholesSolver::initLogTransformedGridWithPayoff : An unknown payoff-type was specified!");
			}

			delete[] dblFuncValues;
		}

		OperationHierarchisation* myHierarchisation = this->myGrid->createOperationHierarchisation();
		myHierarchisation->doHierarchisation(alpha);
		delete myHierarchisation;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::initLogTransformedGridWithPayoff : A grid wasn't constructed before!");
	}
}

size_t BlackScholesSolver::getNeededIterationsToSolve()
{
	return this->nNeededIterations;
}

double BlackScholesSolver::getNeededTimeToSolve()
{
	return this->dNeededTime;
}

size_t BlackScholesSolver::getStartInnerGridSize()
{
	return this->staInnerGridSize;
}

size_t BlackScholesSolver::getFinalInnerGridSize()
{
	return this->finInnerGridSize;
}

size_t BlackScholesSolver::getAverageInnerGridSize()
{
	return this->avgInnerGridSize;
}

void BlackScholesSolver::storeInnerMatrix(DataVector& alpha, std::string tFilename, double timestepsize)
{
	if (this->bGridConstructed)
	{
		OperationParabolicPDESolverSystemDirichlet* myBSSystem = new BlackScholesParabolicPDESolverSystemEuropean(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "ImEul", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
		SGppStopwatch* myStopwatch = new SGppStopwatch();

		std::string mtx = "";

		myStopwatch->start();
		std::cout << "Generating matrix in MatrixMarket format..." << std::endl;
		myBSSystem->getInnerMatrix(mtx);

		std::ofstream outfile(tFilename.c_str());
		outfile << mtx;
		outfile.close();
		std::cout << "Generating matrix in MatrixMarket format... DONE! (" << myStopwatch->stop() << " s)" << std::endl << std::endl << std::endl;

		delete myStopwatch;
		delete myBSSystem;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::storeInnerMatrix : A grid wasn't constructed before!");
	}
}

void BlackScholesSolver::storeInnerMatrixDiagonal(DataVector& alpha, std::string tFilename, double timestepsize)
{
	if (this->bGridConstructed)
	{
		OperationParabolicPDESolverSystemDirichlet* myBSSystem = new BlackScholesParabolicPDESolverSystemEuropean(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "ImEul", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
		SGppStopwatch* myStopwatch = new SGppStopwatch();

		std::string mtx = "";

		myStopwatch->start();
		std::cout << "Generating systemmatrix's diagonal in MatrixMarket format..." << std::endl;
		myBSSystem->getInnerMatrixDiagonal(mtx);

		std::ofstream outfile(tFilename.c_str());
		outfile << mtx;
		outfile.close();
		std::cout << "Generating systemmatrix's diagonal in MatrixMarket format... DONE! (" << myStopwatch->stop() << " s)" << std::endl << std::endl << std::endl;

		delete myStopwatch;
		delete myBSSystem;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::storeInnerMatrix : A grid wasn't constructed before!");
	}
}

void BlackScholesSolver::storeInnerMatrixDiagonalRowSum(DataVector& alpha, std::string tFilename, double timestepsize)
{
	if (this->bGridConstructed)
	{
		OperationParabolicPDESolverSystemDirichlet* myBSSystem = new BlackScholesParabolicPDESolverSystemEuropean(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "ImEul", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
		SGppStopwatch* myStopwatch = new SGppStopwatch();

		std::string mtx = "";

		myStopwatch->start();
		std::cout << "Generating systemmatrix rowsum as diagonal matrix in MatrixMarket format..." << std::endl;
		myBSSystem->getInnerMatrixDiagonalRowSum(mtx);

		std::ofstream outfile(tFilename.c_str());
		outfile << mtx;
		outfile.close();
		std::cout << "Generating systemmatrix rowsum as diagonal matrix in MatrixMarket format... DONE! (" << myStopwatch->stop() << " s)" << std::endl << std::endl << std::endl;

		delete myStopwatch;
		delete myBSSystem;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::storeInnerMatrix : A grid wasn't constructed before!");
	}
}

void BlackScholesSolver::storeInnerRHS(DataVector& alpha, std::string tFilename, double timestepsize)
{
	if (this->bGridConstructed)
	{
		OperationParabolicPDESolverSystemDirichlet* myBSSystem = new BlackScholesParabolicPDESolverSystemEuropean(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "ImEul", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
		SGppStopwatch* myStopwatch = new SGppStopwatch();

		myStopwatch->start();
		std::cout << "Exporting inner right-hand-side..." << std::endl;
		DataVector* rhs_inner = myBSSystem->generateRHS();

		size_t nCoefs = rhs_inner->getSize();
		std::ofstream outfile(tFilename.c_str());
		for (size_t i = 0; i < nCoefs; i++)
		{
			outfile << std::scientific << rhs_inner->get(i) << std::endl;
		}
		outfile.close();
		std::cout << "Exporting inner right-hand-side... DONE! (" << myStopwatch->stop() << " s)" << std::endl << std::endl << std::endl;

		delete myStopwatch;
		delete myBSSystem;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::storeInnerMatrix : A grid wasn't constructed before!");
	}
}

void BlackScholesSolver::storeInnerSolution(DataVector& alpha, size_t numTimesteps, double timestepsize, size_t maxCGIterations, double epsilonCG, std::string tFilename)
{
	if (this->bGridConstructed)
	{
		Euler* myEuler = new Euler("ImEul", numTimesteps, timestepsize, false, 0, myScreen);
		BiCGStab* myCG = new BiCGStab(maxCGIterations, epsilonCG);
		OperationParabolicPDESolverSystemDirichlet* myBSSystem = new BlackScholesParabolicPDESolverSystemEuropean(*this->myGrid, alpha, *this->mus, *this->sigmas, *this->rhos, this->r, timestepsize, "ImEul", this->useLogTransform, this->useCoarsen, this->coarsenThreshold, this->adaptSolveMode, this->numCoarsenPoints, this->refineThreshold, this->refineMode, this->refineMaxLevel);
		SGppStopwatch* myStopwatch = new SGppStopwatch();

		myStopwatch->start();
		std::cout << "Exporting inner solution..." << std::endl;
		myEuler->solve(*myCG, *myBSSystem, false);

		DataVector* alpha_solve = myBSSystem->getGridCoefficientsForCG();
		size_t nCoefs = alpha_solve->getSize();
		std::ofstream outfile(tFilename.c_str());
		for (size_t i = 0; i < nCoefs; i++)
		{
			outfile << std::scientific << alpha_solve->get(i) << std::endl;
		}
		outfile.close();

		std::cout << "Exporting inner solution... DONE!" << std::endl;

		delete myStopwatch;
		delete myBSSystem;
		delete myCG;
		delete myEuler;
	}
	else
	{
		throw new application_exception("BlackScholesSolver::solveImplicitEuler : A grid wasn't constructed before!");
	}

}

}