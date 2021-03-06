////////////////////////////////////////////////////////////////////////////////
//!
//!                         \author Simon C. Davenport 
//!
//!  \file
//!		This file contains an implementation some linear algebra functions for
//!     dense vectors
//!     
//!                    Copyright (C) Simon C Davenport
//!                                                                             
//!     This program is free software: you can redistribute it and/or modify
//!     it under the terms of the GNU General Public License as published by
//!     the Free Software Foundation, either version 3 of the License,
//!     or (at your option) any later version.
//!                                                                             
//!     This program is distributed in the hope that it will be useful, but
//!     WITHOUT ANY WARRANTY; without even the implied warranty of
//!     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//!     General Public License for more details.
//!                                                                             
//!     You should have received a copy of the GNU General Public License
//!     along with this program. If not, see <http://www.gnu.org/licenses/>.
//!                                                                             
////////////////////////////////////////////////////////////////////////////////

#include "dense_vector.hpp"

namespace utilities
{
    //!
    //! Copy part of a vector using a BLAS subroutine
    //!
    void CopyVector(
        double* out,        //!<    Pointer to output vector
        const double* in,   //!<    Pointer to input vector
        const int N)        //!<    Size of vector
    {
        static const int one = 1;
        ccopy_(&N, in, &one, out, &one);
    }

    //!
    //! Mpi sync the given vector with a specified node
    //!
    void MpiSyncVector(
        dvec& vector,               //!<  Vector to synchronize
        int syncNode,               //!<  Node to sync with
        utilities::MpiWrapper& mpi) //!<  Address of mpi wrapper   
    {
        unsigned int dim = vector.size();
        mpi.Sync(&dim, 1, syncNode);
        if(syncNode != mpi.m_id)
        {
            vector.resize(dim);
        }
        mpi.Sync(vector.data(), dim, syncNode);
    }

    //!
    //! Extract elements of the input to be written to a sub vector. Zero values in
    //! the sub vector are skipped.
    //!
    void ToSubVector(
        dvec& sub, 
        const dvec& input, 
        const unsigned int offset, 
        const std::vector<unsigned int>& zeros)
    {
        int nnzConsecutive = 0;
        int nnzCumulative = offset;
        int nzStart = 0;
        for(auto it_zeros = zeros.begin(); it_zeros < zeros.end(); ++it_zeros)
        {
            nnzConsecutive = *it_zeros - nzStart;
            if(nnzConsecutive)
            {
                CopyVector(&sub[nzStart], &input[nnzCumulative], nnzConsecutive);
            }
            nnzCumulative += nnzConsecutive;
            nzStart = *it_zeros + 1;
        }
        nnzConsecutive = sub.size() - nzStart;
        CopyVector(&sub[nzStart], &input[nnzCumulative], nnzConsecutive);
    }

    //!
    //! Overload for ToSubVector where no zeros are present
    //!
    void ToSubVector(
        dvec& sub, 
        const dvec& input, 
        const unsigned int offset)
    {
        int N = sub.size();
        CopyVector(&sub[0], &input[offset], N);
    }
    
    //!
    //! Extract elements of the sub-vector to be written to the output. Zero values in
    //! the sub-vector are skipped.
    //!
    void FromSubVector(
        const dvec& sub, 
        dvec& output, 
        const unsigned int offset,
        const std::vector<unsigned int>& zeros)
    {
        int nnzConsecutive = 0;
        int nnzCumulative = offset;
        int nzStart = 0;
        for(auto it_zeros = zeros.begin(); it_zeros < zeros.end(); ++it_zeros)
        {
            nnzConsecutive = *it_zeros - nzStart;
            if(nnzConsecutive)
            {
                CopyVector(&output[nnzCumulative], &sub[nzStart], nnzConsecutive);
            }
            nnzCumulative += nnzConsecutive;
            nzStart = *it_zeros + 1;
        }
        nnzConsecutive = sub.size() - nzStart;
        CopyVector(&output[nnzCumulative], &sub[nzStart], nnzConsecutive);
    }
    
    //!
    //! Overload for FromSubVector where no zeros are present
    //!
    void FromSubVector(
        const dvec& sub, 
        dvec& output, 
        const unsigned int offset)
    {
        int N = sub.size();
        CopyVector(&output[offset], &sub[0], N);
    }    
    
    //!
    //! Set to a random vector using the given seed
    //!
    void SetToRandomVector(
        std::vector<double>& vec, 
        const double scale, 
        const unsigned int seed)
    {
        std::minstd_rand generator;
        std::uniform_real_distribution<double> distribution(-scale, scale);
        generator.seed(seed);
        for(auto& it: vec)
        {
            it = distribution(generator);
        }
    }
    
    //!
    //! Compute output = a - b
    //!
    void VectorDiff(
        dvec& output, 
        const dvec& a, 
        const dvec& b)
    {
        static const int one = 1;
        static const double mOne = -1;
        int N = output.size();
        output = a;
        daxpy_(&N, &mOne, b.data(), &one, output.data(), &one);
    }
    
    //!
    //! Compute a := a + scale*b
    //!
    void VectorIncrement(
        dvec& a, 
        const double scale,
        const dvec& b)
    {
        static const int one = 1;
        int N = a.size();
        daxpy_(&N, &scale, b.data(), &one, a.data(), &one);
    }
    
    //!
    //! Compute: c_i = scale*a_i b_i + BETA*c_i
    //!
    void VectorHadamard(
        dvec& c, 
        const double scale,
        const dvec& a, 
        const dvec& b)
    {
        static const int one = 1;
        int N = c.size();
        static const int K = 0;
        static const double BETA = 0.0;
        dsbmv_("L", &N, &K, &scale, a.data(), &one, b.data(), 
               &one, &BETA, c.data(), &one);
    }
    
    //!
    //! Compute: a_i *= scale*b_i
    //!
    void VectorHadamardIncrement(
        dvec& a, 
        const double scale, 
        const dvec& b)
    {
        static const int block = 64;
        auto it_b = b.begin();
        for(auto it_a = a.begin(); it_a < a.end(); it_a+=block, it_b+=block)
        {
            auto it_b_block = it_b;
            for(auto it_a_block = it_a; it_a_block < a.end(); ++it_a_block, ++it_b_block)
            {
                *it_a_block *= scale * *it_b_block;
            }
        }
    }  

    //!
    //! Compute the dot product: output = sum_i a_i b_i
    //!
    double VectorDot(
        const dvec& a, 
        const dvec& b)
    {
        static const int one = 1;
        int N = a.size();
        return ddot_(&N, a.data(), &one, b.data(), &one);
    }
    
    //!
    //! Get the signs of the vector elements
    //!
    void VectorSgn(
        dvec& sgnVec,
        const dvec& vec)
    {
        auto it_sgn = sgnVec.begin();
        for(auto it_vec = vec.begin(); it_vec < vec.end(); ++it_vec, ++it_sgn)
        {
            *it_sgn = (*it_vec > 0) - (*it_vec < 0);
        }
    }
    
    //!
    //! Get the L2 sum of vector elements
    //!
    double VectorL2(
        const dvec& a)
    {
        static const int one = 1;
        int N = a.size();
        return ddot_(&N, a.data(), &one, a.data(), &one);
    }
    
    //!
    //! Compute the L1 norm: output = sum_i |a_i|
    //!
    double VectorL1( 
        const dvec& a)
    {
        static const int one = 1;
        int N = a.size();
        return dasum_(&N, a.data(), &one);
    }
    
    //!
    //! Compute the sum of vector elements
    //!
    double VectorSum(
        const dvec& a)
    {
        dvec temp(a.size(), 1.0);
        return VectorDot(a, temp);
    }
    
    //!
    //! Rescale the input factor by the scale factor
    //!
    void VectorScale(
        dvec& output, 
        const double scale,
        const dvec& input)
    {
        static const int one = 1;
        std::fill(output.begin(), output.end(), 0.0);
        int N = input.size();
        daxpy_(&N, &scale, input.data(), &one, output.data(), &one);
    }
}   //  End namespace utilities
