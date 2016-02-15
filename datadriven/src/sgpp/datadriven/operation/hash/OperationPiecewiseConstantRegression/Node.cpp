// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#include <algorithm>
#include <vector>

#include "sgpp/datadriven/operation/hash/OperationPiecewiseConstantRegression/Node.hpp"

namespace SGPP {
namespace datadriven {
namespace PiecewiseConstantRegression {

uint64_t Node::integratedNodes;
uint64_t Node::hierarchizeMaxLevel;

Node::Node(std::vector<float_t> x, std::vector<float_t> h, std::vector<size_t> supportIndizes,
           base::DataMatrix& dataset, base::DataVector& values, bool verbose)
    : x(x),
      h(h),
      dim(x.size()),
      supportIndizes(supportIndizes),
      leftChild(nullptr),
      rightChild(nullptr),
      childDim(0),
      surplus(0.0),
      dataset(dataset),
      values(values),
      childCount(0),
      verbose(verbose) {}

std::vector<size_t> Node::getSupportIndizes(std::vector<float_t>& x, std::vector<float_t>& h,
                                            std::vector<size_t>& parentSupport) {
  std::vector<size_t> supportIndizes;

  for (size_t i = 0; i < parentSupport.size(); i++) {
    size_t dataIndex = parentSupport[i];

    base::DataVector point(dim);
    dataset.getRow(dataIndex, point);
    bool onSupport = true;

    for (size_t d = 0; d < dim; d++) {
      if (point[d] < (x[d] - h[d]) || point[d] > (x[d] + h[d])) {
        onSupport = false;
        break;
      }
    }

    if (onSupport) {
      supportIndizes.push_back(dataIndex);
    }
  }

  return supportIndizes;
}

float_t Node::getAverage(std::vector<size_t>& supportIndizes, std::vector<float_t>& x,
                         std::vector<float_t>& h) {
  float_t sum = 0.0;
  size_t pointOnSupport = 0;

  for (size_t i = 0; i < supportIndizes.size(); i++) {
    size_t dataIndex = supportIndizes[i];
    sum += values[dataIndex];
    pointOnSupport += 1;
  }

  float_t average;

  if (pointOnSupport > 0) {
    average = sum / static_cast<float_t>(pointOnSupport);
  } else {
    average = 0.0;
  }

  return average;
}

float_t Node::getMSE(std::vector<size_t>& supportIndizes, std::vector<float_t>& x,
                     std::vector<float_t>& h, float_t supportValue) {
  float_t sum = 0.0;
  size_t pointOnSupport = 0;

  for (size_t i = 0; i < supportIndizes.size(); i++) {
    size_t dataIndex = supportIndizes[i];

    float_t diff = supportValue - values[dataIndex];
    sum += diff * diff;
    pointOnSupport += 1;
  }

  float_t mse;

  if (pointOnSupport > 0) {
    mse = sum / static_cast<float_t>(pointOnSupport);
  } else {
    mse = 0.0;
  }

  return mse;
}

std::unique_ptr<Node> Node::hierarchizeChild(std::vector<float_t>& x, std::vector<float_t>& h,
                                             std::vector<size_t>& parentSupport,
                                             float_t supportValue, float_t targetMSE,
                                             size_t targetMaxLevel, size_t nextDim,
                                             size_t levelLimit) {
  std::vector<size_t> support = getSupportIndizes(x, h, parentSupport);

  if (support.size() == 0) {
    if (verbose) {
      std::cout << "reached 0-points: " << std::endl;
    }

    return std::unique_ptr<Node>(nullptr);
  }

  float_t mse = getMSE(support, x, h, supportValue);

  if (mse <= targetMSE) {
    if (verbose) {
      std::cout << "reached MSE: " << targetMSE << std::endl;
    }

    return std::unique_ptr<Node>(nullptr);
  }

  std::unique_ptr<Node> childNode = std::make_unique<Node>(x, h, support, dataset, values);
  childNode->hierarchize(targetMSE, targetMaxLevel, supportValue, nextDim, levelLimit);

  return std::move(childNode);
}

void Node::hierarchize(float_t targetMSE, size_t targetMaxLevel, float_t parentValue,
                       size_t refineDim, size_t levelLimit) {
  if (levelLimit > targetMaxLevel) {
    return;
  }

  if (levelLimit > hierarchizeMaxLevel) {
    Node::hierarchizeMaxLevel = levelLimit;
  }

  surplus = getAverage(supportIndizes, x, h) - parentValue;
  float_t supportValue = parentValue + surplus;

  //        for (size_t d = refineDim; d < dim; d++) {

  bool refinedAnyChild = false;

  std::vector<float_t> childH = getChildH(refineDim);

  std::vector<float_t> leftChildX = getLeftChildX(refineDim);
  leftChild = hierarchizeChild(leftChildX, childH, supportIndizes, supportValue, targetMSE,
                               targetMaxLevel, (refineDim + 1) % dim, levelLimit + 1);

  if (leftChild.operator bool()) {
    refinedAnyChild = true;
    childCount += 1 + leftChild->childCount;
  }

  std::vector<float_t> rightChildX = getRightChildX(refineDim);
  rightChild = hierarchizeChild(rightChildX, childH, supportIndizes, supportValue, targetMSE,
                                targetMaxLevel, (refineDim + 1) % dim, levelLimit + 1);

  if (rightChild.operator bool()) {
    refinedAnyChild = true;
    childCount += 1 + rightChild->childCount;
  }

  if (refinedAnyChild) {
    this->childDim = refineDim;
    //                break;
  }

  //        }
}

std::vector<float_t> Node::getLeftChildX(size_t d) {
  std::vector<float_t> childX(x);
  childX[d] -= (h[d] / 2.0);
  return childX;
}

std::vector<float_t> Node::getRightChildX(size_t d) {
  std::vector<float_t> childX(x);
  childX[d] += (h[d] / 2.0);
  return childX;
}

std::vector<float_t> Node::getChildH(size_t d) {
  std::vector<float_t> childH(h);
  childH[d] = h[d] / 2.0;
  return childH;
}

float_t Node::evaluate(std::vector<float_t>& point) {
  float_t sum = 0.0;
  //        std::cout << "evaluate at x:";
  //        for(size_t d = 0; d < dim; d++) {
  //            if (d > 0) {
  //                std::cout << ", ";
  //            }
  //            std::cout << x[d];
  //        }
  //        std::cout << " h: ";
  //        for(size_t d = 0; d < dim; d++) {
  //            if (d > 0) {
  //                std::cout << ", ";
  //            }
  //            std::cout << h[d];
  //        }
  //        std::cout << " surplus: " << surplus;
  //        std::cout << std::endl;

  sum += surplus;

  if (point[this->childDim] < x[this->childDim]) {
    if (this->leftChild.operator bool()) {
      sum += this->leftChild->evaluate(point);
    }
  } else {
    if (this->rightChild.operator bool()) {
      sum += this->rightChild->evaluate(point);
    }
  }

  return sum;
}

float_t Node::integrate(SGPP::base::GridIndex& gridPoint, size_t& integratedNodes,
                        size_t levelLimit) {
  if (levelLimit == 0) {
    integratedNodes = 1;
  } else {
    integratedNodes += 1;
  }

  //        if (levelLimit > 2) {
  //            return 0.0;
  //        }
  SGPP::base::SLinearBase basis;

  float_t sum = 0.0;

  //        if (gridPoint.getLevel(0) == LEVEL_TO_PRINT and gridPoint.getIndex(0) == INDEX_TO_PRINT)
  //        {
  //            std::cout << "x: ";
  //            for (size_t d = 0; d < dim; d++) {
  //                if (d > 0) {
  //                    std::cout << ", ";
  //                }
  //                std::cout << x[d];
  //            }
  //
  //            std::cout << " h: ";
  //            for (size_t d = 0; d < dim; d++) {
  //                if (d > 0) {
  //                    std::cout << ", ";
  //                }
  //                std::cout << h[d];
  //            }
  //            std::cout << std::endl;
  //            std::cout << "surplus: " << surplus << std::endl;
  //        }

  float_t integral = 1.0;

  for (size_t d = 0; d < dim; d++) {
    // integrate left side of triangle
    float_t integral1D = 0.0;

    float_t gridPointHat = gridPoint.getCoord(d);

    float_t gridPointH = (1.0 / static_cast<float_t>(1 << gridPoint.getLevel(d)));

    float_t leftGridPointHat = gridPointHat - gridPointH;
    float_t rightGridPointHat = gridPointHat + gridPointH;

    float_t leftGridPointConstant = x[d] - h[d];
    float_t rightGridPointConstant = x[d] + h[d];

    float_t leftSideLeftBorder = std::max(leftGridPointHat, leftGridPointConstant);
    float_t leftSideRightBorder = std::min(gridPointHat, rightGridPointConstant);

    if (leftSideRightBorder > leftSideLeftBorder) {
      float_t leftIntegral1D = (leftSideRightBorder - leftSideLeftBorder) *
                               basis.eval(gridPoint.getLevel(d), gridPoint.getIndex(d),
                                          (leftSideRightBorder + leftSideLeftBorder) / 2.0);
      integral1D += leftIntegral1D;
    }

    float_t rightSideLeftBorder = std::max(gridPointHat, leftGridPointConstant);
    float_t rightSideRightBorder = std::min(rightGridPointHat, rightGridPointConstant);

    if (rightSideRightBorder > rightSideLeftBorder) {
      float_t rightIntegral1D = (rightSideRightBorder - rightSideLeftBorder) *
                                basis.eval(gridPoint.getLevel(d), gridPoint.getIndex(d),
                                           (rightSideRightBorder + rightSideLeftBorder) / 2.0);
      integral1D += rightIntegral1D;
    }

    integral *= integral1D;
  }

  float_t product = surplus * integral;

  if (integral > 0.0) {
    if (leftChild.operator bool()) {
      sum += leftChild->integrate(gridPoint, integratedNodes, levelLimit + 1);
    }

    if (rightChild.operator bool()) {
      sum += rightChild->integrate(gridPoint, integratedNodes, levelLimit + 1);
    }
  }

  return sum + product;
}

uint64_t Node::getChildCount() { return childCount; }

uint64_t Node::getHierarchizationMaxLevel() { return hierarchizeMaxLevel; }
}  // namespace PiecewiseConstantRegression
}  // namespace datadriven
}  // namespace SGPP