/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#include "distcomp.h"
#include "string.h"
#include "logging.h"
#include "pow.h"

#include <cstdlib>
#include <limits>
#include <algorithm>
#include <cmath>

#ifdef __SSE2__
#include <immintrin.h>
#endif

namespace similarity {

using namespace std;

/*
 * LInf-norm.
 */

template <class T>
T LInfNormStandard(const T *p1, const T *p2, size_t qty) 
{ 
    T res = 0;

    for (size_t i = 0; i < qty; i++) { 
      res = max(res, fabs(p1[i]-p2[i]));
    }

    return res;
}

template float LInfNormStandard<float>(const float *p1, const float *p2, size_t qty);
template double LInfNormStandard<double>(const double *p1, const double *p2, size_t qty);


template <class T>
T LInfNorm(const T* pVect1, const T* pVect2, size_t qty) {
    T res = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    while (pVect1 < pEnd1) {
        res = max(res, fabs(*pVect1++ - *pVect2++));
        res = max(res, fabs(*pVect1++ - *pVect2++));
        res = max(res, fabs(*pVect1++ - *pVect2++));
        res = max(res, fabs(*pVect1++ - *pVect2++));
    }

    while (pVect1 < pEnd2) {
        res = max(res, fabs(*pVect1++ - *pVect2++));
    }

    return res;
}

template float LInfNorm<float>(const float* pVect1, const float* pVect2, size_t qty);
template double LInfNorm<double>(const double* pVect1, const double* pVect2, size_t qty);
/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */

template <> 
float LInfNormSIMD(const float* pVect1, const float* pVect2, size_t qty) {
#ifndef __SSE2__
#warning "LInfNormSIMD<float>: SSE2 is not available, defaulting to pure C++ implementation!"
    return LInfNormStandard(pVect1, pVect2, qty);
#else
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    __m128  diff, v1, v2; 

    /* 
     * A hack to quickly unset the sign flag.
     */
    __m128 mask_sign = reinterpret_cast<__m128>(_mm_set1_epi32(0x7fffffffu));
    __m128 MAX = _mm_setzero_ps();

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_and_ps(diff, mask_sign ));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_and_ps(diff, mask_sign ));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_and_ps(diff, mask_sign ));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_and_ps(diff, mask_sign ));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_and_ps(diff, mask_sign ));
    }

    float __attribute__((aligned(16))) TmpRes[4];

    _mm_store_ps(TmpRes, MAX);
    float res= max(max(TmpRes[0], TmpRes[1]), max(TmpRes[2], TmpRes[3]));

    while (pVect1 < pEnd3) {
        res = max(res, fabsf(*pVect1++ - *pVect2++));
    }

    return res;
#endif
}

template <> 
double LInfNormSIMD(const double* pVect1, const double* pVect2, size_t qty) {
#ifndef __SSE2__
#warning "LInfNormSIMD<double>: SSE2 is not available, defaulting to pure C++ implementation!"
    return LInfNormStandard(pVect1, pVect2, qty);
#else
    size_t qty8 = qty/8;

    const double* pEnd1 = pVect1 + 8 * qty8;
    const double* pEnd2 = pVect1 + qty;

    __m128d  diff, v1, v2; 
    __m128d  MAX = _mm_set1_pd(0); 


    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        MAX  = _mm_max_pd(MAX, _mm_max_pd(_mm_sub_pd(_mm_setzero_pd(), diff), diff));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        MAX  = _mm_max_pd(MAX, _mm_max_pd(_mm_sub_pd(_mm_setzero_pd(), diff), diff));

    }

    double __attribute__((aligned(16))) TmpRes[2];

    _mm_storeu_pd(TmpRes, MAX);
    double res= max(TmpRes[0], TmpRes[1]);

    while (pVect1 < pEnd2) {
        double diff = fabs(*pVect1++ - *pVect2++);
        res = max(res, (double)diff);
    }

    return res;
#endif
}

template float LInfNormSIMD<float>(const float* pVect1, const float* pVect2, size_t qty);
template double LInfNormSIMD<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * L1-norm.
 */

template <class T>
T L1NormStandard(const T *p1, const T *p2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += fabs(p1[i]-p2[i]);
    }

    return sum;
}

template float L1NormStandard<float>(const float *p1, const float *p2, size_t qty);
template double L1NormStandard<double>(const double *p1, const double *p2, size_t qty);


template <class T>
T L1Norm(const T* pVect1, const T* pVect2, size_t qty) {
    T res = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    while (pVect1 < pEnd1) {
        res += fabs(*pVect1++ - *pVect2++);
        res += fabs(*pVect1++ - *pVect2++);
        res += fabs(*pVect1++ - *pVect2++);
        res += fabs(*pVect1++ - *pVect2++);
    }

    while (pVect1 < pEnd2) {
        res += fabs(*pVect1++ - *pVect2++);
    }

    return res;
}

template float L1Norm<float>(const float* pVect1, const float* pVect2, size_t qty);
template double L1Norm<double>(const double* pVect1, const double* pVect2, size_t qty);
/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */

template <> 
float L1NormSIMD(const float* pVect1, const float* pVect2, size_t qty) {
#ifndef __SSE2__
#warning "L1NormSIMD<float>: SSE2 is not available, defaulting to pure C++ implementation!"
    return L1NormStandard(pVect1, pVect2, qty);
#else
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    __m128  diff, v1, v2; 
    __m128  sum = _mm_setzero_ps();

    /* 
     * A hack to quickly unset the sign flag.
     */
    __m128 mask_sign = reinterpret_cast<__m128>(_mm_set1_epi32(0x7fffffffu));


    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_and_ps(diff, mask_sign));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_and_ps(diff, mask_sign));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_and_ps(diff, mask_sign));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_and_ps(diff, mask_sign));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_and_ps(diff, mask_sign));
    }

    float __attribute__((aligned(16))) TmpRes[4];

    _mm_store_ps(TmpRes, sum);
    double res= TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    while (pVect1 < pEnd3) {
        res += fabs(*pVect1++ - *pVect2++);
    }

    return res;
#endif
}

template <> 
double L1NormSIMD(const double* pVect1, const double* pVect2, size_t qty) {
#ifndef __SSE2__
#warning "L1NormSIMD<double>: SSE2 is not available, defaulting to pure C++ implementation!"
    return L1NormStandard(pVect1, pVect2, qty);
#else
    size_t qty8 = qty/8;

    const double* pEnd1 = pVect1 + 8 * qty8;
    const double* pEnd2 = pVect1 + qty;

    __m128d  diff, v1, v2; 
    __m128d  sum = _mm_set1_pd(0); 


    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        sum  = _mm_add_pd(sum, _mm_max_pd(_mm_sub_pd(_mm_setzero_pd(), diff), diff));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        sum  = _mm_add_pd(sum, _mm_max_pd(_mm_sub_pd(_mm_setzero_pd(), diff), diff));

    }

    double __attribute__((aligned(16))) TmpRes[2];

    _mm_store_pd(TmpRes, sum);
    double res= TmpRes[0] + TmpRes[1];

    while (pVect1 < pEnd2) {
        // Leonid (@TODO) sometimes seg-faults in the unit test if float is replaced with double
        float diff = *pVect1++ - *pVect2++;
        res += fabs(diff);
    }

    return res;
}
#endif

template float L1NormSIMD<float>(const float* pVect1, const float* pVect2, size_t qty);
template double L1NormSIMD<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * L2-norm.
 */

template <class T>
T L2NormStandard(const T *p1, const T *p2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      T diff = (p1[i]-p2[i]);
      sum += diff * diff;
    }

    return sqrt(sum);
}

template  float L2NormStandard<float>(const float *p1, const float *p2, size_t qty);
template  double L2NormStandard<double>(const double *p1, const double *p2, size_t qty);


template <class T>
T L2Norm(const T* pVect1, const T* pVect2, size_t qty) {
    T res = 0, diff = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    while (pVect1 < pEnd1) {
        diff = *pVect1++ - *pVect2++; res += diff * diff;
        diff = *pVect1++ - *pVect2++; res += diff * diff;
        diff = *pVect1++ - *pVect2++; res += diff * diff;
        diff = *pVect1++ - *pVect2++; res += diff * diff;
    }

    while (pVect1 < pEnd2) {
        diff = *pVect1++ - *pVect2++; res += diff * diff;
    }

    return sqrt(res);
}

template float  L2Norm<float>(const float* pVect1, const float* pVect2, size_t qty);
template double L2Norm<double>(const double* pVect1, const double* pVect2, size_t qty);

/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */

float L2SqrSIMD(const float* pVect1, const float* pVect2, size_t qty) {
#ifndef __SSE2__
#warning "L2SqrSIMD<float>: SSE2 is not available, defaulting to pure C++ implementation!"
    float res = 0, diff;
    for (int i = 0; i < qty; ++i) {
        diff = pVect1[i] - pVect2[i];
        r += diff * diff;
    }
    return res;
#else
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    __m128  diff, v1, v2; 
    __m128  sum = _mm_set1_ps(0); 

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
    }

    float __attribute__((aligned(16))) TmpRes[4];

    _mm_store_ps(TmpRes, sum);
    float res= TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    while (pVect1 < pEnd3) {
        float diff = *pVect1++ - *pVect2++; 
        res += diff * diff;
    }

    return res;
#endif
}

template <> 
float L2NormSIMD(const float* pVect1, const float* pVect2, size_t qty) {
    float res = L2SqrSIMD(pVect1, pVect2, qty);
    return sqrt(res);
}

template <> 
double L2NormSIMD(const double* pVect1, const double* pVect2, size_t qty) {
#ifndef __SSE2__
#warning "L2NormSIMD<double>: SSE2 is not available, defaulting to pure C++ implementation!"
    return L2NormStandard(pVect1, pVect2, qty);
#else
    size_t qty8 = qty/8;

    const double* pEnd1 = pVect1 + 8 * qty8;
    const double* pEnd2 = pVect1 + qty;

    __m128d  diff, v1, v2; 
    __m128d  sum = _mm_set1_pd(0); 

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        sum  = _mm_add_pd(sum, _mm_mul_pd(diff, diff));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        sum  = _mm_add_pd(sum, _mm_mul_pd(diff, diff));
    }

    double __attribute__((aligned(16))) TmpRes[2];

    _mm_store_pd(TmpRes, sum);
    double res= TmpRes[0] + TmpRes[1];

    while (pVect1 < pEnd2) {
        double diff = *pVect1++ - *pVect2++; 
        res += diff * diff;
    }

    return sqrt(res);
#endif
}

template float  L2NormSIMD<float>(const float* pVect1, const float* pVect2, size_t qty);
template double L2NormSIMD<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * Slower versions of LP-distance
 */

template <typename T>
T LPGenericDistance(const T* x, const T* y, const int length, const T p) {
  T result = 0;
  T temp;

  CHECK(p > 0);

  for (int i = 0; i < length; ++i) {
    // In C++ 11, std::abs is also defined for floating-point numbers
    temp = std::abs(x[i] - y[i]);
    result += pow(temp, p);
  }

  return std::pow(result, T(1.0) / p);
}

template float LPGenericDistance<float>(const float* x, const float* y, const int length, const float p);
template double LPGenericDistance<double>(const double* x, const double* y, const int length, const double p);

/*
 * Exponentiation by square rooting and squaring:
 * a  hacky implementation that works faster than std::pow()
 * if p * 2^maxDig is an integer.
 */
template <typename T>
T LPGenericDistanceOptim(const T* x, const T* y, const int length, const T p) {
  T result = 0;
  T temp;


  CHECK(p > 0);

  constexpr unsigned maxDig  = 18;
  constexpr unsigned maxK = 1 << maxDig;

  unsigned pfm = floor(maxK * p);

  if (fabs(maxK*p - pfm) <= std::numeric_limits<T>::min()) {
    unsigned intPow    = pfm >> maxDig;
    unsigned fractPow  = pfm - (intPow << maxDig);

    if (!intPow) {
      for (int i = 0; i < length; ++i) {
        // In C++ 11, std::abs is also defined for floating-point numbers
        temp = std::abs(x[i] - y[i]);
        result += EfficientFractPowUtil(temp, fractPow, maxK);;
      }
    } else if (!fractPow) {
      for (int i = 0; i < length; ++i) {
        // In C++ 11, std::abs is also defined for floating-point numbers
        temp = std::abs(x[i] - y[i]);
        result += EfficientPow(temp, intPow);
      }
    } else {
      for (int i = 0; i < length; ++i) {
        // In C++ 11, std::abs is also defined for floating-point numbers
        temp = std::abs(x[i] - y[i]);
        result += EfficientPow(temp, intPow) * EfficientFractPowUtil(temp, fractPow, maxK);;
      }
    }
  } else {
    for (int i = 0; i < length; ++i) {
      // In C++ 11, std::abs is also defined for floating-point numbers
      temp = std::abs(x[i] - y[i]);
      result += pow(temp, p);
    }
  }

  return std::pow(result, T(1.0) / p);
}

template float LPGenericDistanceOptim<float>(const float* x, const float* y, const int length, const float p);
template double LPGenericDistanceOptim<double>(const double* x, const double* y, const int length, const double p);

}
