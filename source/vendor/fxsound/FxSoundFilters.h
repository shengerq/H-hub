/*
FxSound
Copyright (C) 2025  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <cmath>
// FxSound filtCalcParametric and filtBW2ANGLE, adapted to a dependency-free
// double-precision host adapter. Algorithms and original attribution retained.
namespace hub::fxsound {
using realtype=double;using biqdRealtype=double;
inline constexpr double PI=3.14159265358979323846,SPN=1.65436e-24;
inline constexpr int IS_FALSE=0,IS_TRUE=1,OKAY=0;
struct filt2ndOrderBoostCutShelfFilterType {double r_samp_freq{},r_center_freq{},boost{},Q{},b0=1,b1{},b2{},a1{},a2{};int section_on_flag=0;};
inline biqdRealtype filtBW2ANGLE(biqdRealtype a, biqdRealtype bandwidth)
{	
	biqdRealtype T, d, sn, cs, mag, delta, theta, tmp, a2, a4, asnd;
	
	T = (biqdRealtype)tan((biqdRealtype)(2*PI)*bandwidth);
	a2 = a*a;
	a4 = a2 * a2;
	d = 2*a2*T;
	sn = (1+a4)*T;
	cs = (1-a4);
	mag = (biqdRealtype)sqrt(sn*sn + cs*cs);
	d /= mag;
	delta = (biqdRealtype)atan2(sn, cs);

	asnd = (biqdRealtype)asin(d);
	
	/* Note- fix hi freq low Q problems */
	theta = (biqdRealtype)0.5 * ((biqdRealtype)PI - asnd - delta);	/* Bandedge for prototype */
	tmp = (biqdRealtype)0.5 * (asnd - delta);			/* Take principal branch */
	if (tmp > (biqdRealtype)0.0 && tmp < theta) 
		theta = tmp;
		
    /* Max out returned theta for this case - hi freq, hi Q */
    if ( (bandwidth >= (biqdRealtype)0.5) )		
    	theta = (biqdRealtype)0.005;
    	
	return(theta/(biqdRealtype)(2*PI));				/* Return norm frequency */
}
inline int filtCalcParametric(struct filt2ndOrderBoostCutShelfFilterType *f)
{
  biqdRealtype bandwidth;
  biqdRealtype a, A, F, xfmbw, C, tmp, alphan, alphad, a0, recipa0, asq, F2, a2plus1, ma2plus1;
  biqdRealtype center_freq;

  /* Note that for 0.0 boost, design routine sends back non-zero b1,b2,a1,a2 coeffs that cause
   * numerator and denominator to cancel. Overridding these with 1.0, 0, 0 ...doesn't
   * work because it causes boinks. If cancelling coeffs are sent as is, hi Q settings
   * cause problems at low freqs/mid freqs. (as high as 440) because of precision
   * limitations. To minimize the boinks and precision probs., we warp Q settings with
   * the boost below 3db to at 0 boost we have a Q of 0.2.
   */
#ifdef FILT_COEFF_ZERO   
  if( boost == 0.0 )
  {
  	 *b0 = 1.0;
  	 f->b1 = f->b2 = f->a1 = f->a2 = 0.0;
  }
  else
#endif //FILT_COEFF_ZERO  

  /* If boost is zero, set the section off */
  if( f->boost == 0.0 )
  {
    f->section_on_flag = IS_FALSE;
  	 f->b0 = 1.0;
  	 f->b1 = f->b2 = f->a1 = f->a2 = 0.0;
  }
  else /* Section on */
  {
   /* For now, because of low frequency accuracy problems, limit Q at low freqs.
    * Limit to Q of 1 at 20 hz, starting taper at FILT_Q_LIMIT_FREQ hz
    */
    realtype maxQ, abs_boost;     
     
	/* Local definitions (used in this function only) */
	#define FILT_Q_UPPER_LIMIT_FREQ (realtype)60.0     
	#define FILT_Q_LOWER_LIMIT_FREQ (realtype)20.0     
	#define FILT_Q_UPPER_LIMIT (realtype)20.0     
	#define FILT_Q_LOWER_LIMIT (realtype)1.0     
	#define FILT_Q_LIMIT_SCALE (FILT_Q_UPPER_LIMIT - FILT_Q_LOWER_LIMIT)/(FILT_Q_UPPER_LIMIT_FREQ - FILT_Q_LOWER_LIMIT_FREQ)
	
	#define FILT_BOOST_WARP_LEVEL (realtype)6.0     
	#define FILT_BOOST_MAX_Q (realtype)20.0     
	#define FILT_BOOST_MIN_Q (realtype)0.2     
	#define FILT_BOOST_SCALE (FILT_BOOST_MAX_Q - FILT_BOOST_MIN_Q)/(FILT_BOOST_WARP_LEVEL)

    f->section_on_flag = IS_TRUE;

	/* Limit low freq Q */
    if( f->r_center_freq < FILT_Q_UPPER_LIMIT_FREQ )
    {
    	maxQ = (f->r_center_freq - FILT_Q_LOWER_LIMIT_FREQ) * FILT_Q_LIMIT_SCALE
    		   + FILT_Q_LOWER_LIMIT;
        if( f->Q > maxQ )
        	f->Q = maxQ;
    }
    			
	/* Limit low boost Q */
    abs_boost = (realtype)fabs(f->boost);
    if( abs_boost < FILT_BOOST_WARP_LEVEL )
    {
    	maxQ = abs_boost * FILT_BOOST_SCALE
    		   + FILT_BOOST_MIN_Q;
        if( f->Q > maxQ )
        	f->Q = maxQ;
    }

    center_freq = f->r_center_freq/f->r_samp_freq;
    bandwidth = center_freq/f->Q;

	a = (biqdRealtype)tan( (biqdRealtype)PI*(center_freq - (biqdRealtype)0.25) );		/* Warp factor */
	asq = a*a;
	A = (biqdRealtype)pow((biqdRealtype)10.0, (biqdRealtype)f->boost/(biqdRealtype)20.0);		/* Cvrt dB to factor */
	if (f->boost < (realtype)6.0 && f->boost > (realtype)-6.0) 
		F = (biqdRealtype)sqrt(A);
	else if (A > (biqdRealtype)1.0) 
		F = A/(biqdRealtype)sqrt(2.0);
	else F = A * (biqdRealtype)sqrt(2.0);
		/* If |boost/cut| < 6dB, then doesn't make sense to use 3dB pt.
		   use of root makes bandedge at half the boost/cut amount
		 */
	xfmbw = filtBW2ANGLE(a, bandwidth);

	C = (biqdRealtype)1.0 / (biqdRealtype)tan((biqdRealtype)(2*PI)*xfmbw);	/* co-tangent of angle */
	F2 = F*F;
	tmp = A*A - F2;
	if (fabs(tmp) <= SPN)
		alphad = C;
	else 
		alphad = (biqdRealtype)sqrt( C*C * (F2 - (biqdRealtype)1.0) / tmp);
	alphan = A*alphad;

	a2plus1 = (biqdRealtype)1.0 + asq;
	ma2plus1 = (biqdRealtype)1.0 - asq;
	f->b0 = (realtype)(a2plus1 + alphan*ma2plus1);
	f->b1 = (realtype)((biqdRealtype)4.0*a);
	f->b2 = (realtype)(a2plus1 - alphan*ma2plus1);

	a0 = (realtype)(a2plus1 + alphad*ma2plus1);
	f->a2 = (realtype)(a2plus1 - alphad*ma2plus1);

	/* Normalize a0 */
	recipa0 = (biqdRealtype)1.0/(biqdRealtype)a0;
	f->b0 = (realtype)((biqdRealtype)f->b0 * recipa0);
	f->b1 = (realtype)((biqdRealtype)f->b1 * recipa0);
	f->b2 = (realtype)((biqdRealtype)f->b2 * recipa0);
	f->a1 = f->b1;
	f->a2 = (realtype)((biqdRealtype)f->a2 * recipa0);
  }
		
  return(OKAY);
}
} // namespace hub::fxsound
