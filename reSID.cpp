/*
	Arduino SID
	Copyright (c) 2015 Frank Bösing
	This library is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with this library.  If not, see <http://www.gnu.org/licenses/>.
	
	Diese Bibliothek ist freie Software: Sie können es unter den Bedingungen
	der GNU General Public License, wie von der Free Software Foundation,
	Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
	veröffentlichten Version, weiterverbreiten und/oder modifizieren.
	Diese Bibliothek wird in der Hoffnung, dass es nützlich sein wird, aber
	OHNE JEDE GEWÄHRLEISTUNG, bereitgestellt; sogar ohne die implizite
	Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
	Siehe die GNU General Public License für weitere Details.
	Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
	Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.

 */
#include "reSID.h"
#include <AudioStream.h>

#define OVERSAMPLE 23

static SID sid;

void AudioPlaySID::begin(void)
{
	this->reset();
	 sid.set_sampling_parameters(OVERSAMPLE * AUDIO_SAMPLE_RATE_EXACT, SAMPLE_FAST, AUDIO_SAMPLE_RATE_EXACT); 
	//sid.set_chip_model(MOS6581);
	//sid.set_chip_model(MOS8580); 
	playing = true;
}

void AudioPlaySID::reset(void)
{
	sid.reset();
}

void AudioPlaySID::stop(void)
{
	__disable_irq();
	playing = false;	
	__enable_irq();
}

//inline 
void AudioPlaySID::setreg(int ofs, int val) {
	sid.write(ofs, val);
}

void AudioPlaySID::update(void) {
unsigned int i;
	audio_block_t *block;

	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

	for (i=0;  i< AUDIO_BLOCK_SAMPLES; i++) {
			sid.clock(OVERSAMPLE);
			block->data[i] = sid.output();
	}

	transmit(block);
	release(block);
}
