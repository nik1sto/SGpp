/*
 * test_serialization.cpp
 *
 *  Created on: 26.01.2016
 *      Author: david
 */

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <sgpp/globaldef.hpp>
#include <sgpp/combigrid/storage/tree/TreeStorage.hpp>
#include <sgpp/combigrid/serialization/FloatSerializationStrategy.hpp>
#include "../src/sgpp/combigrid/serialization/TreeStorageSerializationStrategy.hpp"
#include <sgpp/combigrid/storage/FunctionLookupTable.hpp>
#include <sgpp/combigrid/storage/tree/CombigridTreeStorage.hpp>
#include <sgpp/combigrid/grid/hierarchy/NonNestedPointHierarchy.hpp>
#include <sgpp/combigrid/grid/ordering/ExponentialLevelorderPointOrdering.hpp>
#include <sgpp/combigrid/grid/distribution/ClenshawCurtisDistribution.hpp>
#include <cmath>
#include <limits>
#include <iostream>

using namespace SGPP::combigrid;
using namespace SGPP;

SGPP::float_t testFunc1(SGPP::base::DataVector const &x) {
	return sqrt(x[0]) * exp(x[1]);
}

SGPP::float_t testFunc2(SGPP::base::DataVector const &x) {
	return -1.0;
}

void checkDoubleSerialization(double x) {
	FloatSerializationStrategy<double> strategy;
	double converted = strategy.deserialize(strategy.serialize(x));
	if (std::isnan(x))
		BOOST_CHECK(std::isnan(converted));
	else
		BOOST_CHECK_EQUAL(x, converted);
}

BOOST_AUTO_TEST_CASE(testFloatSerialization) {
	checkDoubleSerialization(sqrt(2));
	checkDoubleSerialization(-sqrt(2));
	checkDoubleSerialization(1.0);
	checkDoubleSerialization(0.0000000001234567891);
	checkDoubleSerialization(0.0);
	checkDoubleSerialization(1000000.000000001);
	checkDoubleSerialization(pow(10000000.0, 1000000000000.0));
	checkDoubleSerialization(std::numeric_limits<double>::quiet_NaN());
	checkDoubleSerialization(-std::numeric_limits<double>::quiet_NaN());
	checkDoubleSerialization(std::numeric_limits<double>::infinity());
	checkDoubleSerialization(-std::numeric_limits<double>::infinity());
	checkDoubleSerialization(1.0 / 0.0);
	checkDoubleSerialization(sqrt(-1.0));
}

BOOST_AUTO_TEST_CASE(testFunctionLookupTableSerialization) {
	auto func = testFunc1;
	FunctionLookupTable table((MultiFunction(func)));

	SGPP::base::DataVector vec(2);
	vec[0] = 12345.0;
	vec[1] = 1.0;
	BOOST_CHECK_EQUAL(func(vec), table(vec));
	vec[0] = 0.0;
	BOOST_CHECK_EQUAL(func(vec), table(vec));
	vec[0] = 1.0;
	BOOST_CHECK_EQUAL(func(vec), table(vec));
	vec[0] = 2.0;
	BOOST_CHECK_EQUAL(func(vec), table(vec));

	std::string str = table.serialize();

	FunctionLookupTable table2(MultiFunction([](SGPP::base::DataVector const &x) {return -1.0;}));
	table2.deserialize(str);

	vec[0] = 12345.0;
	vec[1] = 1.0;
	BOOST_CHECK_EQUAL(func(vec), table2(vec));
	vec[0] = 0.0;
	BOOST_CHECK_EQUAL(func(vec), table2(vec));
	vec[0] = 1.0;
	BOOST_CHECK_EQUAL(func(vec), table2(vec));
	vec[0] = 2.0;
	BOOST_CHECK_EQUAL(func(vec), table2(vec));
}

BOOST_AUTO_TEST_CASE(testCombigridTreeStorageSerialization) {
	std::vector<std::shared_ptr<AbstractPointHierarchy>> hierarchies(2,
			std::make_shared<NonNestedPointHierarchy>(std::make_shared<ClenshawCurtisDistribution>(),
					std::make_shared<ExponentialLevelorderPointOrdering>()));

	CombigridTreeStorage storage(hierarchies, testFunc1);

	MultiIndex level(2, 2);
	MultiIndex bounds(2, 5);
	MultiIndexIterator mIt(bounds);
	std::vector<bool> orderingConfiguration(2, false);

	double sum = 0.0; // prevent optimizing away

	for (auto it = storage.getGuidedIterator(level, mIt, orderingConfiguration); it->isValid(); it->moveToNext()) {
		sum += it->value();
	}

	std::string str = storage.serialize();

	CombigridTreeStorage otherStorage(hierarchies, testFunc2);

	otherStorage.deserialize(str);

	mIt.reset();
	MultiIndexIterator otherMIt(bounds);

	auto otherIt = otherStorage.getGuidedIterator(level, otherMIt, orderingConfiguration);
	for (auto it = storage.getGuidedIterator(level, mIt, orderingConfiguration); it->isValid(); it->moveToNext(), otherIt->moveToNext()) {
		BOOST_CHECK(otherIt->isValid());
		BOOST_CHECK_EQUAL(it->value(), otherIt->value());
	}
	BOOST_CHECK(!otherIt->isValid());

	otherMIt.reset();
	level[0] = 5;
	BOOST_CHECK_EQUAL(otherStorage.getGuidedIterator(level, otherMIt, orderingConfiguration)->value(), -1.0);

	if(sum == 0.0) {
		std::cout << "\n"; // prevent optimizing away
	}
}
