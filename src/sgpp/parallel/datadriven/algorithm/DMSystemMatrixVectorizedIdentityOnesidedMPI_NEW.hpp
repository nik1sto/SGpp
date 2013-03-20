/******************************************************************************
* Copyright (C) 2010 Technische Universitaet Muenchen                         *
* This file is part of the SG++ project. For conditions of distribution and   *
* use, please see the copyright notice at http://www5.in.tum.de/SGpp          *
******************************************************************************/
// @author Alexander Heinecke (Alexander.Heinecke@mytum.de)
// @author Roman Karlstetter (karlstetter@mytum.de)

#ifndef DMSYSTEMMATRIXVECTORIZEDIDENTITYONESIDEDNEWMPI_HPP
#define DMSYSTEMMATRIXVECTORIZEDIDENTITYONESIDEDNEWMPI_HPP

#include "datadriven/algorithm/DMSystemMatrixBase.hpp"
#include "parallel/tools/MPI/SGppMPITools.hpp"
#include "base/datatypes/DataVector.hpp"
#include "base/exception/operation_exception.hpp"
#include "base/grid/Grid.hpp"
#include "parallel/datadriven/operation/OperationMultipleEvalVectorized.hpp"
#include "parallel/datadriven/tools/DMVectorizationPaddingAssistant.hpp"
#include "parallel/tools/PartitioningTool.hpp"
#include "parallel/tools/TypesParallel.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

#define APPROXCHUNKSIZEGRID_ASYNC 100 // approximately how many blocks should be computed for the grid before sending
#define APPROXCHUNKSIZEDATA_ASYNC 1000 // approximately how many blocks should be computed for the data before sending

namespace sg
{
namespace parallel
{

/**
 * Class that implements the virtual class sg::base::OperationMatrix for the
 * application of classification for the Systemmatrix
 *
 * The Identity matrix is used as regularization operator.
 *
 * For the Operation B's mult and mutlTransposed functions
 * vectorized formulations are used.
 */
template<typename MultType, typename MultTransType>
class DMSystemMatrixVectorizedIdentityOneSidedMPI : public sg::datadriven::DMSystemMatrixBase
{
private:
	/// vectorization mode
	VectorizationType vecMode_;
	/// Number of orignal training instances
	size_t numTrainingInstances_;
	/// Number of patched and used training instances
	size_t numPatchedTrainingInstances_;

	sg::base::DataMatrix* level_;
	/// Member to store the sparse grid's indices for better vectorization
	sg::base::DataMatrix* index_;


public:
	/**
	 * Constructor
	 *
	 * @param SparseGrid reference to the sparse grid
	 * @param trainData reference to sg::base::DataMatrix that contains the training data
	 * @param lambda the lambda, the regression parameter
	 * @param vecMode vectorization mode
	 */
	DMSystemMatrixVectorizedIdentityOneSidedMPI(sg::base::Grid& SparseGrid, sg::base::DataMatrix& trainData, double lambda, VectorizationType vecMode)
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

		size_t blockCountData = this->numPatchedTrainingInstances_/sg::parallel::DMVectorizationPaddingAssistant::getVecWidth(this->vecMode_); // this process has (in total) blockCountData blocks of Data to process
		_chunkCountData = blockCountData/APPROXCHUNKSIZEDATA_ASYNC;
		_chunkCountData = fmax(_chunkCountData, mpi_size); // every process should have at least one chunk

		// arrays for distribution settings
		_mpi_data_sizes = new int[_chunkCountData];
		_mpi_data_offsets = new int[_chunkCountData];
		_mpi_data_sizes_global = new int[mpi_size];
		_mpi_data_offsets_global = new int[mpi_size];

		sg::parallel::PartitioningTool::calcDistribution(this->numPatchedTrainingInstances_, _chunkCountData, _mpi_data_sizes, _mpi_data_offsets, sg::parallel::DMVectorizationPaddingAssistant::getVecWidth(this->vecMode_));
		sg::parallel::PartitioningTool::calcDistribution(_chunkCountData, mpi_size, _mpi_data_sizes_global, _mpi_data_offsets_global, 1);

		_chunkCountGrid = m_grid.getSize()/APPROXCHUNKSIZEGRID_ASYNC;
		_chunkCountGrid = fmax(_chunkCountGrid, mpi_size); // every process should have at least one chunk

		// arrays for distribution settings
		_mpi_grid_sizes = new int[_chunkCountGrid];
		_mpi_grid_offsets = new int[_chunkCountGrid];
		_mpi_grid_sizes_global = new int[mpi_size];
		_mpi_grid_offsets_global = new int[mpi_size];

		sg::parallel::PartitioningTool::calcDistribution(m_grid.getSize(), _chunkCountGrid, _mpi_grid_sizes, _mpi_grid_offsets, 1);
		sg::parallel::PartitioningTool::calcDistribution(_chunkCountGrid, mpi_size, _mpi_grid_sizes_global, _mpi_grid_offsets_global, 1);

		this->level_ = new sg::base::DataMatrix(m_grid.getSize(), m_grid.getStorage()->dim());
		this->index_ = new sg::base::DataMatrix(m_grid.getSize(), m_grid.getStorage()->dim());

		m_grid.getStorage()->getLevelIndexArraysForEval(*(this->level_), *(this->index_));

		// mult: distribute calculations over dataset
		// multTranspose: distribute calculations over grid
	}

	/**
	 * Std-Destructor
	 */
	virtual ~DMSystemMatrixVectorizedIdentityOneSidedMPI(){
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

	virtual void mult(sg::base::DataVector& alpha, sg::base::DataVector& result){
		sg::base::DataVector temp(this->numPatchedTrainingInstances_);
		result.setAll(0.0);
		double* ptrResult = result.getPointer();
		double* ptrTemp = temp.getPointer();

		int mpi_size = sg::parallel::myGlobalMPIComm->getNumRanks();
		int mpi_myrank = sg::parallel::myGlobalMPIComm->getMyRank();

		this->myTimer_->start();
		int thread_count = 1;
		int thread_num = 0;
		#pragma omp parallel private (thread_num, thread_count)
		{
	#ifdef _OPENMP
			thread_count = omp_get_num_threads();
			thread_num = omp_get_thread_num();
	#endif

			size_t myDataChunkStart = _mpi_data_offsets_global[mpi_myrank];
			size_t myDataChunkEnd = myDataChunkStart + _mpi_data_sizes_global[mpi_myrank];
			for(size_t thread_chunk = myDataChunkStart + thread_num; thread_chunk<myDataChunkEnd; thread_chunk+=thread_count){
				size_t start = _mpi_data_offsets[thread_chunk];
				size_t end =  start + _mpi_data_sizes[thread_chunk];

				if (start % MultType::getChunkDataPoints() != 0 || end % MultType::getChunkDataPoints() != 0)
				{
					std::cout << "start%CHUNKDATAPOINTS_X86: " << start%sg::parallel::X86SimdLinearMult::getChunkDataPoints() << "; end%CHUNKDATAPOINTS_X86: " << end%sg::parallel::X86SimdLinearMult::getChunkDataPoints() << std::endl;
					throw sg::base::operation_exception("processed vector segment must fit to CHUNKDATAPOINTS_X86!");
				}

				MultType::mult(level_, index_, dataset_, alpha, temp, 0, alpha.getSize(), start, end);

				sg::parallel::myGlobalMPIComm->IsendToAll(&ptrTemp[start], _mpi_data_sizes[thread_chunk], start*2 + 2);

			}
		#pragma omp single
			{
				this->computeTimeMult_ += this->myTimer_->stop();

//				MPI_Waitall(_chunkCountData, dataRecvReqs, MPI_STATUSES_IGNORE);
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

			} //implicit OpenMP barrier here

			size_t myGridChunkStart = _mpi_grid_offsets_global[mpi_myrank];
			size_t myGridChunkEnd = myGridChunkStart + _mpi_grid_sizes_global[mpi_myrank];

			for(size_t thread_chunk = myGridChunkStart + thread_num; thread_chunk<myGridChunkEnd; thread_chunk+=thread_count){
				size_t start = _mpi_grid_offsets[thread_chunk];
				size_t end =  start + _mpi_grid_sizes[thread_chunk];

				MultTransType::multTranspose(level_, index_, dataset_, temp, result, start, end, 0, this->numPatchedTrainingInstances_);
				sg::parallel::myGlobalMPIComm->IsendToAll(&ptrResult[start], _mpi_grid_sizes[thread_chunk], start*2 + 3);
			}


		}
		this->computeTimeMultTrans_ += this->myTimer_->stop();

//		MPI_Waitall(_chunkCountGrid, gridRecvReqs, MPI_STATUSES_IGNORE);
		this->completeTimeMultTrans_ += this->myTimer_->stop();

		result.axpy(static_cast<double>(this->numTrainingInstances_)*this->lambda_, alpha);
		if (mpi_myrank == 0) std::cout << "*";
	}

	virtual void generateb(sg::base::DataVector& classes, sg::base::DataVector& b){
		int mpi_size = sg::parallel::myGlobalMPIComm->getNumRanks();
		int mpi_myrank = sg::parallel::myGlobalMPIComm->getMyRank();

		double* ptrB = b.getPointer();
		b.setAll(0.0);

		sg::base::DataVector myClasses(classes);
		// Apply padding
		if (this->numPatchedTrainingInstances_ != myClasses.getSize())
		{
			myClasses.resizeZero(this->numPatchedTrainingInstances_);
		}

		int thread_count = 1;
		int thread_num = 0;
	#pragma omp parallel private(thread_count, thread_num)
		{
	#ifdef _OPENMP
			thread_count = omp_get_num_threads();
			thread_num = omp_get_thread_num();
	#endif
			size_t myGridChunkStart = _mpi_grid_offsets_global[mpi_myrank];
			size_t myGridChunkEnd = myGridChunkStart + _mpi_grid_sizes_global[mpi_myrank];
			for(size_t thread_chunk = myGridChunkStart + thread_num; thread_chunk<myGridChunkEnd; thread_chunk+=thread_count){
				size_t start = _mpi_grid_offsets[thread_chunk];
				size_t end =  start + _mpi_grid_sizes[thread_chunk];

				MultTransType::multTranspose(level_, index_, dataset_, myClasses, b, start, end, 0, this->numPatchedTrainingInstances_);
				sg::parallel::myGlobalMPIComm->IsendToAll(&ptrB[start], _mpi_grid_sizes[thread_chunk], start+1);
			}
		}
		MPI_Status stats[_chunkCountGrid];

	}

	virtual void rebuildLevelAndIndex(){
		delete this->level_;
		delete this->index_;

		this->level_ = new sg::base::DataMatrix(m_grid.getSize(), m_grid.getStorage()->dim());
		this->index_ = new sg::base::DataMatrix(m_grid.getSize(), m_grid.getStorage()->dim());

		m_grid.getStorage()->getLevelIndexArraysForEval(*(this->level_), *(this->index_));

		_chunkCountGrid = m_grid.getSize()/APPROXCHUNKSIZEGRID_ASYNC;
		_chunkCountGrid = fmax(_chunkCountGrid, sg::parallel::myGlobalMPIComm->getNumRanks()); // every process should have at least one chunk


		delete[] _mpi_grid_sizes;
		delete[] _mpi_grid_offsets;

		// arrays for distribution settings
		_mpi_grid_sizes = new int[_chunkCountGrid];
		_mpi_grid_offsets = new int[_chunkCountGrid];

		sg::parallel::PartitioningTool::calcDistribution(m_grid.getSize(), _chunkCountGrid, _mpi_grid_sizes, _mpi_grid_offsets, 1);
		sg::parallel::PartitioningTool::calcDistribution(_chunkCountGrid, sg::parallel::myGlobalMPIComm->getNumRanks(), _mpi_grid_sizes_global, _mpi_grid_offsets_global, 1);

		if(sg::parallel::myGlobalMPIComm->getMyRank() == 0) {
			for(int i = 0; i<sg::parallel::myGlobalMPIComm->getNumRanks(); i++){
				//std::cout << "glob: " << _mpi_grid_offsets_global[i] << " (" << _mpi_grid_sizes_global[i] << ")" <<std::endl;
			}
			for (int i = 0; i<_chunkCountGrid; i++){
				//std::cout << "loc: " << _mpi_grid_offsets[i] << " (" << _mpi_grid_sizes[i] << ")" << std::endl;
			}
		}
	}

private:
	/// how to distribute storage array across processes
	int * _mpi_grid_sizes;
	int * _mpi_grid_offsets;

	/// reference to grid. needed to get new grid size after it changes
	sg::base::Grid& m_grid;

	/// how to distribute dataset across processes
	int * _mpi_data_sizes;
	int * _mpi_data_offsets;

	/// which chunks belong to which process
	int * _mpi_data_sizes_global;
	int * _mpi_data_offsets_global;

	/// which chunks belong to which process
	int * _mpi_grid_sizes_global;
	int * _mpi_grid_offsets_global;

	/// MPI windows
	sg::base::DataVector* _mpi_grid_window_buffer;
	sg::base::DataVector* _mpi_data_window_buffer;
	MPI_Win* _mpi_grid_window;
	MPI_Win* _mpi_data_window;

	/// into how many chunks should data and grid be partitioned
	size_t _chunkCountData;
	size_t _chunkCountGrid;

	void initWindows(double* windowBufferPtr, MPI_Win* windowsArray, int* sizes, int* offsets, int* sizes_global, int* offsets_global)
	{
		for(int rank = 0; rank < sg::parallel::myGlobalMPIComm->getNumRanks(); rank++) {
			int size = 0;
			for(int i = 0; i<sizes_global[rank]; i++){
				size += sizes[offsets_global[rank] + i];
			}
			int offset = offsets[offsets_global[rank]];
			MPI_Win_create(&windowBufferPtr[offset], size*sizeof(double), sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD, &windowsArray[rank]);
			MPI_Win_fence(0, windowsArray[rank]);

			if(sg::parallel::myGlobalMPIComm->getMyRank() == 0){
				std::cout << "win init [" << rank << "] " <<  offset << " - " << offset + size - 1 << " = " << size << std::endl;
			}
		}
	}


	void createAndInitGridBuffers(){
		int mpi_size = sg::parallel::myGlobalMPIComm->getNumRanks();
		_chunkCountGrid = m_grid.getSize()/APPROXCHUNKSIZEGRID;
		_chunkCountGrid = fmax(_chunkCountGrid, mpi_size);

		// arrays for distribution settings
		_mpi_grid_sizes = new int[_chunkCountGrid];
		_mpi_grid_offsets = new int[_chunkCountGrid];
		_mpi_grid_sizes_global = new int[mpi_size];
		_mpi_grid_offsets_global = new int[mpi_size];

		sg::parallel::PartitioningTool::calcDistribution(m_grid.getSize(), _chunkCountGrid, _mpi_grid_sizes, _mpi_grid_offsets, 1);
		sg::parallel::PartitioningTool::calcDistribution(_chunkCountGrid, mpi_size, _mpi_grid_sizes_global, _mpi_grid_offsets_global, 1);

		this->level_ = new sg::base::DataMatrix(m_grid.getSize(), m_grid.getStorage()->dim());
		this->index_ = new sg::base::DataMatrix(m_grid.getSize(), m_grid.getStorage()->dim());

		m_grid.getStorage()->getLevelIndexArraysForEval(*(this->level_), *(this->index_));

		// MPI Windows
		_mpi_grid_window_buffer = new sg::base::DataVector(m_grid.getSize());
		_mpi_grid_window = new MPI_Win[mpi_size];

		initWindows(_mpi_grid_window_buffer->getPointer(), _mpi_grid_window, _mpi_grid_sizes, _mpi_grid_offsets, _mpi_grid_sizes_global, _mpi_grid_offsets_global);
	}

	void createAndInitDataBuffers(sg::base::DataMatrix &trainData){
		int mpi_size = sg::parallel::myGlobalMPIComm->getNumRanks();

		this->dataset_ = new sg::base::DataMatrix(trainData);
		this->numTrainingInstances_ = this->dataset_->getNrows();
		this->numPatchedTrainingInstances_ = sg::parallel::DMVectorizationPaddingAssistant::padDataset(*(this->dataset_), vecMode_);

		if (this->vecMode_ != OpenCL && this->vecMode_ != ArBB && this->vecMode_ != Hybrid_X86SIMD_OpenCL)
		{
			this->dataset_->transpose();
		}

		size_t blockCountData = this->numPatchedTrainingInstances_/sg::parallel::DMVectorizationPaddingAssistant::getVecWidth(this->vecMode_); // this process has (in total) blockCountData blocks of Data to process
		_chunkCountData = blockCountData/APPROXCHUNKSIZEDATA;
		_chunkCountData = fmax(_chunkCountData, mpi_size);

		// arrays for distribution settings
		_mpi_data_sizes = new int[_chunkCountData];
		_mpi_data_offsets = new int[_chunkCountData];
		_mpi_data_sizes_global = new int[mpi_size];
		_mpi_data_offsets_global = new int[mpi_size];

		sg::parallel::PartitioningTool::calcDistribution(this->numPatchedTrainingInstances_, _chunkCountData, _mpi_data_sizes, _mpi_data_offsets, sg::parallel::DMVectorizationPaddingAssistant::getVecWidth(this->vecMode_));
		sg::parallel::PartitioningTool::calcDistribution(_chunkCountData, mpi_size, _mpi_data_sizes_global, _mpi_data_offsets_global, 1);

		// create MPI windows
		_mpi_data_window_buffer = new sg::base::DataVector(this->numPatchedTrainingInstances_);
		_mpi_data_window = new MPI_Win[mpi_size];
		initWindows(_mpi_data_window_buffer->getPointer(), _mpi_data_window, _mpi_data_sizes, _mpi_data_offsets, _mpi_data_sizes_global, _mpi_data_offsets_global);
	}

	void freeDataBuffers(){
		delete this->dataset_;

		delete[] this->_mpi_data_sizes;
		delete[] this->_mpi_data_offsets;
		delete[] this->_mpi_data_sizes_global;
		delete[] this->_mpi_data_offsets_global;

		delete this->_mpi_data_window_buffer;
		for (int rank = 0; rank<sg::parallel::myGlobalMPIComm->getNumRanks(); rank++) {
			MPI_Win_free(&_mpi_data_window[rank]);
		}
		std::cout << "freed data wins" << std::endl;
		delete[] _mpi_data_window;
	}

	void freeGridBuffers(){
		delete this->level_;
		delete this->index_;

		delete[] this->_mpi_grid_sizes;
		delete[] this->_mpi_grid_offsets;
		delete[] this->_mpi_grid_sizes_global;
		delete[] this->_mpi_grid_offsets_global;

		delete _mpi_grid_window_buffer;
		std::cout << "freeing grid wins" << std::endl;
		for (int rank = 0; rank<sg::parallel::myGlobalMPIComm->getNumRanks(); rank++) {
			MPI_Win_free(&_mpi_grid_window[rank]);
		}
		delete[] _mpi_grid_window;
	}
};

}
}

#endif /* DMSYSTEMMATRIXVECTORIZEDIDENTITYONESIDEDNEWMPI_HPP */