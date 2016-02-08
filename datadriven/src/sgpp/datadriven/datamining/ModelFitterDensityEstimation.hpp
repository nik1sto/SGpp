// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org


#pragma once

#include <sgpp/globaldef.hpp>

#include <sgpp/datadriven/datamining/ModelFittingBase.hpp>
#include <sgpp/base/grid/Grid.hpp>
#include <sgpp/base/datatypes/DataVector.hpp>
#include <sgpp/base/datatypes/DataMatrix.hpp>


namespace SGPP {
  namespace datadriven {

    class ModelFitterDensityEstimation: public sg::datadriven::ModelFittingBase {
      public:
        ModelFitterDensityEstimation(SGPP::datadriven::DataMiningConfiguration config);
        virtual ~ModelFitterDensityEstimation();

        void fit() override;

      private:
        SGPP::datadriven::DataMiningConfigurationDensityEstimation config;
    };

  } /* namespace datadriven */
} /* namespace SGPP */
