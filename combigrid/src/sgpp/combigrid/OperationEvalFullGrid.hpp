// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#pragma once

#include <sgpp/globaldef.hpp>
#include <sgpp/base/datatypes/DataVector.hpp>
#include <sgpp/base/operation/hash/OperationEval.hpp>
#include <sgpp/combigrid/FullGrid.hpp>
#include <sgpp/combigrid/IndexVectorRange.hpp>

namespace sgpp {
namespace combigrid {

class OperationEvalFullGrid : public base::OperationEval {
 public:
  OperationEvalFullGrid() : grid() {
  }

  explicit OperationEvalFullGrid(const FullGrid& grid) : grid(grid) {
  }

  ~OperationEvalFullGrid() override {
  }

  double eval(const base::DataVector& surpluses, const base::DataVector& point) override {
    const LevelVector& level = grid.getLevel();
    const HeterogeneousBasis& basis = grid.getBasis();
    size_t i = 0;
    double result = 0.0;

    for (const IndexVector& index : IndexVectorRange(grid)) {
      result += surpluses[i] * basis.eval(level, index, point);
      i++;
    }

    return result;
  }

  const FullGrid& getGrid() const {
    return grid;
  }

  void setGrid(const FullGrid& grid) {
    this->grid = grid;
  }

 protected:
  FullGrid grid;
};

}  // namespace combigrid
}  // namespace sgpp