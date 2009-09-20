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

#include "tools/common/GridPrinter.hpp"
#include "operation/common/OperationEval.hpp"

#include <fstream>
#include <vector>

namespace sg
{

GridPrinter::GridPrinter(Grid& SparseGrid): myGrid(&SparseGrid)
{
}

GridPrinter::~GridPrinter()
{
}

void GridPrinter::printGrid(DataVector& alpha, std::string tFilename, double PointsPerDimension)
{
	DimensionBoundary dimOne;
	DimensionBoundary dimTwo;
	std::ofstream fileout;

	if (myGrid->getStorage()->size() > 0)
	{
		if (myGrid->getStorage()->dim() > 2)
		{
			// @todo (heinecke) thrown a tool exception
		}
		else
		{
			// Open filehandle
			fileout.open(tFilename.c_str());
			OperationEval* myEval = myGrid->createOperationEval();

			if (myGrid->getStorage()->dim() == 1)
			{
				dimOne = myGrid->getBoundingBox()->getBoundary(0);

				for (double i = dimOne.leftBoundary; i <= dimOne.rightBoundary; i+=((dimOne.rightBoundary - dimOne.leftBoundary)/PointsPerDimension))
				{
					std::vector<double> point;
					point.push_back(i);
					fileout << i << " " << myEval->eval(alpha,point) << std::endl;
				}
			}
			else if (myGrid->getStorage()->dim() == 2)
			{
				dimOne = myGrid->getBoundingBox()->getBoundary(0);
				dimTwo = myGrid->getBoundingBox()->getBoundary(1);

				for (double i = dimOne.leftBoundary; i <= dimOne.rightBoundary; i+=((dimOne.rightBoundary - dimOne.leftBoundary)/PointsPerDimension))
				{
					for (double j = dimTwo.leftBoundary; j <= dimTwo.rightBoundary; j+=((dimTwo.rightBoundary - dimTwo.leftBoundary)/PointsPerDimension))
					{
						std::vector<double> point;
						point.push_back(i);
						point.push_back(j);
						fileout << i << " " << j << " " << myEval->eval(alpha,point) << std::endl;
					}
					fileout << std::endl;
				}
			}
			else
			{
				// @todo (heinecke) thrown a tool exception
			}

			delete myEval;
			// close filehandle
			fileout.close();
		}
	}
	else
	{
		// @todo (heinecke) thrown a tool exception
	}
}

}
