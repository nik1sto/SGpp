// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#pragma once

#include <sgpp/globaldef.hpp>
#include <sgpp/base/datatypes/DataVector.hpp>
#include <sgpp/combigrid/FullGrid.hpp>
#include <sgpp/combigrid/LevelIndexTypes.hpp>
#include <sgpp/combigrid/IndexVectorRange.hpp>
#include <sgpp/combigrid/OperationPole.hpp>

#include <memory>
#include <vector>

namespace sgpp {
namespace combigrid {

class OperationUPFullGrid {
 public:
  OperationUPFullGrid(const FullGrid& grid,
      const std::vector<std::unique_ptr<OperationPole>>& operationPole) :
      grid(grid), operationPole() {
    for (const std::unique_ptr<OperationPole>& operationPole1d : operationPole) {
      this->operationPole.push_back(operationPole1d.get());
    }
  }

  OperationUPFullGrid(const FullGrid& grid, const std::vector<OperationPole*> operationPole) :
      grid(grid), operationPole(operationPole) {
  }

  OperationUPFullGrid(const FullGrid& grid, OperationPole& operationPole) :
      grid(grid), operationPole(grid.getDimension(), &operationPole) {
  }

  virtual ~OperationUPFullGrid() {
  }

  virtual void apply(base::DataVector& values) {
    const size_t dim = grid.getDimension();
    const bool hasBoundary = grid.hasBoundary();
    FullGrid gridProjection(LevelVector(dim-1, 0), grid.getBasis(), grid.hasBoundary());
    LevelVector& levelProjection = gridProjection.getLevel();
    IndexVectorRange range(grid);
    IndexVector indexStart(dim);
    size_t step = 1;

    for (size_t d = 0; d < dim; d++) {
      const LevelVector& level = grid.getLevel();

      for (size_t d2 = 0; d2 < dim; d2++) {
        if (d2 < d) {
          levelProjection[d2] = level[d2];
        } else if (d2 > d) {
          levelProjection[d2-1] = level[d2];
        }
      }

      range.setGrid(grid);
      const index_t count = grid.getNumberOfIndexVectors(d);
      IndexVectorRange rangeProjection(gridProjection);
      indexStart[d] = grid.getMinIndex(d);

      for (const IndexVector& indexProjection : rangeProjection) {
        for (size_t d2 = 0; d2 < dim; d2++) {
          if (d2 < d) {
            indexStart[d2] = indexProjection[d2];
        } else if (d2 > d) {
            indexStart[d2] = indexProjection[d2-1];
          }
        }

        operationPole[d]->apply(values, range.find(indexStart), step, count, level[d], hasBoundary);
      }

      step *= count;
    }
  }

  const FullGrid& getGrid() const {
    return grid;
  }

  void setGrid(const FullGrid& grid) {
    this->grid = grid;
  }

  const std::vector<OperationPole*>& getOperationPole() const {
    return operationPole;
  }

  void setOperationPole(const std::vector<OperationPole*>& operationPole) {
    this->operationPole = operationPole;
  }

 protected:
  FullGrid grid;
  std::vector<OperationPole*> operationPole;
};

}  // namespace combigrid
}  // namespace sgpp