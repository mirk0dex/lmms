/*
 * basic_filters.h - simple but powerful filter-class with most used filters
 *
 * original file by ??? 
 * modified and enhanced by Tobias Doerffel
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _BASIC_FILTERS_H
#define _BASIC_FILTERS_H

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>

#include "lmms_basics.h"
#include "mixer.h"
#include "templates.h"
#include "lmms_constants.h"

//#include <iostream>
//#include <cstdlib>

template<ch_cnt_t CHANNELS/* = DEFAULT_CHANNELS*/>
class basicFilters
{
public:
	enum FilterTypes
	{
		LowPass,
		HiPass,
		BandPass_CSG,
		BandPass_CZPG,
		Notch,
		AllPass,
		Moog,
		DoubleLowPass,
		Lowpass_RC,
		Bandpass_RC,
		Highpass_RC,
		NumFilters
	} ;

	static inline float minFreq()
	{
		return( 0.01f );
	}

	static inline float minQ()
	{
		return( 0.01f );
	}

	inline void setFilterType( const int _idx )
	{
		m_doubleFilter = _idx == DoubleLowPass;
		if( !m_doubleFilter )
		{
			m_type = static_cast<FilterTypes>( _idx );
			return;
		}

		// Double lowpass mode, backwards-compat for the goofy
		// Add-NumFilters to signify doubleFilter stuff
		m_type = static_cast<FilterTypes>( LowPass );
		if( m_subFilter == NULL )
		{
			m_subFilter = new basicFilters<CHANNELS>(
						static_cast<sample_rate_t>(
							m_sampleRate ) );
		}
		m_subFilter->m_type = m_type;
	}

	inline basicFilters( const sample_rate_t _sample_rate ) :
		m_b0a0( 0.0f ),
		m_b1a0( 0.0f ),
		m_b2a0( 0.0f ),
		m_a1a0( 0.0f ),
		m_a2a0( 0.0f ),
		m_rca( 0.0f ),
		m_rcb( 1.0f ),
		m_rcc( 0.0f ),
		m_doubleFilter( false ),
		m_sampleRate( (float) _sample_rate ),
		m_subFilter( NULL )
	{
		clearHistory();
	}

	inline ~basicFilters()
	{
		delete m_subFilter;
	}

	inline void clearHistory()
	{
		// reset in/out history
		for( ch_cnt_t _chnl = 0; _chnl < CHANNELS; ++_chnl )
		{
			// reset in/out history for simple filters
			m_ou1[_chnl] = m_ou2[_chnl] = m_in1[_chnl] =
					m_in2[_chnl] = 0.0f;
					
			// reset in/out historey for moog-filter
			m_y1[_chnl] = m_y2[_chnl] = m_y3[_chnl] = m_y4[_chnl] =
					m_oldx[_chnl] = m_oldy1[_chnl] =
					m_oldy2[_chnl] = m_oldy3[_chnl] = 0.0f;
					
			// reset in/out historey for RC-filter
			m_rclp[_chnl] = m_rcbp[_chnl] = m_rchp[_chnl] = m_rclast[_chnl] = 0.0f;
		}
	}

	inline sample_t update( sample_t _in0, ch_cnt_t _chnl )
	{
		sample_t out;
		switch( m_type )
		{
			case Moog:
			{
				sample_t x = _in0 - m_r*m_y4[_chnl];

				// four cascaded onepole filters
				// (bilinear transform)
				m_y1[_chnl] = tLimit(
						( x + m_oldx[_chnl] ) * m_p
							- m_k * m_y1[_chnl],
								-10.0f, 10.0f );
				m_y2[_chnl] = tLimit(
					( m_y1[_chnl] + m_oldy1[_chnl] ) * m_p
							- m_k * m_y2[_chnl],
								-10.0f, 10.0f );
				m_y3[_chnl] = tLimit(
					( m_y2[_chnl] + m_oldy2[_chnl] ) * m_p
							- m_k * m_y3[_chnl],
								-10.0f, 10.0f );
				m_y4[_chnl] = tLimit(
					( m_y3[_chnl] + m_oldy3[_chnl] ) * m_p
							- m_k * m_y4[_chnl],
								-10.0f, 10.0f );

				m_oldx[_chnl] = x;
				m_oldy1[_chnl] = m_y1[_chnl];
				m_oldy2[_chnl] = m_y2[_chnl];
				m_oldy3[_chnl] = m_y3[_chnl];
				out = m_y4[_chnl] - m_y4[_chnl] * m_y4[_chnl] *
						m_y4[_chnl] * ( 1.0f / 6.0f );
				break;
			}


			// 4-times oversampled simulation of an active RC-Bandpass,-Lowpass,-Highpass-
			// Filter-Network as it was used in nearly all modern analog synthesizers. This
			// can be driven up to self-oscillation (BTW: do not remove the limits!!!).
			// (C) 1998 ... 2009 S.Fendt. Released under the GPL v2.0  or any later version.

			case Lowpass_RC:
			case Bandpass_RC:
			case Highpass_RC:
			{
				sample_t lp, hp, bp;

				sample_t in;

				// 4-times oversampled... (even the moog would benefit from this)
				for( int n = 4; n != 0; --n )
				{
					in = _in0 + m_rcbp[_chnl] * m_rcq;
					in = (in > +1.f) ? +1.f : in;
					in = (in < -1.f) ? -1.f : in;

					lp = in * m_rcb + m_rclp[_chnl] * m_rca;
					lp = (lp > +1.f) ? +1.f : lp;
					lp = (lp < -1.f) ? -1.f : lp;

					hp = m_rcc * ( m_rchp[_chnl] + in - m_rclast[_chnl] );
					hp = (hp > +1.f) ? +1.f : hp;
					hp = (hp < -1.f) ? -1.f : hp;

					bp = hp * m_rcb + m_rcbp[_chnl] * m_rca;
					bp = (bp > +1.f) ? +1.f : bp;
					bp = (bp < -1.f) ? -1.f : bp;

					m_rclast[_chnl] = in;
					m_rclp[_chnl] = lp;
					m_rchp[_chnl] = hp;
					m_rcbp[_chnl] = bp;

				}

				if( m_type == Lowpass_RC )
					out = lp;
				else if( m_type == Bandpass_RC )
					out = bp;
				else
					out = hp;
				
				return( out );
				break;
			}


			default:
				// filter
				out = m_b0a0*_in0 +
						m_b1a0*m_in1[_chnl] +
						m_b2a0*m_in2[_chnl] -
						m_a1a0*m_ou1[_chnl] -
						m_a2a0*m_ou2[_chnl];

				// push in/out buffers
				m_in2[_chnl] = m_in1[_chnl];
				m_in1[_chnl] = _in0;
				m_ou2[_chnl] = m_ou1[_chnl];

				m_ou1[_chnl] = out;
				break;
		}

		if( m_doubleFilter )
		{
			return m_subFilter->update( out, _chnl );
		}

		// Clipper band limited sigmoid
		return out;
	}


	inline void calcFilterCoeffs( float _freq, float _q
				/*, const bool _q_is_bandwidth = false*/ )
	{
		// temp coef vars
		_freq = qMax( _freq, minFreq() );// limit freq and q for not getting
					      // bad noise out of the filter...
		_q = qMax( _q, minQ() );

		if( m_type == Lowpass_RC  ||
				m_type == Bandpass_RC ||
				m_type == Highpass_RC )
		{
			if( _freq < 50.f )
			{
				_freq = 50.f;
			}
		
			m_rca = 1.0f - (1.0f/(m_sampleRate*4)) / ( (1.0f/(_freq*2.0f*M_PI)) + (1.0f/(m_sampleRate*4)) );
			m_rcb = 1.0f - m_rca;
			m_rcc = (1.0f/(_freq*2.0f*M_PI)) / ( (1.0f/(_freq*2.0f*M_PI)) + (1.0f/(m_sampleRate*4)) );
			
			// Stretch Q/resonance, as self-oscillation reliably starts at a q of ~2.5 - ~2.6
			m_rcq = _q/4.f;
		}
		
		if( m_type == Moog )
		{
			// [ 0 - 0.5 ]
			const float f = _freq / m_sampleRate;
			// (Empirical tunning)
			m_p = ( 3.6f - 3.2f * f ) * f;
			m_k = 2.0f * m_p - 1;
			m_r = _q * powf( M_E, ( 1 - m_p ) * 1.386249f );

			if( m_doubleFilter )
			{
				m_subFilter->m_r = m_r;
				m_subFilter->m_p = m_p;
				m_subFilter->m_k = m_k;
			}
			return;
		}

		// other filters
		const float omega = F_2PI * _freq / m_sampleRate;
		const float tsin = sinf( omega );
		const float tcos = cosf( omega );
		//float alpha;

		//if (q_is_bandwidth)
		//alpha = tsin*sinhf(logf(2.0f)/2.0f*q*omega/
		//					tsin);
		//else
		const float alpha = 0.5f * tsin / _q;

		const float a0 = 1.0f / ( 1.0f + alpha );

		m_a1a0 = -2.0f * tcos * a0;
		m_a2a0 = ( 1.0f - alpha ) * a0;

		switch( m_type )
		{
			case LowPass:
				m_b1a0 = ( 1.0f - tcos ) * a0;
				m_b0a0 = m_b1a0 * 0.5f;
				m_b2a0 = m_b0a0;//((1.0f-tcos)/2.0f)*a0;
				break;
			case HiPass:
				m_b1a0 = ( -1.0f - tcos ) * a0;
				m_b0a0 = m_b1a0 * -0.5f;
				m_b2a0 = m_b0a0;//((1.0f+tcos)/2.0f)*a0;
				break;
			case BandPass_CSG:
				m_b1a0 = 0.0f;
				m_b0a0 = tsin * 0.5f * a0;
				m_b2a0 = -m_b0a0;
				break;
			case BandPass_CZPG:
				m_b1a0 = 0.0f;
				m_b0a0 = alpha * a0;
				m_b2a0 = -m_b0a0;
				break;
			case Notch:
				m_b1a0 = m_a1a0;
				m_b0a0 = a0;
				m_b2a0 = a0;
				break;
			case AllPass:
				m_b1a0 = m_a1a0;
				m_b0a0 = m_a2a0;
				m_b2a0 = 1.0f;//(1.0f+alpha)*a0;
				break;
			default:
				break;
		}

		if( m_doubleFilter )
		{
			m_subFilter->m_b0a0 = m_b0a0;
			m_subFilter->m_b1a0 = m_b1a0;
			m_subFilter->m_b2a0 = m_b2a0;
			m_subFilter->m_a1a0 = m_a1a0;
			m_subFilter->m_a2a0 = m_a2a0;
		}
	}


private:
	// filter coeffs
	float m_b0a0, m_b1a0, m_b2a0, m_a1a0, m_a2a0;

	// coeffs for moog-filter
	float m_r, m_p, m_k;

	// coeffs for RC-type-filters
	float m_rca, m_rcb, m_rcc, m_rcq;

	typedef sample_t frame[CHANNELS];

	// in/out history
	frame m_ou1, m_ou2, m_in1, m_in2;

	// in/out history for moog-filter
	frame m_y1, m_y2, m_y3, m_y4, m_oldx, m_oldy1, m_oldy2, m_oldy3;
	
	// in/out history for RC-type-filters
	frame m_rcbp, m_rclp, m_rchp, m_rclast;

	FilterTypes m_type;
	bool m_doubleFilter;

	float m_sampleRate;
	basicFilters<CHANNELS> * m_subFilter;

} ;


#endif
