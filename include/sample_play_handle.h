/*
 * sample_play_handle.h - play-handle for playing a sample
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _SAMPLE_PLAY_HANDLE_H
#define _SAMPLE_PLAY_HANDLE_H

#include "play_handle.h"
#include "types.h"


class sampleBuffer;
class audioPort;


class samplePlayHandle : public playHandle
{
public:
	samplePlayHandle( const QString & _sample_file );
	samplePlayHandle( sampleBuffer  * _sample_buffer );
	virtual ~samplePlayHandle();

	virtual void play( void );
	virtual bool done( void ) const;

	Uint32 totalFrames( void ) const;
	inline Uint32 framesDone( void ) const
	{
		return( m_frame );
	}
	void setDoneMayReturnTrue( bool _enable )
	{
		m_doneMayReturnTrue = _enable;
	}


private:
	sampleBuffer * m_sampleBuffer;
	const bool m_ownSampleBuffer;
	bool m_doneMayReturnTrue;

	Uint32 m_frame;

	audioPort * m_audioPort;

} ;


#endif
