// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#ifndef SGPP_OPTIMIZATION_FUNCTION_TEST_SHCB_HPP
#define SGPP_OPTIMIZATION_FUNCTION_TEST_SHCB_HPP

#include <sgpp/globaldef.hpp>

#include <sgpp/optimization/function/test/TestFunction.hpp>

namespace SGPP {
  namespace optimization {
    namespace test_functions {

      /**
       * SHCB test function.
       *
       * Definition:
       * \f$f(\vec{x}) := x_1^2 \left(4 - 2.1 x_1^2 + x_1^4/3\right) + x_1 x_2
       *                + 4 x_2^2 \left(x_2^2 - 1\right)\f$,
       * \f$\vec{x} \in [-5, 5]^2\f$,
       * \f$\vec{x}_{\text{opt}} \in \{(0.0898, -0.7127)^{\mathrm{T}},
       *                               (-0.0898, 0.7127)^{\mathrm{T}}\}\f$,
       * \f$f_{\text{opt}} = -1.031628\f$
       * (domain scaled to \f$[0, 1]^2\f$)
       */
      class SHCB : public TestFunction {
        public:
          /**
           * Constructor.
           */
          SHCB() : TestFunction(2) {
          }

          /**
           * Evaluates the test function.
           *
           * @param x     point \f$\vec{x} \in [0, 1]^2\f$
           * @return      \f$f(\vec{x})\f$
           */
          float_t evalUndisplaced(const base::DataVector& x) {
            const float_t x1 = 10.0 * x.get(0) - 5.0;
            const float_t x2 = 10.0 * x.get(1) - 5.0;

            return x1 * x1 * (4.0 - 2.1 * x1 * x1 + x1 * x1 * x1 * x1 / 3.0) +
                   x1 * x2 + 4.0 * x2 * x2 * (x2 * x2 - 1.0);
          }

          /**
           * Returns minimal point and function value of the test function.
           *
           * @param[out] x minimal point
           *               \f$\vec{x}_{\text{opt}} \in [0, 1]^2\f$
           * @return       minimal function value
           *               \f$f_{\text{opt}} = f(\vec{x}_{\text{opt}})\f$
           */
          float_t getOptimalPointUndisplaced(base::DataVector& x) {
            x.resize(2);
            x[0] = 0.50898420131003180624224905;
            x[1] = 0.42873435969792603666027341858;
            return evalUndisplaced(x);
          }

          /**
           * @param[out] clone pointer to cloned object
           */
          virtual void clone(std::unique_ptr<ObjectiveFunction>& clone) const {
            clone = std::unique_ptr<ObjectiveFunction>(new SHCB(*this));
          }
      };

    }
  }
}

#endif /* SGPP_OPTIMIZATION_FUNCTION_TEST_SHCB_HPP */