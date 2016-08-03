// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#pragma once

#include <memory>
#include <mutex>

namespace sgpp {
namespace combigrid {

class PtrGuard {
  std::shared_ptr<std::mutex> mutexPtr;

 public:
  explicit PtrGuard(std::shared_ptr<std::mutex> mutexPtr) : mutexPtr(mutexPtr) {
    if (mutexPtr) {
      mutexPtr->lock();
    }
  }

  ~PtrGuard() {
    if (mutexPtr) {
      mutexPtr->unlock();
    }
  }
};

} /* namespace combigrid */
} /* namespace sgpp */
