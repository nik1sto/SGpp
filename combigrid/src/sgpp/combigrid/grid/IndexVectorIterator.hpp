// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#pragma once

#include <sgpp/globaldef.hpp>
#include <sgpp/combigrid/LevelIndexTypes.hpp>
#include <sgpp/combigrid/grid/FullGrid.hpp>

#include <iterator>
#include <vector>

namespace sgpp {
namespace combigrid {

class IndexVectorIterator : public std::iterator<std::random_access_iterator_tag, IndexVector,
                                                 size_t, IndexVector*, IndexVector&> {
 public:
  inline IndexVectorIterator() : dim(0), indexVector(), minIndex(), maxIndex(),
      numberOfIndexVectors(0), sequenceIndex(0) {
  }

  inline explicit IndexVectorIterator(const FullGrid& grid) : dim(grid.getDimension()),
      indexVector(dim), minIndex(dim), maxIndex(dim),
      numberOfIndexVectors(grid.getNumberOfIndexVectors()), sequenceIndex(0) {
    // use move semantics as soon as DataVector supports it
    grid.getMinIndex(minIndex);
    grid.getMaxIndex(maxIndex);
  }

  inline IndexVectorIterator(const IndexVectorIterator&) = default;
  inline IndexVectorIterator& operator=(const IndexVectorIterator&) = default;

  static inline IndexVectorIterator begin(const FullGrid& grid) {
    return IndexVectorIterator(grid);
  }

  static inline IndexVectorIterator end(const FullGrid& grid) {
    IndexVectorIterator result(grid);
    result.setSequenceIndex(grid.getNumberOfIndexVectors());
    return result;
  }

  inline size_t getSequenceIndex() const {
    return sequenceIndex;
  }

  inline void setSequenceIndex(size_t sequenceIndex) {
    this->sequenceIndex = sequenceIndex;
  }

  inline IndexVector& operator*() {
    return operator[](sequenceIndex);
  }

  inline IndexVector* operator->() {
    return &operator[](sequenceIndex);
  }

  inline IndexVector& operator[](size_t rhs) {
    for (size_t d = 0; d < dim; d++) {
      indexVector[d] = minIndex[d] +
          static_cast<index_t>(rhs % (maxIndex[d] - minIndex[d] + 1));
      rhs /= (maxIndex[d] - minIndex[d] + 1);
    }

    return indexVector;
  }

  inline IndexVectorIterator& operator++() {
    sequenceIndex++;
    return *this;
  }

  inline IndexVectorIterator operator++(int) {
    IndexVectorIterator tmp(*this);
    sequenceIndex++;
    return tmp;
  }

  inline IndexVectorIterator& operator--() {
    sequenceIndex--;
    return *this;
  }

  inline IndexVectorIterator operator--(int) {
    IndexVectorIterator tmp(*this);
    sequenceIndex--;
    return tmp;
  }

  inline IndexVectorIterator operator+(size_t rhs) const {
    IndexVectorIterator tmp(*this);
    tmp.sequenceIndex += rhs;
    return tmp;
  }

  friend inline IndexVectorIterator operator+(size_t lhs, const IndexVectorIterator& rhs) {
    IndexVectorIterator tmp(rhs);
    tmp.sequenceIndex += lhs;
    return tmp;
  }

  inline IndexVectorIterator operator-(size_t rhs) const {
    IndexVectorIterator tmp(*this);
    tmp.sequenceIndex -= rhs;
    return tmp;
  }

  inline size_t operator-(const IndexVectorIterator& other) const {
    return sequenceIndex - other.sequenceIndex;
  }

  inline IndexVectorIterator& operator+=(size_t rhs) {
    sequenceIndex += rhs;
    return *this;
  }

  inline IndexVectorIterator& operator-=(size_t rhs) {
    sequenceIndex -= rhs;
    return *this;
  }

  inline bool operator==(const IndexVectorIterator& other) const {
    return (sequenceIndex == other.sequenceIndex);
  }

  inline bool operator!=(const IndexVectorIterator& other) const {
    return (sequenceIndex != other.sequenceIndex);
  }

  inline bool operator<(const IndexVectorIterator& other) const {
    return (sequenceIndex < other.sequenceIndex);
  }

  inline bool operator>(const IndexVectorIterator& other) const {
    return (sequenceIndex > other.sequenceIndex);
  }

  inline bool operator<=(const IndexVectorIterator& other) const {
    return (sequenceIndex <= other.sequenceIndex);
  }

  inline bool operator>=(const IndexVectorIterator& other) const {
    return (sequenceIndex >= other.sequenceIndex);
  }

 protected:
  size_t dim;
  IndexVector indexVector;
  IndexVector minIndex;
  IndexVector maxIndex;
  const size_t numberOfIndexVectors;
  size_t sequenceIndex;
};

}  // namespace combigrid
}  // namespace sgpp