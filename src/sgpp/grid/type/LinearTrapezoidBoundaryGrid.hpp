/*****************************************************************************/
/* This file is part of sgpp, a program package making use of spatially      */
/* adaptive sparse grids to solve numerical problems                         */
/*                                                                           */
/* Copyright (C) 2009 Alexander Heinecke (Alexander.Heinecke@mytum.de)       */
/*                                                                           */
/* sgpp is free software; you can redistribute it and/or modify              */
/* it under the terms of the GNU Lesser General Public License as published  */
/* by the Free Software Foundation; either version 3 of the License, or      */
/* (at your option) any later version.                                       */
/*                                                                           */
/* sgpp is distributed in the hope that it will be useful,                   */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU Lesser General Public License for more details.                       */
/*                                                                           */
/* You should have received a copy of the GNU Lesser General Public License  */
/* along with sgpp; if not, write to the Free Software                       */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/* or see <http://www.gnu.org/licenses/>.                                    */
/*****************************************************************************/

#ifndef LINEARTRAPEZOIDBOUNDARYGRID_HPP
#define LINEARTRAPEZOIDBOUNDARYGRID_HPP

#include "grid/Grid.hpp"

#include <iostream>

namespace sg
{

/**
 * grid with linear base functions with boundaries, pentagon cut
 */
class LinearTrapezoidBoundaryGrid : public Grid
{
protected:
	LinearTrapezoidBoundaryGrid(std::istream& istr);

public:
	/**
	 * Constructor Linear Trapezoid Boundary Grid
	 *
	 * @param dim the dimension of the grid
	 */
	LinearTrapezoidBoundaryGrid(size_t dim);

	/**
	 * Constructor Linear Trapezoid Boundary Grid
	 *
	 * @param BB the BoundingBox of the grid
	 */
	LinearTrapezoidBoundaryGrid(BoundingBox& BB);

	/**
	 * Destructor
	 */
	virtual ~LinearTrapezoidBoundaryGrid();

	virtual const char* getType();

	virtual OperationB* createOperationB();
	virtual GridGenerator* createGridGenerator();
	virtual OperationMatrix* createOperationLaplace();
	virtual OperationEval* createOperationEval();
	virtual OperationHierarchisation* createOperationHierarchisation();

	// @todo (heinecke) remove this when done
	virtual OperationMatrix* createOperationUpDownTest();

	// finance operations
	virtual OperationMatrix* createOperationDelta(DataVector& mu);
	virtual OperationMatrix* createOperationGammaPartOne(DataVector& sigma, DataVector& rho);
	virtual OperationMatrix* createOperationGammaPartTwo(DataVector& sigma, DataVector& rho);
	virtual OperationMatrix* createOperationGammaPartThree(DataVector& sigma, DataVector& rho);
	virtual OperationMatrix* createOperationRiskfreeRate();

	static Grid* unserialize(std::istream& istr);
};

}

#endif /* LINEARTRAPEZOIDBOUNDARYGRID_HPP */
