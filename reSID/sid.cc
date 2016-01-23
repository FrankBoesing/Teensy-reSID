//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2004  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#include "sid.h"
#include <math.h>
#include <AudioStream.h>
RESID_NAMESPACE_START

// Resampling constants.
// The error in interpolated lookup is bounded by 1.234/L^2,
// while the error in non-interpolated lookup is bounded by
// 0.7854/L + 0.4113/L^2, see
// http://www-ccrma.stanford.edu/~jos/resample/Choice_Table_Size.html
// For a resolution of 16 bits this yields L >= 285 and L >= 51473,
// respectively.
const int SID::FIR_N = 125;
const int SID::FIR_RES_INTERPOLATE = 285;
const int SID::FIR_RES_FAST = 51473;
const int SID::FIR_SHIFT = 15;
const int SID::RINGSIZE = 16384;

// Fixpoint constants (16.16 bits).
const int SID::FIXP_SHIFT = 16;
const int SID::FIXP_MASK = 0xffff;

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
SID::SID()
{

  voice[0].set_sync_source(&voice[2]);
  voice[1].set_sync_source(&voice[0]);
  voice[2].set_sync_source(&voice[1]);

  set_sampling_parameters(985248, SAMPLE_FAST, AUDIO_SAMPLE_RATE_EXACT);  

  bus_value = 0;
  bus_value_ttl = 0;

  ext_in = 0;

}
/*
void SID::printFilter(void){
		Serial.print(filter.f0_count);
		for (int i=0; i< 2048; i++) {
				if (i % 16==0) Serial.println();
				Serial.printf("0x%04X, ",filter.f0_6581[i]);
		}
}
*/
// ----------------------------------------------------------------------------
// Destructor.
// ----------------------------------------------------------------------------
SID::~SID()
{
}


// ----------------------------------------------------------------------------
// Set chip model.
// ----------------------------------------------------------------------------
/*
void SID::set_chip_model(chip_model model)
{
  voice[0].set_chip_model(model);
  voice[1].set_chip_model(model);
  voice[2].set_chip_model(model);

  filter.set_chip_model(model);
  extfilt.set_chip_model(model);
}
*/

// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void SID::reset()
{

	voice[0].reset();
	voice[1].reset();
	voice[2].reset();
	
  filter.reset();
  extfilt.reset();

  bus_value = 0;
  bus_value_ttl = 0;
}


// ----------------------------------------------------------------------------
// Write 16-bit sample to audio input.
// NB! The caller is responsible for keeping the value within 16 bits.
// Note that to mix in an external audio signal, the signal should be
// resampled to 1MHz first to avoid sampling noise.
// ----------------------------------------------------------------------------
void SID::input(int sample)
{
  // Voice outputs are 20 bits. Scale up to match three voices in order
  // to facilitate simulation of the MOS8580 "digi boost" hardware hack.
  ext_in = (sample << 4)*3;
}

// ----------------------------------------------------------------------------
// Read sample from audio output.
// ----------------------------------------------------------------------------
int SID::output()
{
  const int range = 1 << 16;
  //const int half = range >> 1;
  int sample = extfilt.output()/((4095*255 >> 7)*3*15*2/range);
	
	asm ("ssat %0, #16, %1" : "=r" (sample) : "r" (sample));
	/*
  if (sample >= half) {
    return half - 1;
  }
  if (sample < -half) {
    return -half;
  }
	*/
  return sample;
}


// ----------------------------------------------------------------------------
// Read registers.
//
// Reading a write only register returns the last byte written to any SID
// register. The individual bits in this value start to fade down towards
// zero after a few cycles. All bits reach zero within approximately
// $2000 - $4000 cycles.
// It has been claimed that this fading happens in an orderly fashion, however
// sampling of write only registers reveals that this is not the case.
// NB! This is not correctly modeled.
// The actual use of write only registers has largely been made in the belief
// that all SID registers are readable. To support this belief the read
// would have to be done immediately after a write to the same register
// (remember that an intermediate write to another register would yield that
// value instead). With this in mind we return the last value written to
// any SID register for $2000 cycles without modeling the bit fading.
// ----------------------------------------------------------------------------
reg8 SID::read(reg8 offset)
{
  switch (offset) {
  case 0x19:
    return potx.readPOT();
  case 0x1a:
    return poty.readPOT();
  case 0x1b:
    return voice[2].wave.readOSC();
  case 0x1c:
    return voice[2].envelope.readENV();
  default:
    return bus_value;
  }
}


// ----------------------------------------------------------------------------
// Write registers.
// ----------------------------------------------------------------------------
void SID::write(reg8 offset, reg8 value)
{
  bus_value = value;
  bus_value_ttl = 0x2000;

  switch (offset) {
  case 0x00:
    voice[0].wave.writeFREQ_LO(value);
    break;
  case 0x01:
    voice[0].wave.writeFREQ_HI(value);
    break;
  case 0x02:
    voice[0].wave.writePW_LO(value);
    break;
  case 0x03:
    voice[0].wave.writePW_HI(value);
    break;
  case 0x04:
    voice[0].writeCONTROL_REG(value);
    break;
  case 0x05:
    voice[0].envelope.writeATTACK_DECAY(value);
    break;
  case 0x06:
    voice[0].envelope.writeSUSTAIN_RELEASE(value);
    break;
  case 0x07:
    voice[1].wave.writeFREQ_LO(value);
    break;
  case 0x08:
    voice[1].wave.writeFREQ_HI(value);
    break;
  case 0x09:
    voice[1].wave.writePW_LO(value);
    break;
  case 0x0a:
    voice[1].wave.writePW_HI(value);
    break;
  case 0x0b:
    voice[1].writeCONTROL_REG(value);
    break;
  case 0x0c:
    voice[1].envelope.writeATTACK_DECAY(value);
    break;
  case 0x0d:
    voice[1].envelope.writeSUSTAIN_RELEASE(value);
    break;
  case 0x0e:
    voice[2].wave.writeFREQ_LO(value);
    break;
  case 0x0f:
    voice[2].wave.writeFREQ_HI(value);
    break;
  case 0x10:
    voice[2].wave.writePW_LO(value);
    break;
  case 0x11:
    voice[2].wave.writePW_HI(value);
    break;
  case 0x12:
    voice[2].writeCONTROL_REG(value);
    break;
  case 0x13:
    voice[2].envelope.writeATTACK_DECAY(value);
    break;
  case 0x14:
    voice[2].envelope.writeSUSTAIN_RELEASE(value);
    break;
  case 0x15:
    filter.writeFC_LO(value);
    break;
  case 0x16:
    filter.writeFC_HI(value);
    break;
  case 0x17:
    filter.writeRES_FILT(value);
    break;
  case 0x18:
    filter.writeMODE_VOL(value);
    break;
  default:
    break;
  }
}


// ----------------------------------------------------------------------------
// SID voice muting.
// ----------------------------------------------------------------------------
void SID::mute(reg8 channel, bool enable)
{
  // Only have 3 voices!
  if (channel >= 3)
    return;

  voice[channel].mute (enable);
}
  

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
SID::State::State()
{
  int i;

  for (i = 0; i < 0x20; i++) {
    sid_register[i] = 0;
  }

  bus_value = 0;
  bus_value_ttl = 0;

  for (i = 0; i < 3; i++) {
    accumulator[i] = 0;
    shift_register[i] = 0x7ffff8;
    rate_counter[i] = 0;
    rate_counter_period[i] = 9;
    exponential_counter[i] = 0;
    exponential_counter_period[i] = 1;
    envelope_counter[i] = 0;
    envelope_state[i] = EnvelopeGenerator::RELEASE;
    hold_zero[i] = true;
  }
}


// ----------------------------------------------------------------------------
// Read state.
// ----------------------------------------------------------------------------
SID::State SID::read_state()
{
  State state;
  int i, j;

  for (i = 0, j = 0; i < 3; i++, j += 7) {
    WaveformGenerator& wave = voice[i].wave;
    EnvelopeGenerator& envelope = voice[i].envelope;
    state.sid_register[j + 0] = wave.freq & 0xff;
    state.sid_register[j + 1] = wave.freq >> 8;
    state.sid_register[j + 2] = wave.pw & 0xff;
    state.sid_register[j + 3] = wave.pw >> 8;
    state.sid_register[j + 4] =
      (wave.waveform << 4)
      | (wave.test ? 0x08 : 0)
      | (wave.ring_mod ? 0x04 : 0)
      | (wave.sync ? 0x02 : 0)
      | (envelope.gate ? 0x01 : 0);
    state.sid_register[j + 5] = (envelope.attack << 4) | envelope.decay;
    state.sid_register[j + 6] = (envelope.sustain << 4) | envelope.release;
  }

  state.sid_register[j++] = filter.fc & 0x007;
  state.sid_register[j++] = filter.fc >> 3;
  state.sid_register[j++] = (filter.res << 4) | filter.filt;
  state.sid_register[j++] =
    (filter.voice3off ? 0x80 : 0)
    | (filter.hp_bp_lp << 4)
    | filter.vol;

  // These registers are superfluous, but included for completeness.
  for (; j < 0x1d; j++) {
    state.sid_register[j] = read(j);
  }
  for (; j < 0x20; j++) {
    state.sid_register[j] = 0;
  }

  state.bus_value = bus_value;
  state.bus_value_ttl = bus_value_ttl;

  for (i = 0; i < 3; i++) {
    state.accumulator[i] = voice[i].wave.accumulator;
    state.shift_register[i] = voice[i].wave.shift_register;
    state.rate_counter[i] = voice[i].envelope.rate_counter;
    state.rate_counter_period[i] = voice[i].envelope.rate_period;
    state.exponential_counter[i] = voice[i].envelope.exponential_counter;
    state.exponential_counter_period[i] = voice[i].envelope.exponential_counter_period;
    state.envelope_counter[i] = voice[i].envelope.envelope_counter;
    state.envelope_state[i] = voice[i].envelope.state;
    state.hold_zero[i] = voice[i].envelope.hold_zero;
  }

  return state;
}


// ----------------------------------------------------------------------------
// Write state.
// ----------------------------------------------------------------------------
void SID::write_state(const State& state)
{
  int i;

  for (i = 0; i <= 0x18; i++) {
    write(i, state.sid_register[i]);
  }

  bus_value = state.bus_value;
  bus_value_ttl = state.bus_value_ttl;

  for (i = 0; i < 3; i++) {
    voice[i].wave.accumulator = state.accumulator[i];
    voice[i].wave.shift_register = state.shift_register[i];
    voice[i].envelope.rate_counter = state.rate_counter[i];
    voice[i].envelope.rate_period = state.rate_counter_period[i];
    voice[i].envelope.exponential_counter = state.exponential_counter[i];
    voice[i].envelope.exponential_counter_period = state.exponential_counter_period[i];
    voice[i].envelope.envelope_counter = state.envelope_counter[i];
    voice[i].envelope.state = state.envelope_state[i];
    voice[i].envelope.hold_zero = state.hold_zero[i];
  }
}


// ----------------------------------------------------------------------------
// Enable filter.
// ----------------------------------------------------------------------------
void SID::enable_filter(bool enable)
{
  filter.enable_filter(enable);
}


// ----------------------------------------------------------------------------
// Enable external filter.
// ----------------------------------------------------------------------------
void SID::enable_external_filter(bool enable)
{
  extfilt.enable_filter(enable);
}


// ----------------------------------------------------------------------------
// I0() computes the 0th order modified Bessel function of the first kind.
// This function is originally from resample-1.5/filterkit.c by J. O. Smith.
// ----------------------------------------------------------------------------
/*
float SID::I0(float x)
{
  // Max error acceptable in I0.
  const float I0e = 1e-6;

  float sum, u, halfx, temp;
  int n;

  sum = u = n = 1;
  halfx = x/2.0;

  do {
    temp = halfx/n++;
    u *= temp*temp;
    sum += u;
  } while (u >= I0e*sum);

  return sum;
}
*/

// ----------------------------------------------------------------------------
// Setting of SID sampling parameters.
//
// Use a clock freqency of 985248Hz for PAL C64, 1022730Hz for NTSC C64.
// The default end of passband frequency is pass_freq = 0.9*sample_freq/2
// for sample frequencies up to ~ 44.1kHz, and 20kHz for higher sample
// frequencies.
//
// For resampling, the ratio between the clock frequency and the sample
// frequency is limited as follows:
//   125*clock_freq/sample_freq < 16384
// E.g. provided a clock frequency of ~ 1MHz, the sample frequency can not
// be set lower than ~ 8kHz. A lower sample frequency would make the
// resampling code overfill its 16k sample ring buffer.
// 
// The end of passband frequency is also limited:
//   pass_freq <= 0.9*sample_freq/2

// E.g. for a 44.1kHz sampling rate the end of passband frequency is limited
// to slightly below 20kHz. This constraint ensures that the FIR table is
// not overfilled.
// ----------------------------------------------------------------------------
bool SID::set_sampling_parameters(float clock_freq, sampling_method method,
				  float sample_freq, float pass_freq,
				  float filter_scale)
{

  // The default passband limit is 0.9*sample_freq/2 for sample
  // frequencies below ~ 44.1kHz, and 20kHz for higher sample frequencies.
  if (pass_freq < 0) {
    pass_freq = 20000;
    if (2.0*pass_freq/sample_freq >= 0.9) {
      pass_freq = 0.9*sample_freq/2.0;
    }
  }
  // Check whether the FIR table would overfill.
  else if (pass_freq > 0.9*sample_freq/2.0) {
    return false;
  }

  // The filter scaling is only included to avoid clipping, so keep
  // it sane.
  if (filter_scale < 0.9 || filter_scale > 1.0) {
    return false;
  }

  // Set the external filter to the pass freq
  extfilt.set_sampling_parameter (pass_freq);
  clock_frequency = clock_freq;
  sampling = method;

  cycles_per_sample =
    cycle_count(clock_freq/sample_freq*(1 << FIXP_SHIFT) + 0.5);

  sample_offset = 0;
  sample_prev = 0;

  return true;
}


// ----------------------------------------------------------------------------
// Adjustment of SID sampling frequency.
//
// In some applications, e.g. a C64 emulator, it can be desirable to
// synchronize sound with a timer source. This is supported by adjustment of
// the SID sampling frequency.
//
// NB! Adjustment of the sampling frequency may lead to noticeable shifts in
// frequency, and should only be used for interactive applications. Note also
// that any adjustment of the sampling frequency will change the
// characteristics of the resampling filter, since the filter is not rebuilt.
// ----------------------------------------------------------------------------
void SID::adjust_sampling_frequency(float sample_freq)
{
  cycles_per_sample =
    cycle_count(clock_frequency/sample_freq*(1 << FIXP_SHIFT) + 0.5);
}


// ----------------------------------------------------------------------------
// Return array of default spline interpolation points to map FC to
// filter cutoff frequency.
// ----------------------------------------------------------------------------
/*
void SID::fc_default(const fc_point*& points, int& count)
{
  filter.fc_default(points, count);
}
*/

// ----------------------------------------------------------------------------
// Return FC spline plotter object.
// ----------------------------------------------------------------------------
/*
PointPlotter<sound_sample> SID::fc_plotter()
{
  return filter.fc_plotter();
}
*/

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
void SID::clock()
{

  // Age bus value.
  if (--bus_value_ttl <= 0) {
    bus_value = 0;
    bus_value_ttl = 0;
  }

  // Clock amplitude modulators.
	voice[0].envelope.clock();
	voice[1].envelope.clock();
	voice[2].envelope.clock();

  // Clock oscillators.
	voice[0].wave.clock();
	voice[1].wave.clock();
	voice[2].wave.clock();
	
  // Synchronize oscillators.
	voice[0].wave.synchronize();
	voice[1].wave.synchronize();
	voice[2].wave.synchronize();
	
  // Clock filter.
  filter.clock(voice[0].output(), voice[1].output(), voice[2].output(), ext_in);

  // Clock external filter.
  extfilt.clock(filter.output());
}


// ----------------------------------------------------------------------------
// SID clocking - delta_t cycles.
// ----------------------------------------------------------------------------
void SID::clock(cycle_count delta_t)
{
  int i;

  if (delta_t <= 0) {
    return;
  }

  // Age bus value.
  bus_value_ttl -= delta_t;
  if (bus_value_ttl <= 0) {
    bus_value = 0;
    bus_value_ttl = 0;
  }

  // Clock amplitude modulators.

	voice[0].envelope.clock(delta_t);
	voice[1].envelope.clock(delta_t);
	voice[2].envelope.clock(delta_t);
	
  // Clock and synchronize oscillators.
  // Loop until we reach the current cycle.
  cycle_count delta_t_osc = delta_t;
  while (delta_t_osc) {
    cycle_count delta_t_min = delta_t_osc;

    // Find minimum number of cycles to an oscillator accumulator MSB toggle.
    // We have to clock on each MSB on / MSB off for hard sync to operate
    // correctly.
    for (i = 0; i < 3; i++) {
      WaveformGenerator& wave = voice[i].wave;

      // It is only necessary to clock on the MSB of an oscillator that is
      // a sync source and has freq != 0.
      if (!(wave.sync_dest->sync && wave.freq)) {
	continue;
      }

      reg16 freq = wave.freq;
      reg24 accumulator = wave.accumulator;

      // Clock on MSB off if MSB is on, clock on MSB on if MSB is off.
      reg24 delta_accumulator =
				(accumulator & 0x800000 ? 0x1000000 : 0x800000) - accumulator;

      cycle_count delta_t_next = delta_accumulator/freq;
      if (delta_accumulator%freq) {
				++delta_t_next;
      }

      if (delta_t_next < delta_t_min) {
				delta_t_min = delta_t_next;
      }
    }

    // Clock oscillators.
		voice[0].wave.clock(delta_t_min);
		voice[1].wave.clock(delta_t_min);
		voice[2].wave.clock(delta_t_min);
		
		
    // Synchronize oscillators.
		voice[0].wave.synchronize();
		voice[1].wave.synchronize();
		voice[2].wave.synchronize();

    delta_t_osc -= delta_t_min;
  }

  // Clock filter.
  filter.clock(delta_t,
	       voice[0].output(), voice[1].output(), voice[2].output(), ext_in);

  // Clock external filter.
  extfilt.clock(delta_t, filter.output());
}


// ----------------------------------------------------------------------------
// SID clocking with audio sampling.
// Fixpoint arithmetics is used.
//
// The example below shows how to clock the SID a specified amount of cycles
// while producing audio output:
//
// while (delta_t) {
//   bufindex += sid.clock(delta_t, buf + bufindex, buflength - bufindex);
//   write(dsp, buf, bufindex*2);
//   bufindex = 0;
// }
// 
// ----------------------------------------------------------------------------
int SID::clock(cycle_count& delta_t, short* buf, int n)
{
  switch (sampling) {
  default:
  case SAMPLE_FAST:
    return clock_fast(delta_t, buf, n);
  case SAMPLE_INTERPOLATE:
    return clock_interpolate(delta_t, buf, n);
  }
}

// ----------------------------------------------------------------------------
// SID clocking with audio sampling - delta clocking picking nearest sample.
// ----------------------------------------------------------------------------
RESID_INLINE
int SID::clock_fast(cycle_count& delta_t, short* buf, int n)
{
  int s = 0;

  for (;;) {
    cycle_count next_sample_offset = sample_offset + cycles_per_sample + (1 << (FIXP_SHIFT - 1));
    cycle_count delta_t_sample = next_sample_offset >> FIXP_SHIFT;
    if (delta_t_sample > delta_t) {
      break;
    }
    if (s >= n) {
      return s;
    }
    clock(delta_t_sample);
    delta_t -= delta_t_sample;
    sample_offset = (next_sample_offset & FIXP_MASK) - (1 << (FIXP_SHIFT - 1));
    buf[s++] = output();
  }

  clock(delta_t);
  sample_offset -= delta_t << FIXP_SHIFT;
  delta_t = 0;
  return s;
}


// ----------------------------------------------------------------------------
// SID clocking with audio sampling - cycle based with linear sample
// interpolation.
//
// Here the chip is clocked every cycle. This yields higher quality
// sound since the samples are linearly interpolated, and since the
// external filter attenuates frequencies above 16kHz, thus reducing
// sampling noise.
// ----------------------------------------------------------------------------
RESID_INLINE
int SID::clock_interpolate(cycle_count& delta_t, short* buf, int n)
{

  int s = 0;
  int i;

  for (;;) {
    cycle_count next_sample_offset = sample_offset + cycles_per_sample;
    cycle_count delta_t_sample = next_sample_offset >> FIXP_SHIFT;
    if (delta_t_sample > delta_t) {
      break;
    }
    if (s >= n) {
      return s;
    }
    for (i = 0; i < delta_t_sample - 1; i++) {
      clock();
    }
    if (i < delta_t_sample) {
      sample_prev = output();
      clock();
    }

    delta_t -= delta_t_sample;
    sample_offset = next_sample_offset & FIXP_MASK;

    short sample_now = output();
    buf[s++] =
      sample_prev + (sample_offset*(sample_now - sample_prev) >> FIXP_SHIFT);
    sample_prev = sample_now;
  }

  for (i = 0; i < delta_t - 1; i++) {
    clock();
  }
  if (i < delta_t) {
    sample_prev = output();
    clock();
  }

  sample_offset -= delta_t << FIXP_SHIFT;
  delta_t = 0;
  return s;
}


RESID_NAMESPACE_STOP
