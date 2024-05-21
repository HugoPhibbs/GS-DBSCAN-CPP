//
// Created by hphi344 on 10/05/24.
//

#include "../../include/GsDBSCAN.h"
#include <arrayfire.h>
#include <cmath>
#include <cuda_runtime.h> // TODO need to resolve this
#include <af/cuda.h>
#include <cassert>

// Constructor to initialize the DBSCAN parameters
GsDBSCAN::GsDBSCAN(const af::array &X, const af::array &D, int minPts, int k, int m, float eps, bool skip_pre_checks)
        : X(X), D(D), minPts(minPts), k(k), m(m), eps(eps), skip_pre_checks(skip_pre_checks) {
        n = X.dims(0);
        d = X.dims(1);

}

/**
 * Performs the gs dbscan algorithm
 *
 * @param X ArrayFire af::array matrix for the X data points
 * @param D int for number of random vectors to generate
 * @param minPts min number of points as per the DBSCAN algorithm
 * @param k k parameter for the sDBSCAN algorithm. I.e. the number of closest/furthest random vectors to take for ecah data point
 * @param m m parameter for the sDBSCAN algorithm. I.e. the number of closest/furthest dataset vecs for each random vec to take
 * @param eps epsilon parameter for the sDBSCAN algorithm. I.e. the threshold for the distance between the random vec and the dataset vec
 * @param skip_pre_checks boolean flag to skip the pre-checks
 */
void GsDBSCAN::performGsDbscan(af::array &X, int D, int minPts, int k, int m, float eps ) {
    if (!skip_pre_checks) {
        preChecks(X, D, minPts, k, m, eps);
    }
    // Something something ... TODO
}

/**
 * Performs the pre-checks for the gs dbscan algorithm
 *
 * @param X ArrayFire af::array matrix for the X data points
 * @param D int for number of random vectors to generate
 * @param minPts min number of points as per the DBSCAN algorithm
 * @param k k parameter for the sDBSCAN algorithm. I.e. the number of closest/furthest random vectors to take for ecah data point
 * @param m m parameter for the sDBSCAN algorithm. I.e. the number of closest/furthest dataset vecs for each random vec to take
 * @param eps epsilon parameter for the sDBSCAN algorithm. I.e. the threshold for the distance between the random vec and the dataset vec
 */
void GsDBSCAN::preChecks(af::array &X, int D, int minPts, int k, int m, float eps) {
    assert(X.dims(1) > 0);
    assert(X.shape[1] > 0);
    assert(D > 0);
    assert(D >= k);
    assert(m >= minPts)
}

/**
 * Performs the pre-processing for the gs dbscan algorithm
 *
 * @param Y ArrayFire af::array matrix for the D random vectors
 * @return A, B matrices as per the GS-DBSCAN algorithm. A has size (n, 2*k) and B has size (2*k, m)
 */
void GsDBSCAN::preProcessing(af::array &Y) {
    // TODO implement me!
}

/**
 * Performs random projections between the X dataset and the random vector
 *
 * @param X af::array matrix for the X data points
 * @param D int for number of random vectors to generate
 * @param k k parameter for the sDBSCAN algorithm. I.e. the number of closest/furthest random vectors to take for ecah data point
 * @param m m parameter for the sDBSCAN algorithm. I.e. the number of closest/furthest dataset vecs for each random vec to take
 * @param eps epsilon parameter for the sDBSCAN algorithm. I.e. the threshold for the distance between the random vec and the dataset vec
 * @return af::array matrix for the random projections
 */
void GsDBSCAN::randomProjections(af::array &X, int D, int k, int m, float eps) {
    // TODO implement me!
}

/**
 * Constructs the A and B matrices as per the GS-DBSCAN algorithm
 *
 * A stores the indices of the closest and furthest random vectors per data point.
 *
 * @param X matrix containing the X dataset vectors
 * @param Y matrix containing the random vectors
 * @param k k parameter as per the DBSCAN algorithm
 * @param m m parameter as per the DBSCAN algorithm
 */
void GsDBSCAN::constructABMatrices(af::array &X, af::array &Y, int k, int m) {
    // TODO implement me!
}

/**
 * Finds the distances between each of the query points and their candidate neighbourhood vectors
 *
 * @param X matrix containing the X dataset vectors
 * @param A A matrix, see constructABMatrices
 * @param B B matrix, see constructABMatrices
 * @param alpha float for the alpha parameter to tune the batch size
 */
af::array GsDBSCAN::findDistances(af::array &X, af::array &A, af::array &B, float alpha) {
    int k = A.dims(1) / 2;
    int m = B.dims(1) / 2;

    int n = X.dims(0);
    int d = X.dims(1);

    int batchSize = GsDBSCAN::findDistanceBatchSize(alpha, n, d, k, m);

    af::array distances(n, 2*k*m, af::dtype::f32);
    af::array ABatch(batchSize, A.dims(1), A.type());
    af::array BBatch(batchSize, B.dims(1), B.type());
    af::array XBatch(batchSize, 2*k, m, d, X.type());
    af::array XBatchAdj(batchSize, 2*k*m, d, X.type());
    af::array XSubset(batchSize, d, X.type());
    af::array XSubsetReshaped = af::constant(0, XBatchAdj.dims(), XBatchAdj.type());
    af::array YBatch = af::constant(0, XBatchAdj.dims(), XBatchAdj.type());

    for (int i = 0; i < n; i += batchSize) {
        int maxBatchIdx = i + batchSize - 1;
        ABatch = A(af::seq(i, maxBatchIdx));
        BBatch = B(A);

        XBatch = X(BBatch); // TODO need to create XBatch before for loop?
        XBatchAdj = af::moddims(XBatch, XBatch.dims(0), XBatch.dims(1) * XBatch.dims(2), XBatch.dims(3));

        XSubset = X(af::seq(i, maxBatchIdx), af::span);

        XSubsetReshaped = moddims(XSubset, XSubset.dims(0), 1, XSubset.dims(1)); // Insert new dim

        YBatch = XBatchAdj - XSubsetReshaped;

        distances(af::seq(i, maxBatchIdx), af::span) = af::sqrt(af::sum(af::pow(YBatch, 2), 2)); // af doesn't have norms across arbitrary axes
    }

    return distances;
}

/**
 * Calculates the batch size for distance calculations
 *
 * @param n size of the X dataset
 * @param d dimension of the X dataset
 * @param k k parameter of the DBSCAN algorithm
 * @param m m parameter of the DBSCAN algorithm
 * @param alpha alpha param to tune the batch size
 * @return int for the calculated batch size
 */
int GsDBSCAN::findDistanceBatchSize(float alpha, int n, int d, int k, int m) {
    int batchSize = static_cast<int>(static_cast<long long>(n) * d * 2 * k * m / (std::pow(1024, 3) * alpha));

    if (batchSize == 0) {
        return n;
    }

    for (int div = batchSize; div > 0; div--) {
        if (n % div == 0) {
            return div;
        }
    }

    return -1; // Should never reach here
}

/**
 * Constructs the cluster graph for the DBSCAN algorithm
 *
 * @param distances matrix containing the distances between each query vector and it's candidate vectors
 * @param eps epsilon DBSCAN density param
 * @param k k parameter of the DBSCAN algorithm
 * @param m m parameter of the DBSCAN algorithm
 * @return a vector containing the (flattened) adjacency list of the cluster graph, along with another list V containing the starting index of each query vector in the adjacency list
 */
void GsDBSCAN::constructClusterGraph(af::array &distances, float eps, int k, int m) {
    af::array E = constructQueryVectorDegreeArray(distances, eps);
    af::array V = af::scan(E, 0, AF_BINARY_ADD, true); # Do an exclusive scan
    af::array adjacencyList = assembleAdjacencyList(distances, E, V, A, B, eps, 1024);

    return adjacencyList, V;
}

/**
 * Calculates the degree of the query vectors as per the G-DBSCAN algorithm.
 *
 * This function is used in the construction of the cluster graph by determining how many
 * candidate vectors fall within a given epsilon distance from each query vector.
 *
 * @param distances The matrix containing the distances between the query and candidate vectors.
 *                  Expected shape is (datasetSize, 2*k*m).
 * @param eps       The epsilon value for DBSCAN. Should be a scalar array of the same data type
 *                  as the distances array.
 *
 * @return The degree array of the query vectors, with shape (datasetSize, 1).
 */
af::array GsDBSCAN::constructQueryVectorDegreeArray(af::array &distances, float eps) {
    return af::sum(af::lt(distances, eps), 1);
}

/**
 * Assembles the adjacency list for the cluster graph
 *
 * See https://arrayfire.org/docs/interop_cuda.htm for info on this
 *
 * @param distances matrix containing the distances between each query vector and it's candidate vectors
 * @param E vector containing the degree of each query vector (how many candidate vectors are within eps distance of it)
 * @param V vector containing the starting index of each query vector in the resultant adjacency list (See the G-DBSCAN algorithm)
 * @param A A matrix, see constructABMatrices
 * @param B B matrix, see constructABMatrices
 * @param eps epsilon DBSCAN density param
 * @param blockSize size of each block when calculating the adjacency list - essentially the amount of query vectors to process per block
 */
af::array GsDBSCAN::assembleAdjacencyList(af::array &distances, af::array &E, af::array &V, af::array &A, af::array &B, float eps, int blockSize) {
    int n = E.dims(0);
    int k = A.dims(1) / 2;
    int m = B.dims(1);

    af::array adjacencyList = af::constant(-1, (E(n-1) + V(n-1)).scalar<int>(), af::dtype::u32);

    // Eval all matrices to ensure they are synced
    adjacencyList.eval();
    distances.eval();
    E.eval();
    V.eval();
    A.eval();
    B.eval();

    // Getting device pointers
    int *adjacencyList_d= x.device<float>();
    int *distances_d = distances.device<float>();
    int *E = E.device<int>();
    int *V_d = V.device<int>();
    int *A_d = A.device<int>();
    int *B_d = B.device<int>();

    // Getting cuda stream from af
    int afId = af::getDevice();
    int v = afcu::getNativeId(afId);
    cudaStream_t afCudaStream = afcu::getStream(cudaId);

    // Now we can call the kernel
    int numBlocks = std::max(1, n / blockSize);
    blockSize = std::min(n, blockSize);
    constructAdjacencyListForQueryVector<<numBlocks, blockSize, 0, afCudaStream>>(distances_d, adjacencyList_d, V_d, A_d, B_d, eps, n, k, m);

    // Unlock all the af arrays
    adjacencyList.unlock();
    distances.unlock();
    E.unlock();
    V.unlock();
    A.unlock();
    B.unlock();

    return adjacencyList;
}



/**
 * Kernel for constructing part of the cluster graph adjacency list for a particular vector
 *
 * @param distances matrix containing the distances between each query vector and it's candidate vectors
 * @param adjacencyList
 * @param V vector containing the degree of each query vector (how many candidate vectors are within eps distance of it)
 * @param A A matrix, see constructABMatrices. Stored flat as a float array
 * @param B B matrix, see constructABMatrices. Stored flat as a float array
 * @param n number of query vectors in the dataset
 * @param eps epsilon DBSCAN density param
 */
__global__ void constructAdjacencyListForQueryVector(float *distances, int *adjacencyList, int *V, int *A, int *B, float eps, int n, int k, int m) {
    idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return; // Exit if out of bounds. Don't assume that numQueryVectors is equal to the total number o threads

    int curr_idx = V[idx];

    int distances_rows = 2 * k * m;

    for (int j = 0; j < distances_rows; j++) {
        if (distances[idx * distances_rows + j] < eps) {
            ACol = j / m;
            BCol = j % m;
            BRow = A[idx * 2 * k + ACol];
            neighbourhoodVecIdx = B[BRow * m + BCol];

            adjacencyList[curr_idx] = neighbourhoodVecIdx;
            curr_idx++;
        }
    }
}
