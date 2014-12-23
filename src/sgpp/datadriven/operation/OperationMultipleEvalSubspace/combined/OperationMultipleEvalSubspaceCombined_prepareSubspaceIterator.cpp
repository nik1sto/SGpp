#include "../../OperationMultipleEvalSubspace/combined/OperationMultipleEvalSubspaceCombined.hpp"

using namespace sg::base;

namespace sg {
namespace datadriven {

/*
 levels needs to have size of the number of subspaces, stores a level tuple for each subspace
 level level part of a level index tuple, needs to have size of grid
 index index part of a level index tuple, needs to have size of grid

 Important: In contrast to other eval preparation method, this creates the actual level, not 2**level
 */
void OperationMultipleEvalSubspaceCombined::prepareSubspaceIterator() {

    /////////////////////////////////////////////////////
    // extract the subspace and grid points
    // and put them in a map with flatLevel -> subspace
    /////////////////////////////////////////////////////

    DataVector level(this->dim);
    DataVector index(this->dim);
    DataVector maxIndex(this->dim);

    unsigned int curLevel;
    unsigned int curIndex;

    this->allSubspaceNodes.clear();
    this->subspaceCount = 0;
    this->maxLevel = 0;

    //calculate the maxLevel first - required for level vector flattening
    for (size_t gridIndex = 0; gridIndex < this->storage->size(); gridIndex++) {
        sg::base::GridIndex *point = this->storage->get(gridIndex);
        for (size_t d = 0; d < this->dim; d++) {
            point->get(d, curLevel, curIndex);
            if (curLevel > this->maxLevel) {
                this->maxLevel = curLevel;
            }
        }
    }

    //create a list of subspaces and setup the grid points - now we know which subspaces actually exist in the grid
    //create map of flatLevel -> subspaceIndex (for convenience operations, not for time-critical code)
    //also find out how many grid points the largest subspace contains
    this->maxGridPointsOnLevel = 0;
    for (size_t gridIndex = 0; gridIndex < this->storage->size(); gridIndex++) {
        sg::base::GridIndex *point = this->storage->get(gridIndex);

        for (size_t d = 0; d < this->dim; d++) {
            point->get(d, curLevel, curIndex);
            level.set(d, curLevel);
            index.set(d, curIndex);
            maxIndex.set(d, 1 << curLevel);
        }

        uint32_t flatLevel = OperationMultipleEvalSubspaceCombined::flattenLevel(this->dim, maxLevel, level);

        std::map<uint32_t, uint32_t>::iterator it = this->allLevelsIndexMap.find(flatLevel);
        if (it == this->allLevelsIndexMap.end()) {
            this->allLevelsIndexMap.insert(std::make_pair(flatLevel, this->subspaceCount));

            this->allSubspaceNodes.emplace_back(X86CombinedSubspaceNode(level, flatLevel, maxIndex, index));

            X86CombinedSubspaceNode &subspace = this->allSubspaceNodes[this->subspaceCount];
            if (subspace.gridPointsOnLevel > this->maxGridPointsOnLevel) {
                this->maxGridPointsOnLevel = subspace.gridPointsOnLevel;
            }
            this->subspaceCount += 1;
        } else {
            // add the current grid point to its subspace
            uint32_t subspaceIndex = it->second;
            X86CombinedSubspaceNode &subspace = this->allSubspaceNodes[subspaceIndex];
            subspace.addGridPoint(index);
        }
    }

    // sort the subspaces lexicographically to get efficient curve though the subspaces
    std::sort(this->allSubspaceNodes.begin(), this->allSubspaceNodes.end(), X86CombinedSubspaceNode::subspaceCompare);

    // - after sorting the allLevelsIndexMap indices have to be recomputed
    // - the grid points are "unpacked", they choose their representation depending on their configuration
    // - also collect some statistical information
    this->allLevelsIndexMap.clear();
    size_t totalRegularGridPoints = 0;
    size_t actualGridPoints = 0;
    size_t largestArraySubspace = 0;
    size_t largestListSubspace = 0;
    size_t numberOfListSubspaces = 0;
    size_t nonVirtualGridPoints = 0;
//    vector<size_t> preferedDimBuckets(this->dim, 0);

    for (size_t subspaceIndex = 0; subspaceIndex < this->subspaceCount; subspaceIndex++) {
        X86CombinedSubspaceNode &subspace = this->allSubspaceNodes[subspaceIndex];
        //select representation
        subspace.unpack();

        this->allLevelsIndexMap.insert(std::make_pair(subspace.flatLevel, subspaceIndex));

        //collect statistics
        totalRegularGridPoints += subspace.gridPointsOnLevel;
        nonVirtualGridPoints += subspace.existingGridPointsOnLevel;
        if (subspace.type == X86CombinedSubspaceNode::SubspaceType::LIST) {
            actualGridPoints += subspace.existingGridPointsOnLevel;
            numberOfListSubspaces += 1;
            if (subspace.existingGridPointsOnLevel > largestListSubspace) {
                largestListSubspace = subspace.existingGridPointsOnLevel;
            }
        } else {
            actualGridPoints += subspace.gridPointsOnLevel;
            if (subspace.gridPointsOnLevel > largestArraySubspace) {
                largestArraySubspace = subspace.gridPointsOnLevel;
            }
        }

//        //fill prefered dim buckets
//        for (size_t i = 0; i < this->dim; i++) {
//            if (subspace.level[i] > 1) {
//                preferedDimBuckets[i] += 1;
//            }
//        }

    }

//    cout << "prefered dims: " << endl;
//    for (size_t i = 0; i < this->dim; i++) {
//        cout << "dim " << i << " -> " << preferedDimBuckets[i] << endl;
//    }

    //cout << "maxGridPointsOnLevel: " << this->maxGridPointsOnLevel << endl;
    //cout << "no. of subspaces: " << subspaceCount << endl;
    //cout << "maxLevel: " << maxLevel << endl;

#ifdef X86COMBINED_WRITE_STATS
    // cout << "nonVirtualGridPoints: " << nonVirtualGridPoints << endl;
    // cout << "totalRegularGridPoints: " << totalRegularGridPoints << endl;
    // cout << "actualGridPoints: " << actualGridPoints << endl;
    // cout << "largestArraySubspace: " << largestArraySubspace << endl;
    // cout << "largestListSubspace: " << largestListSubspace << endl;
    // cout << "numberOfListSubspaces: " << numberOfListSubspaces << endl;
    // cout << "subspaceCount: " << subspaceCount << endl;
    // cout << "avr. points per subspace: " << ((double) nonVirtualGridPoints / (double) subspaceCount) << endl;

    this->statsFile << this->refinementStep << this->csvSep;
    this->statsFile << nonVirtualGridPoints << this->csvSep;
    this->statsFile << totalRegularGridPoints << this->csvSep;
    this->statsFile << actualGridPoints << this->csvSep;
    this->statsFile << largestArraySubspace << this->csvSep;
    this->statsFile << largestListSubspace << this->csvSep;
    this->statsFile << numberOfListSubspaces << this->csvSep;
    this->statsFile << subspaceCount << this->csvSep;
    this->statsFile << ((double) nonVirtualGridPoints / (double) subspaceCount) << this->csvSep;
    size_t numberOfThreads = omp_get_max_threads();
    this->statsFile << ((double) (this->maxGridPointsOnLevel * numberOfThreads + actualGridPoints) * 8.0) / (1024.0 * 1024.0) << this->csvSep;
    this->statsFile << (double) (this->maxGridPointsOnLevel * numberOfThreads + actualGridPoints) / nonVirtualGridPoints << endl;
    this->refinementStep += 1;
#endif

    //add padding subspace at the end
    this->allSubspaceNodes.emplace_back(X86CombinedSubspaceNode(this->dim, subspaceCount));

    //////////////////////////////////////
    // now the jump links can be created
    //////////////////////////////////////

    //marker for the padding subspace - traversal is finished when this subspace is reached
    uint32_t computationFinishedMarker = static_cast<uint32_t>(subspaceCount); //TODO ugly conversion

    //tracks at which position the next subspace can be found that has a higher value in at least the n-th component
    uint32_t *jumpIndexMap = new uint32_t[this->dim];
    for (size_t i = 0; i < this->dim; i++) {
        jumpIndexMap[i] = computationFinishedMarker;
    }

    for (size_t i = subspaceCount - 1; i > 0; i--) {
        X86CombinedSubspaceNode &currentNode = this->allSubspaceNodes[i];
        X86CombinedSubspaceNode &previousNode = this->allSubspaceNodes[i - 1];

        //number of dimension for which this subspace is responsible
        uint32_t recomputeComponentPreviousNode = static_cast<uint32_t>(X86CombinedSubspaceNode::compareLexicographically(currentNode,
                previousNode)); //TODO ugly conversion

        // which dimension have to be precomputed at the next subspace?
        currentNode.arriveDiff = recomputeComponentPreviousNode;

        //where to jump to?
        currentNode.jumpTargetIndex = jumpIndexMap[recomputeComponentPreviousNode];

        //update jump map
        //current node is a valid jump target for all predeccessors that change in one of the higher dims
        // sssssssssss <change> uuuuuuuu (s = same component, u = different component)
        // for predecessors that changed in the "u" array, jump to the current node
        // they also update the jump map
        for (size_t j = recomputeComponentPreviousNode + 1; j < dim; j++) {
            jumpIndexMap[j] = static_cast<uint32_t>(i); //TODO ugly conversion
        }

    }
    delete jumpIndexMap;

    //make sure that the tensor products are calculated entirely at the first subspace
    X86CombinedSubspaceNode &firstNode = this->allSubspaceNodes[0];
    firstNode.jumpTargetIndex = computationFinishedMarker;
    firstNode.arriveDiff = 0; //recompute all dimensions at the first subspace
}

}
}
