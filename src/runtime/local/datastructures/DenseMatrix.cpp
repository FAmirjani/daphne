/*
 * Copyright 2021 The DAPHNE Consortium
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// TODO DenseMatrix should not be concerned about CUDA.

#include "DenseMatrix.h"
#include <chrono>

#ifdef USE_CUDA
    #include <runtime/local/kernels/CUDA/HostUtils.h>
#endif

template<typename ValueType>
DenseMatrix<ValueType>::~DenseMatrix()
{
#ifdef USE_CUDA
    if(cuda_ptr) {
//#ifndef NDEBUG
//        std::cerr << "calling cudaFree " << cuda_ptr << std::endl;
//#endif
        CHECK_CUDART(cudaFree(cuda_ptr));
        cuda_ptr = nullptr;
    }
#endif
}

#ifdef USE_CUDA
template <typename ValueType>
void DenseMatrix<ValueType>::cudaAlloc() {
    size_t requested = numRows*numCols*sizeof(ValueType);
    size_t available; size_t total;
    auto start = std::chrono::high_resolution_clock::now();
    cudaMemGetInfo(&available, &total);
    auto end = std::chrono::high_resolution_clock::now();
    if(requested > available) {
        throw std::runtime_error("Insufficient GPU memory! Requested=" + std::to_string(requested) + " Available="
                + std::to_string(available));
    }
#ifndef NDEBUG
    std::cerr << "cudaMalloc " << requested << " bytes" << " of " << available << "(query time=" <<
    std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "µs)" << std::endl;
#endif
    CHECK_CUDART(cudaMalloc(reinterpret_cast<void**>(&cuda_ptr), numRows*numCols*sizeof(ValueType)));
#ifndef NDEBUG
    std::cerr << " address=" << cuda_ptr << std::endl;
#endif
}

template <typename ValueType>
void DenseMatrix<ValueType>::cuda2host() {
//#ifndef NDEBUG
//    std::cerr << "dev2host" << std::endl;
//#endif
    if(!values)
        values = std::make_shared<ValueType>(numRows*numCols);
    CHECK_CUDART(cudaMemcpy(values.get(), cuda_ptr, numRows*numCols*sizeof(ValueType), cudaMemcpyDeviceToHost));
    cuda_dirty = false;
}

template <typename ValueType>
void DenseMatrix<ValueType>::host2cuda() {
//#ifndef NDEBUG
//    std::cerr << "host2dev" << std::endl;
//#endif
    if (!cuda_ptr) {
        cudaAlloc();
    }
//    if(values.get() == nullptr) {
//        std::cout <<" values is null!" << std::endl;
//        return;
//    }
//    if(!cuda_ptr) {
//        std::cout << " device ptr is still null!" << std::endl;
//        host_dirty = false;
//        return;
//    }
    CHECK_CUDART(cudaMemcpy(cuda_ptr, values.get(), numRows*numCols*sizeof(ValueType), cudaMemcpyHostToDevice));
    host_dirty = false;
}

#endif // USE_CUDA

template <typename ValueType> void DenseMatrix<ValueType>::printValue(std::ostream & os, ValueType val) const {
    os << val;
}

template<typename ValueType>
DenseMatrix<ValueType>* DenseMatrix<ValueType>::vectorTranspose() const {
    assert((this->numRows == 1 || this->numCols == 1) && "no-op transpose for vectors only");

    auto transposed = DataObjectFactory::create<DenseMatrix<ValueType>>(this->getNumCols(), this->getNumRows(),
                                                                        this->getValuesSharedPtr(), this->getCUDAValuesSharedPtr());
    transposed->cuda_dirty = this->cuda_dirty;
    transposed->cuda_buffer_current = this->cuda_buffer_current;
    transposed->host_dirty = this->host_dirty;
    transposed->host_buffer_current = this->host_buffer_current;
    return transposed;
}

// Convert to an integer to print uint8_t values as numbers
// even if they fall into the range of special ASCII characters.
template <> void DenseMatrix<unsigned char>::printValue(std::ostream & os, unsigned char val) const
{
    os << (unsigned int)val;
}
template <> void DenseMatrix<signed char>::printValue(std::ostream & os, signed char val) const
{
    os << (int)val;
}

// explicitly instantiate to satisfy linker
template class DenseMatrix<double>;
template class DenseMatrix<float>;
template class DenseMatrix<int>;
template class DenseMatrix<long>;
template class DenseMatrix<signed char>;
template class DenseMatrix<unsigned char>;
template class DenseMatrix<unsigned int>;
template class DenseMatrix<unsigned long>;
