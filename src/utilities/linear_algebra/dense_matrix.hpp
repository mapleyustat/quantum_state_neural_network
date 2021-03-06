////////////////////////////////////////////////////////////////////////////////
//!
//!                         \author Simon C. Davenport 
//!
//!  \file
//!		This file contains an implementation of a generic dense matrix 
//!     container implementation and some related linear algebra
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

#ifndef _DENSE_MATRIX_HPP_INCLUDED_
#define _DENSE_MATRIX_HPP_INCLUDED_

///////     LIBRARY INCLUSIONS     /////////////////////////////////////////////
#include "../general/dvec_def.hpp"
#include "../general/cout_tools.hpp"
#include "../wrappers/mpi_wrapper.hpp"
#include "dense_vector.hpp"
#include "blas.hpp"
#include <iostream>
#if _DEBUG_
#include "../general/debug.hpp"
#endif

namespace utilities
{
    //!
    //! Simple container for a general dense matrix
    //!
    template<typename T>
    struct matrix
    {
        unsigned int m_dLeading;    //!<    Leading dimension of matrix
        unsigned int m_dSecond;     //!<    Second dimension of matrix
        std::vector<T> m_data;      //!<    Container for matrix data
        
        //!
        //! Default constructor
        //!
        matrix()
            :
            m_dLeading(0),
            m_dSecond(0)
        {}
        
        //!
        //! Allocate space to store a n*m dense matrix
        //!
        matrix(const unsigned int dLeading, const unsigned int dSecond)
        {
            this->resize(dLeading, dSecond);
        }
        
        //!
        //! Mpi sync the container
        //!
        void MpiSync(
            int syncNode,
            utilities::MpiWrapper& mpi)
        {
            mpi.Sync(&m_dLeading, 1, syncNode);
            mpi.Sync(&m_dSecond, 1, syncNode);
            utilities::MpiSyncVector(m_data, syncNode, mpi);
        }
        
        //!
        //! Resize the container
        //!
        void resize(const unsigned int dLeading, const unsigned int dSecond)
        {
            m_dLeading = dLeading;
            m_dSecond = dSecond;
            m_data.resize(m_dLeading*m_dSecond);
        }
        
        //!
        //! Get a pointer to the underlying container
        //!
        T* data()
        {
            return m_data.data();
        }
        
        //!
        //! Get a pointer to the underlying container
        //!
        const T* data() const
        {
            return m_data.data();
        }
        
        //!
        //! Set accessor for a single matrix element
        //!
        T& operator() (const unsigned int row, const unsigned int col)
        {
            return m_data.at(row*m_dSecond+col);
        }
        
        //!
        //! Get accessor for a single matrix element
        //!
        T operator() (const unsigned int row, const unsigned int col) const
        {
            return m_data.at(row*m_dSecond+col);
        }
        
        //!
        //! Set a row of the matrix from a vector
        //!
        void SetRow(
            const unsigned int row, 
            const std::vector<T>& rowBuffer)
        {
            CopyVector(&m_data[row*m_dSecond], rowBuffer.data(), rowBuffer.size());
        }
        
        //!
        //! Print out matrix elements
        //!
        void Print() const
        {
            for(unsigned int row=0; row<m_dLeading; ++row)
            {
                for(unsigned int col=0; col<m_dSecond; ++col)
                {
                    utilities::cout.SecondaryOutput() <<  m_data[row*m_dSecond+col] << " ";
                }
                utilities::cout.SecondaryOutput() << std::endl;
            }
        }
    };
    
    static const char UPLO = 'U';   //!<    Symmetric matrix update convention
    void SetToRandomMatrix(matrix<double>& mat, const double scale, const unsigned int seed);
    void SetToConstantMatrix(matrix<double>& mat, const double value);
    void SetToIdentityMatrix(matrix<double>& mat);
    void MatrixVectorMultiply(dvec& output, const double scale, const matrix<double>& a, 
                              const dvec& x, char TRANS);
    void SymmetricMatrixVectorMultiply(dvec& output, const double scale, 
                                       const matrix<double>& a, const dvec& x);
    void MatrixMatrixMultiply(matrix<double>& c, const matrix<double>& a, 
                              const matrix<double>& b, std::string trOpt);
    void OuterProductIncrement(matrix<double>& a, const double scale, const dvec& x, 
                               const dvec& y);
    void SymmetricOuterProductIncrement(matrix<double>& a, const double scale, const dvec& x);
    void SymmetricOuterProductIncrement(matrix<double>& a, const double scale, const dvec& x, 
                                        const dvec& y);
    void MatrixHadamard(matrix<double>& c, const double scale, const matrix<double>& a, 
                        const matrix<double>& b);
    void MatrixHadamardIncrement(matrix<double>& a, const double scale, const matrix<double>& b);
    void MatrixIncrement(matrix<double>& a, const double& scale, const matrix<double>& b);
    void MatrixSgn(matrix<double>& sgnMat, const matrix<double>& mat);
    void MatrixMask(matrix<double>& mat, const std::vector<unsigned int>& zeros);
    double MatrixL2(const matrix<double>& mat);
    double MatrixL1(const matrix<double>& mat);
    void ToSubMatrix(matrix<double>& output, const matrix<double>& input, 
                     const unsigned int leadingOffset, const unsigned int secondOffset);
}   //  End namespace utilities
#endif
