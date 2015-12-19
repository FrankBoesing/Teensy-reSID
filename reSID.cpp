
#include "reSID.h"
#include <AudioStream.h>

#define OVERSAMPLE 24

static SID sid;

void AudioPlaySID::begin(void)
{
	this->reset();
	 sid.set_sampling_parameters(OVERSAMPLE * 44117, SAMPLE_FAST, 44117); 
	//sid.set_chip_model(MOS6581);
	sid.set_chip_model(MOS8580);
 
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

void AudioPlaySID::setreg(int ofs, int val) {
	sid.write(ofs, val);
}

void AudioPlaySID::update(void) {
unsigned int i, n;
	audio_block_t *block;

	static cycle_count delta_t;
	
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
