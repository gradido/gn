#ifndef _GRADIDO_MATH_H
#define _GRADIDO_MATH_H

#include "mpfr.h"

#ifdef __cplusplus
extern "C" {
#endif

struct S_GradidoWithDecimal
{
	long long gradido;
	long decimal; // / 1,000,000
};

// 1 second = 1×10^9 nanoseconds

#define GRADIDO_DECIMAL_CONVERSION_FACTOR 1000000

typedef struct S_GradidoWithDecimal GradidoWithDecimal;

//! \param decay_factor to store the result, decay factor per second
//! \param days_per_year 365 days in normal year, 366 days in leap year
//! \brief calculate decay factor per second for exponential decay calculation, needs 100-400 micro seconds depending on processor
/*!
 *  calculate decay factor with compound interest formula converted to q <br>
 *  n = (lg Kn - lg K0) / lg q => <br>
 *  lg q = (lg Kn - lg K0) / n => <br>
 *  q = e^((lg Kn - lg K0) / n)   <br>
 * <br>
 * with:
 * <ul>
 *  <li>q = decay_factor</li>
 *  <li>n = days_per_year * 60 * 60 * 24 = seconds per year</li>
 *  <li>Kn = 50 (capital after a year)</li>
 *  <li>K0 = 100 (capital at start)</li>
 * </ul>
 * 
 */
void calculateDecayFactor(mpfr_ptr decay_factor, int days_per_year);

//! \param decay_for_duration store the result, decay factor for specific duration
//! \param decay_factor per second from ::calculateDecayFactor
//! \param seconds duration in seconds
//! \brief calculate decay_for_duration = decay_factor^seconds
/*!
 * calculate the decay factor for the specific duration
 */
void calculateDecayFactorForDuration(mpfr_ptr decay_for_duration, mpfr_ptr decay_factor, unsigned long seconds);

//! \param decay_for_duration decay factor for specific duration, taken from ::calculateDecayFactorForDuration
//! \param gradido_decimal gradido decimal, already as decimal < 1, for saving more accuracy for decay, contain updated value after function call
//! \param gradido_cent as mpz, contain new, reduced value after function call
//! \param temp only for storing between result, can reuse afterwards for another things
//! \brief calculate decayed balance without memory allocation
/*!
 * calculate decay with large precision: <br>
 * temp = (gradido_decimal + gradido_cent) * decay_for_duration <br>
 * gradido_cent = round(temp) <br>
 * gradido_decimal = temp - gradido_cent <br>
 */
void calculateDecayFast(mpfr_ptr decay_for_duration, mpfr_ptr gradido_decimal, mpz_ptr gradido_cent, mpfr_ptr temp);

//! \param decay_for_duration decay factor for specific duration, taken from ::calculateDecayFactorForDuration
//! \param input Gradido cent and decimal in on struct, will be converted before calculating
//! \brief calculate decayed balance for specific time duration, allocate memory 
//! \return decayed gradido balance
/*!
 * convert input.gradido to gradido_cent:mpz_t and input.decimal / GRADIDO_DECIMAL_CONVERSION_FACTOR to gradido_decimal:mpfr_t
 * call calculateDecayFast
 * convert result back to GradidoWithDecimal struct
 */
GradidoWithDecimal calculateDecayForDuration(mpfr_ptr decay_for_duration, GradidoWithDecimal input);

//! \param input Gradido cent and decimal in on struct, will be converted before calculating
//! \param seconds duration in seconds
//! \param decay_factor per second from ::calculateDecayFactor
//! \brief call ::calculateDecayFactorForDuration and ::calculateDecayForDuration, allocate memory 
//! \return decayed gradido balance
GradidoWithDecimal calculateDecay(GradidoWithDecimal input, unsigned long seconds, mpfr_ptr decay_factor);

#ifdef __cplusplus
}
#endif

#endif //_GRADIDO_MATH_H
