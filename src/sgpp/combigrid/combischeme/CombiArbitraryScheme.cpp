/*
 * combigrid::CombiArbitraryScheme.cpp
 *
 *  Created on: Apr 28, 2011
 *      Author: kowitz_local
 */

#include "CombiArbitraryScheme.hpp"

combigrid::CombiArbitraryScheme::CombiArbitraryScheme(std::vector<std::vector<
		int> > level_vectors) :
	combigrid::CombiSchemeBasis(level_vectors[0].size(), level_vectors[0]) {
	combigrid::CombigridLevelVector buffer =
			combigrid::CombigridLevelVector::getCombiLevels(level_vectors);
	levels_vector_ = buffer.getLevelVec();
	cofficients_ = buffer.getCoef();
}

combigrid::CombiArbitraryScheme::CombiArbitraryScheme(
		combigrid::CombigridLevelVector in) :
	combigrid::CombiSchemeBasis(in.getDim(), in.getLevelVec()[0]) {
	levels_vector_ = in.getLevelVec();
	cofficients_ = in.getCoef();
}