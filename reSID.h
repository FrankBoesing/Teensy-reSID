
#include "sid.h"

#ifndef play_sid_h_
#define play_sid_h_

#include <AudioStream.h>

class AudioPlaySID : public AudioStream
{
public:
	AudioPlaySID(void) : AudioStream(0, NULL) { begin(); }
	void begin(void);
	void setreg(int ofs, int val);
	void reset(void);
	void stop(void);
	bool isPlaying(void) { return playing; }	
	virtual void update(void);	
private:
	volatile bool playing;
};


#endif
