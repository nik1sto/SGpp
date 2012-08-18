/******************************************************************************
* Copyright (C) 2010 Technische Universitaet Muenchen                         *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
******************************************************************************/
// @author Alexander Heinecke (Alexander.Heinecke@mytum.de)
// @author Roman Karlstetter (karlstetter@mytum.de)

#include "parallel/tools/MPI/SGppMPITools.hpp"
#include "base/exception/operation_exception.hpp"

#include "parallel/datadriven/tools/DMVectorizationPaddingAssistant.hpp"
#include "parallel/datadriven/basis/linear/noboundary/operation/impl/ChunkSizes.h"
#include "parallel/datadriven/basis/linear/noboundary/operation/impl/X86SimdLinearMult.h"
#include "parallel/datadriven/basis/linear/noboundary/operation/impl/X86SimdLinearMultTranspose.h"
#include "parallel/datadriven/algorithm/DMSystemMatrixVectorizedIdentityAsyncMPI.hpp"
#include "parallel/operation/ParallelOpFactory.hpp"
#include "parallel/tools/PartitioningTool.hpp"
#include <omp.h>

namespace sg
{
namespace parallel
{

DMSystemMatrixVectorizedIdentityAsyncMPI::DMSystemMatrixVectorizedIdentityAsyncMPI(sg::base::Grid& SparseGrid, sg::base::DataMatrix& trainData, double lambda, VectorizationType vecMode)
	: DMSystemMatrixBase(trainData, lambda), vecMode_(vecMode), numTrainingInstances_(0), numPatchedTrainingInstances_(0), m_grid(SparseGrid)
{
	// handle unsupported vector extensions
	if (this->vecMode_ != X86SIMD &&
			this->vecMode_ != MIC &&
			this->vecMode_ != Hybrid_X86SIMD_MIC &&
			this->vecMode_ != OpenCL &&
			this->vecMode_ != ArBB &&
			this->vecMode_ != Hybrid_X86SIMD_OpenCL)
	{
		throw new sg::base::operation_exception("DMSystemMatrixSPVectorizedIdentity : un-supported vector extension!");
	}

	this->dataset_ = new sg::base::DataMatrix(trainData);
	this->numTrainingInstances_ = this->dataset_->getNrows();
	this->numPatchedTrainingInstances_ = sg::parallel::DMVectorizationPaddingAssistant::padDataset(*(this->dataset_), vecMode_);

	if (this->vecMode_ != OpenCL && this->vecMode_ != ArBB && this->vecMode_ != Hybrid_X86SIMD_OpenCL)
	{
		this->dataset_->transpose();
	}

	int mpi_size = sg::parallel::myGlobalMPIComm->getNumRanks();
	int thread_count = 1;
#ifdef _OPENMP
	#pragma omp parallel
	{
		thread_count = omp_get_num_threads();
	}
#endif

	size_t approximateChunkSizeData = 100; // approximately how many blocks should be computed before sending
	size_t blockCountData = this->numPatchedTrainingInstances_/sg::parallel::DMVectorizationPaddingAssistant::getVecWidth(this->vecMode_); // this process has (in total) blockCountData blocks of Data to process
	_chunkCountData = blockCountData/approximateChunkSizeData;

	// arrays for distribution settings
	_mpi_data_sizes = new int[_chunkCountData];
	_mpi_data_offsets = new int[_chunkCountData];
	_mpi_data_sizes_global = new int[mpi_size];
	_mpi_data_offsets_global = new int[mpi_size];

	calcDistribution(this->numPatchedTrainingInstances_, _chunkCountData, _mpi_data_sizes, _mpi_data_offsets, sg::parallel::DMVectorizationPaddingAssistant::getVecWidth(this->vecMode_));
	calcDistribution(_chunkCountData, mpi_size, _mpi_data_sizes_global, _mpi_data_offsets_global, 1);



	size_t approximateChunkSizeGrid = 50; // approximately how many blocks should be computed before sending
	_chunkCountGrid = m_grid.getStorage()->size()/approximateChunkSizeGrid;

	// arrays for distribution settings
	_mpi_grid_sizes = new int[_chunkCountGrid];
	_mpi_grid_offsets = new int[_chunkCountGrid];
	_mpi_grid_sizes_global = new int[mpi_size];
	_mpi_grid_offsets_global = new int[mpi_size];

	calcDistribution(m_grid.getStorage()->size(), _chunkCountGrid, _mpi_grid_sizes, _mpi_grid_offsets, 1);
	calcDistribution(_chunkCountGrid, mpi_size, _mpi_grid_sizes_global, _mpi_grid_offsets_global, 1);


	this->level_ = new sg::base::DataMatrix(m_grid.getStorage()->size(), m_grid.getStorage()->dim());
	this->index_ = new sg::base::DataMatrix(m_grid.getStorage()->size(), m_grid.getStorage()->dim());

	m_grid.getStorage()->getLevelIndexArraysForEval(*(this->level_), *(this->index_));

	// mult: distribute calculations over dataset
	// multTranspose: distribute calculations over grid
}

DMSystemMatrixVectorizedIdentityAsyncMPI::~DMSystemMatrixVectorizedIdentityAsyncMPI()
{
	delete this->dataset_;

	delete this->level_;
	delete this->index_;

	delete[] this->_mpi_grid_sizes;
	delete[] this->_mpi_grid_offsets;
	delete[] this->_mpi_data_sizes;
	delete[] this->_mpi_data_offsets;

	delete[] this->_mpi_grid_sizes_global;
	delete[] this->_mpi_grid_offsets_global;
	delete[] this->_mpi_data_sizes_global;
	delete[] this->_mpi_data_offsets_global;
}

void DMSystemMatrixVectorizedIdentityAsyncMPI::mult(sg::base::DataVector& alpha, sg::base::DataVector& result)
{
	sg::base::DataVector temp(this->numPatchedTrainingInstances_);
	result.setAll(0.0);
	double* ptrResult = result.getPointer();
	double* ptrTemp = temp.getPointer();

	int mpi_size = sg::parallel::myGlobalMPIComm->getNumRanks();
	int mpi_myrank = sg::parallel::myGlobalMPIComm->getMyRank();


	MPI_Request dataRecvReqs[_chunkCountData]; //allocating a little more than necessary, otherwise complicated index computations needed
	int tagsData[_chunkCountData];
	for (int i = 0; i<_chunkCountData; i++){
		tagsData[i] = _mpi_data_offsets[i]*2 + 2;
	}
	sg::parallel::myGlobalMPIComm->IrecvFromAll(ptrTemp, _mpi_data_sizes_global, _mpi_data_offsets_global, _mpi_data_sizes, _mpi_data_offsets, tagsData, _chunkCountData, dataRecvReqs);

	MPI_Request gridRecvReqs[_chunkCountGrid]; //allocating a little more than necessary, otherwise complicated index computations needed
	int tagsGrid[_chunkCountGrid];
	for (int i = 0; i<_chunkCountGrid; i++){
		tagsGrid[i] = _mpi_grid_offsets[i]*2 + 3;
	}
	sg::parallel::myGlobalMPIComm->IrecvFromAll(ptrResult, _mpi_grid_sizes_global, _mpi_grid_offsets_global, _mpi_grid_sizes, _mpi_grid_offsets, tagsGrid, _chunkCountGrid, gridRecvReqs);

	sg::parallel::X86SimdLinearMult mult = sg::parallel::X86SimdLinearMult(level_, index_, dataset_, alpha, temp);
	sg::parallel::X86SimdLinearMultTranspose multTranspose = sg::parallel::X86SimdLinearMultTranspose(level_, index_, dataset_, temp, result);

	this->myTimer_->start();

	int thread_count = 1;
	int thread_num = 0;
#ifdef _OPENMP
	#pragma omp parallel private (thread_num, thread_count)
	{
		thread_count = omp_get_num_threads();
		thread_num = omp_get_thread_num();
#endif

		size_t myDataChunkStart = _mpi_data_offsets_global[mpi_myrank];
		size_t myDataChunkEnd = myDataChunkStart + _mpi_data_sizes_global[mpi_myrank];
		for(size_t thread_chunk = myDataChunkStart + thread_num; thread_chunk<myDataChunkEnd; thread_chunk+=thread_count){
			size_t start = _mpi_data_offsets[thread_chunk];
			size_t end =  start + _mpi_data_sizes[thread_chunk];

			if (start % CHUNKDATAPOINTS_X86 != 0 || end % CHUNKDATAPOINTS_X86 != 0)
			{
				std::cout << "start%CHUNKDATAPOINTS_X86: " << start%CHUNKDATAPOINTS_X86 << "; end%CHUNKDATAPOINTS_X86: " << end%CHUNKDATAPOINTS_X86 << std::endl;
				throw sg::base::operation_exception("processed vector segment must fit to CHUNKDATAPOINTS_X86!");
			}

			mult(start, end);
			sg::parallel::myGlobalMPIComm->IsendToAll(&ptrTemp[start], _mpi_data_sizes[thread_chunk], start*2 + 2);

		}
#ifdef _OPENMP
	#pragma omp single
		{
#endif

	MPI_Waitall(_chunkCountData, dataRecvReqs, MPI_STATUSES_IGNORE);
	this->completeTimeMult_ += this->myTimer_->stop();

	// patch result -> set additional entries zero
	if (this->numTrainingInstances_ != temp.getSize())
	{
		for (size_t i = 0; i < (temp.getSize()-this->numTrainingInstances_); i++)
		{
			temp.set(temp.getSize()-(i+1), 0.0f);
		}
	}

	this->myTimer_->start();

#ifdef _OPENMP
		} //implicit OpenMP barrier here
#endif

		size_t myGridChunkStart = _mpi_grid_offsets_global[mpi_myrank];
		size_t myGridChunkEnd = myGridChunkStart + _mpi_grid_sizes_global[mpi_myrank];
		for(size_t thread_chunk = myGridChunkStart + thread_num; thread_chunk<myGridChunkEnd; thread_chunk+=thread_count){
			size_t start = _mpi_grid_offsets[thread_chunk];
			size_t end =  start + _mpi_grid_sizes[thread_chunk];

			multTranspose(start, end);

			sg::parallel::myGlobalMPIComm->IsendToAll(&ptrResult[start], _mpi_grid_sizes[thread_chunk], start*2 + 3);
		}


#ifdef _OPENMP
	}
#endif
	MPI_Waitall(_chunkCountGrid, gridRecvReqs, MPI_STATUSES_IGNORE);
	this->completeTimeMultTrans_ += this->myTimer_->stop();

	result.axpy(static_cast<double>(this->numTrainingInstances_)*this->lambda_, alpha);
}

void DMSystemMatrixVectorizedIdentityAsyncMPI::generateb(sg::base::DataVector& classes, sg::base::DataVector& b)
{
	int mpi_size = sg::parallel::myGlobalMPIComm->getNumRanks();
	int mpi_myrank = sg::parallel::myGlobalMPIComm->getMyRank();

	double* ptrB = b.getPointer();

	sg::base::DataVector myClasses(classes);
	// Apply padding
	if (this->numPatchedTrainingInstances_ != myClasses.getSize())
	{
		myClasses.resizeZero(this->numPatchedTrainingInstances_);
	}

	MPI_Request gridRecvReqs[_chunkCountGrid]; //allocating a little more than necessary, otherwise complicated index computations needed
	int tags[_chunkCountGrid];
	for(int i = 0; i<_chunkCountGrid; i++){
		tags[i] = _mpi_grid_offsets[i]+1;
	}
	sg::parallel::myGlobalMPIComm->IrecvFromAll(ptrB, _mpi_grid_sizes_global, _mpi_grid_offsets_global, _mpi_grid_sizes, _mpi_grid_offsets, tags, _chunkCountGrid, gridRecvReqs);

	sg::parallel::X86SimdLinearMultTranspose multTranspose = sg::parallel::X86SimdLinearMultTranspose(level_, index_, dataset_, myClasses, b);

	std::cout << "before openmp" << std::endl;

	int thread_count = 1;
	int thread_num = 0;
#ifdef _OPENMP
	#pragma omp parallel
	{
		thread_count = omp_get_num_threads();
		thread_num = omp_get_thread_num();
#endif

		size_t myGridChunkStart = _mpi_grid_offsets_global[mpi_myrank];
		size_t myGridChunkEnd = myGridChunkStart + _mpi_grid_sizes_global[mpi_myrank];
		for(size_t thread_chunk = myGridChunkStart + thread_num; thread_chunk<myGridChunkEnd; thread_chunk+=thread_count){
			size_t start = _mpi_grid_offsets[thread_chunk];
			size_t end =  start + _mpi_grid_sizes[thread_chunk];

			multTranspose(start, end);

			std::cout << "alltoall[" << thread_chunk << "]: from " << start << "; size" << _mpi_grid_sizes[thread_chunk] <<   std::endl;
			sg::parallel::myGlobalMPIComm->IsendToAll(&ptrB[start], _mpi_grid_sizes[thread_chunk], start+1);
			//std::cout << "after isendtoall " << thread_chunk << " --- " << _mpi_grid_offsets[thread_chunk] <<  std::endl;
		}

#ifdef _OPENMP
	}
#endif

	std::cout << "after openmp" << std::endl;

	MPI_Waitall(_chunkCountGrid, gridRecvReqs, MPI_STATUSES_IGNORE);

	std::cout << "after waitall" << std::endl;
}

void DMSystemMatrixVectorizedIdentityAsyncMPI::rebuildLevelAndIndex()
{
	delete this->level_;
	delete this->index_;

	this->level_ = new sg::base::DataMatrix(m_grid.getStorage()->size(), m_grid.getStorage()->dim());
	this->index_ = new sg::base::DataMatrix(m_grid.getStorage()->size(), m_grid.getStorage()->dim());

	m_grid.getStorage()->getLevelIndexArraysForEval(*(this->level_), *(this->index_));

	size_t approximateChunkSizeGrid = 100; // approximately how many blocks should be computed before sending
	_chunkCountGrid = m_grid.getStorage()->size()/approximateChunkSizeGrid;

	delete[] _mpi_grid_sizes;
	delete[] _mpi_grid_offsets;

	// arrays for distribution settings
	_mpi_grid_sizes = new int[_chunkCountGrid];
	_mpi_grid_offsets = new int[_chunkCountGrid];

	calcDistribution(m_grid.getStorage()->size(), _chunkCountGrid, _mpi_grid_sizes, _mpi_grid_offsets, 1);
	calcDistribution(m_grid.getStorage()->size(), sg::parallel::myGlobalMPIComm->getNumRanks(), _mpi_grid_sizes_global, _mpi_grid_offsets_global, 1);

}

void DMSystemMatrixVectorizedIdentityAsyncMPI::multVec(sg::parallel::X86SimdLinearMult &mult)
{

}

void DMSystemMatrixVectorizedIdentityAsyncMPI::multTransposeVec(sg::parallel::X86SimdLinearMultTranspose &multTranspose)
{

}

void DMSystemMatrixVectorizedIdentityAsyncMPI::calcDistribution(int totalSize, int numChunks, int *sizes, int *offsets, size_t blocksize)
{
	for(int chunkID = 0; chunkID < numChunks; ++chunkID){
		size_t size;
		size_t offset;
		sg::parallel::PartitioningTool::getPartitionSegment(totalSize, numChunks, chunkID, &size, &offset, blocksize);
		sizes[chunkID] = size;
		offsets[chunkID] = offset;
	}
}

int DMSystemMatrixVectorizedIdentityAsyncMPI::getWorkerCount()
{
	int thread_count = 1;
	int mpi_size = sg::parallel::myGlobalMPIComm->getNumRanks();
#ifdef _OPENMP
	#pragma omp parallel
	{
		thread_count = omp_get_num_threads();
	}
#endif
	int worker_count = mpi_size*thread_count;
	return worker_count;
}

}
}
