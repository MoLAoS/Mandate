// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2011	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "sound_player_xa2.h"

#include <cassert>
#include <cmath>

#include "util.h"
#include "leak_dumper.h"

namespace Shared { namespace Sound { namespace Xa2 {

using namespace Util;

void reportVoiceState(IXAudio2SourceVoice *voice) {
	XAUDIO2_VOICE_STATE state;
	voice->GetState(&state);
	cout << "stream samples played = " << state.SamplesPlayed
		<< ", buffers queued == " << state.BuffersQueued
		<< ", Current buffer = 0x" << intToHex(int(state.pCurrentBufferContext))
		<< endl;
}

// =====================================================
//	class XaVoice
// =====================================================

// ===================== PUBLIC ========================

XaVoice::XaVoice()
		: m_xaVoice(0), m_sound(0), m_size(0) {
}

bool XaVoice::isFree() {
	if (m_xaVoice == 0) {
		return true;
	}
    return false;
}

// ==================== PROTECTED ======================

void XaVoice::createXaVoice(IXAudio2 *xaEngine, SoundCallback *callback) {
	if (m_xaVoice) {
		m_xaVoice->DestroyVoice();
		m_xaVoice = 0;
	}
	// fill in wave format struct
	const SoundInfo &inf = *m_sound->getInfo();
	WAVEFORMATEX format;
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = inf.getChannels();
	format.nSamplesPerSec = inf.getSamplesPerSecond();
	format.nAvgBytesPerSec = (inf.getBitsPerSample() * inf.getSamplesPerSecond() * inf.getChannels()) / 8;
	format.nBlockAlign = (inf.getChannels() * inf.getBitsPerSample()) / 8;
	format.wBitsPerSample = inf.getBitsPerSample();
	format.cbSize = 0;

	uint32 flags = XAUDIO2_VOICE_NOPITCH;
	// create voice
	HRESULT hr = xaEngine->CreateSourceVoice(&m_xaVoice, &format, flags, 2.f, callback, 0, 0);
	CHECK_HR(hr, "Failed to create XAudio2 source voice.");
}

// =====================================================
//	class StaticBuffer
// =====================================================

// ===================== PUBLIC ========================

void StaticXaVoice::init(IXAudio2 *xaEngine, Sound *sound, SoundCallback *callback) {
	m_sound = sound;
	m_size = sound->getInfo()->getSize();

	// create source voice
	createXaVoice(xaEngine, callback);

	// pointer to raw riff chunk
	const BYTE *data = reinterpret_cast<const BYTE*>(static_cast<StaticSound*>(m_sound)->getSamples());

	// buffer desc
	XAUDIO2_BUFFER xaBuffer;
	memset(&xaBuffer, 0, sizeof(xaBuffer));
	xaBuffer.AudioBytes = m_size;
	xaBuffer.pAudioData = data;
	xaBuffer.Flags = XAUDIO2_END_OF_STREAM;

	// submit buffer to voice
	HRESULT hr = m_xaVoice->SubmitSourceBuffer(&xaBuffer);
	CHECK_HR(hr, "Failed to submit XAudio2 buffer to source voice.");
}

void StaticXaVoice::end() {
	if (m_xaVoice) {
		//m_xaVoice->Stop(0);
		m_xaVoice->DestroyVoice();
		m_xaVoice = 0;
	}
	m_sound = 0;
}

void StaticXaVoice::play() {
	m_xaVoice->SetVolume(m_sound->getVolume());
	m_xaVoice->Start(0, XAUDIO2_COMMIT_NOW);
}

// =====================================================
//	class StreamXaVoice
// =====================================================

 // 44KHz, 16 bps, 2 channels => a 1 second buffer will need (44050 * 2 * 2)
const unsigned StreamXaVoice::bufferSize = (44050 * 2 * 2) * 2; // 2 seconds

// ===================== PUBLIC ========================

StreamXaVoice::StreamXaVoice() {
	m_state = StreamState::FREE;
	// create buffers
	for (unsigned i=0; i < numBuffers; ++i) {
		m_buffers[i] = new BYTE[bufferSize];
		m_bufferFlags[i] = false;
	}
}

StreamXaVoice::~StreamXaVoice() {
	for (unsigned i=0; i < numBuffers; ++i) {
		delete [] m_buffers[i];
	}
}

void StreamXaVoice::init(IXAudio2 *xaEngine, Sound *sound, SoundCallback *callback) {
	assert(m_state == StreamState::FREE);
	m_sound = sound;
	m_size = sound->getInfo()->getSize();

	// create source voice
	createXaVoice(xaEngine, callback);

	// fill buffers with data and submit to voice
	MutexLock lock(m_flagsMutex);
	for (unsigned i=0; i < numBuffers; ++i) {
		fillBuffer(i);
		m_bufferFlags[i] = true;
	}
	m_state = StreamState::READY;
}

void StreamXaVoice::end() {
	m_state = StreamState::FREE;
	if (m_xaVoice) {
		m_xaVoice->Stop();
		m_xaVoice->DestroyVoice();
		m_xaVoice = 0;
	}
	// no lock, XAudio2 has told us the stream is done, no need.
	for (unsigned i=0; i < numBuffers; ++i) {
		m_bufferFlags[i] = false;
	}
	m_sound = 0;
}

void StreamXaVoice::play(int64 fadeIn) {
	assert(m_state == StreamState::READY);
	HRESULT hr;
	if (fadeIn == 0) {
		m_state = StreamState::PLAYING;
		hr = m_xaVoice->SetVolume(m_sound->getVolume());
	} else {
		m_fade = fadeIn;
		m_state = StreamState::FADING_IN;
		m_timer.start();
		hr = m_xaVoice->SetVolume(0.f);
	}
	CHECK_HR(hr, "Error setting volume on XAudio voice.");
	hr = m_xaVoice->Start(0, 0);
	CHECK_HR(hr, "Error starting XAudio voice.");
}

void StreamXaVoice::update() {
	switch (m_state) {
		case StreamState::FADING_IN:
			if (m_timer.getMillis() > m_fade) {
				m_xaVoice->SetVolume(m_sound->getVolume());
				m_state = StreamState::PLAYING;
			} else {
				m_xaVoice->SetVolume(m_sound->getVolume() * (float(m_timer.getMillis()) / m_fade));
			}
			break;

		case StreamState::FADING_OUT:
			if (m_timer.getMillis() > m_fade) {
				end();
				return; // state now FREE, don't fall through
			} else {
				m_xaVoice->SetVolume(m_sound->getVolume() * (1.f - float(m_timer.getMillis()) / m_fade));
			}
			break;

		case StreamState::PLAYING:
			m_xaVoice->SetVolume(m_sound->getVolume());
			break;

		default: // FREE or STOPPED
			return;
	}
	// fall through if playing (FADING_IN || PLAYING || FADING_OUT)

	// lock flags and copy
	bool update[numBuffers];
	m_flagsMutex.p();
		for (unsigned i=0; i < numBuffers; ++i) {
			update[i] = !m_bufferFlags[i];
		}
	m_flagsMutex.v();
	// fill empty buffers and resubmit
	for (unsigned i=0; i < numBuffers; ++i) {
		if (update[i]) {
			fillBuffer(i);
		}
	}
}

void StreamXaVoice::stop(int64 fadeOut) {
	if (fadeOut == 0) {
		end();
	} else {
		m_fade = fadeOut;
		m_state = StreamState::FADING_OUT;
		m_timer.start();
	}
}

void StreamXaVoice::fillBuffer(unsigned buffNdx) {
	// fill buffer
	int8 *buffer = reinterpret_cast<int8*>(m_buffers[buffNdx]);
	//cout << "filling buffer " << buffNdx << " @ " << intToHex(int(buffer)) << endl;
	StrSound *s = getStrSound();
	uint32 readSize = s->read(buffer, bufferSize);
	if (readSize < bufferSize) {
		s = (s->getNext() ? s->getNext() : s);
		s->restart();
		readSize += s->read(&buffer[readSize], bufferSize - readSize);
		if (readSize != bufferSize) {
			// should check and reject oggs less than 1 second long.
			assert(false);
		}
		s->setVolume(s->getVolume());
	}

	// fill XAudio2 buffer struct
	XAUDIO2_BUFFER xaBuffer;
	memset(&xaBuffer, 0, sizeof(xaBuffer));
	xaBuffer.AudioBytes = bufferSize;
	xaBuffer.pAudioData = m_buffers[buffNdx];
	xaBuffer.pContext = m_buffers[buffNdx];

	// submit buffer to voice
	HRESULT hr = m_xaVoice->SubmitSourceBuffer(&xaBuffer);
	CHECK_HR(hr, "Failed to submit XAudio2 buffer to source voice.");

	// set in use flag
	m_flagsMutex.p();
		assert(m_bufferFlags[buffNdx] == false);
		m_bufferFlags[buffNdx] = true;
	m_flagsMutex.v();
}

// called from XAudio thread
void StreamXaVoice::onBufferEnd(void *context) {
	assert(context);
	//Chrono timer;
	//timer.start();
	MutexLock lock(m_flagsMutex);
	//timer.stop();
	//if (timer.getMicros() > 10) {
	//	cout << "WARNING: XAudio thread had to wait " << timer.getMicros() << "us.\n";
	//}
	const BYTE *buf = reinterpret_cast<const BYTE*>(context);
	for (unsigned i=0; i < numBuffers; ++i) {
		if (buf == m_buffers[i]) {
			m_bufferFlags[i] = false;
			return;
		}
	}
	assert(false);
}

// =====================================================
//	class SoundCallback
// =====================================================

void SoundCallback::OnStreamEnd() {
	m_player->onFinished(m_stream, m_index);
}

void SoundCallback::OnBufferEnd(void *context) {
	m_player->onBufferEnd(m_index, context);
}

// =====================================================
//	class SoundPlayerXa2
// =====================================================

SoundPlayerXa2::SoundPlayerXa2()
		: m_XAudioEngine(0), m_masterVoice(0) {
}

SoundPlayerXa2::~SoundPlayerXa2() {
	end();
}

void SoundPlayerXa2::init(const SoundPlayerParams *params){
    HRESULT hr;

	m_params= *params;

	// allocate voice slots and create call-back objects
	int n = params->staticBufferCount;
	m_staticVoiceSlots.resize(n);
	m_staticVoiceCallbacks.resize(n);
	for (int i=0; i < n; ++i) {
		m_staticVoiceSlots[i] = new StaticXaVoice();
		m_staticVoiceCallbacks[i] = new SoundCallback(this, false, i);
	}

	n = params->strBufferCount;
	m_streamVoiceSlots.resize(n);
	m_streamVoiceCallbacks.resize(n);
	for (int i=0; i < n; ++i) {
		m_streamVoiceSlots[i] = new StreamXaVoice();
		m_streamVoiceCallbacks[i] = new SoundCallback(this, true, i);
	}

	// create XAudio 2 Engine
	int flags = 0;
#	ifndef NDEBUG
		flags |= XAUDIO2_DEBUG_ENGINE;
#	endif
	CoInitializeEx(0, COINIT_MULTITHREADED);
	hr = XAudio2Create(&m_XAudioEngine, flags);
	CHECK_HR(hr, "Couldn't create XAudio2 Engine.");

	hr = m_XAudioEngine->CreateMasteringVoice(&m_masterVoice);
	CHECK_HR(hr, "Couldn't create XAudio2 Mastering voice.");
}

void SoundPlayerXa2::end() {
	foreach (vector<StaticXaVoice*>, it, m_staticVoiceSlots) {
		(*it)->end();
		delete *it;
	}
	foreach (vector<StreamXaVoice*>, it, m_streamVoiceSlots) {
		(*it)->end();
		delete *it;
	}
	foreach (vector<SoundCallback*>, it, m_staticVoiceCallbacks) {
		delete *it;
	}
	foreach (vector<SoundCallback*>, it, m_streamVoiceCallbacks) {
		delete *it;
	}

	m_masterVoice->DestroyVoice();
	m_masterVoice = 0;

	m_XAudioEngine->Release();
	m_XAudioEngine = 0;

	m_staticVoiceSlots.clear();
	m_streamVoiceSlots.clear();
	m_staticVoiceCallbacks.clear();
	m_streamVoiceCallbacks.clear();
}

void SoundPlayerXa2::play(StaticSound *staticSound) {
	assert(staticSound != 0);
	// play sound if voice slot found
	int ndx;
	if (findStaticVoice(ndx)) {
		m_staticVoiceSlots[ndx]->init(m_XAudioEngine, staticSound, m_staticVoiceCallbacks[ndx]);
		m_staticVoiceSlots[ndx]->play();
	}
}

void SoundPlayerXa2::play(StrSound *streamSound, int64 fadeIn){
	// play sound if voice slot found
	int ndx;
	if (findStreamVoice(ndx)) {
		m_streamVoiceSlots[ndx]->init(m_XAudioEngine, streamSound, m_streamVoiceCallbacks[ndx]);
		m_streamVoiceSlots[ndx]->play(fadeIn);
	}
}

void SoundPlayerXa2::stop(StrSound *streamSound, int64 fadeOut) {
	// find the buffer with this sound and stop it
	for (int i= 0; i < m_params.strBufferCount; ++i) {
		if (m_streamVoiceSlots[i]->getSound() == streamSound) {
			m_streamVoiceSlots[i]->stop(fadeOut);
		}
	}
}

void SoundPlayerXa2::stopAllSounds(){
	for (int i=0; i < m_params.strBufferCount; ++i) {
		if (!m_streamVoiceSlots[i]->isFree()) {
			m_streamVoiceSlots[i]->stop(0);
			m_streamVoiceSlots[i]->end();
		}
	}
	for (int i=0; i < m_params.staticBufferCount; ++i) {
		if (!m_staticVoiceSlots[i]->isFree()) {
			m_staticVoiceSlots[i]->end();
		}
	}
}

void SoundPlayerXa2::updateStreams() {
	// get lock, then copy and clear finshed vector
	m_callbackMutex.p();
		VoiceIds finished = m_finishedVoices;
		m_finishedVoices.clear();
	m_callbackMutex.v();

	// release lock then process copied vectors
	foreach (VoiceIds, it, finished) {
		if (it->first) {
			m_streamVoiceSlots[it->second]->end();
		} else {
			m_staticVoiceSlots[it->second]->end();
		}
	}
	// update streams
	for (int i=0; i < m_params.strBufferCount; ++i) {
		m_streamVoiceSlots[i]->update();
	}
}

// called from XAudio thread
void SoundPlayerXa2::onFinished(bool stream, int index) {
	//Chrono timer;
	//timer.start();
	MutexLock lock(m_callbackMutex);
	//timer.stop();
	//if (timer.getMicros() > 10) {
	//	cout << "WARNING: XAudio thread had to wait " << timer.getMicros() << "us.\n";
	//}
	m_finishedVoices.push_back(std::make_pair(stream, index));
}

// called from XAudio thread
void SoundPlayerXa2::onBufferEnd(int ndx, void *context) {
	if (context) {
		m_streamVoiceSlots[ndx]->onBufferEnd(context);
	}
}

// ===================== PRIVATE =======================

bool SoundPlayerXa2::findStaticVoice(int &out_voiceIndex) {
    // look for free slot
	for (int i=0; i < m_staticVoiceSlots.size(); ++i) {
		if (m_staticVoiceSlots[i]->isFree()) {
			out_voiceIndex = i;
			return true;
		}
	}
	return false;
}

bool SoundPlayerXa2::findStreamVoice(int &out_voiceIndex) {
    // look for free slot
    for (int i=0; i < m_streamVoiceSlots.size(); ++i) {
		if (m_streamVoiceSlots[i]->isFree()) {
			out_voiceIndex = i;
			return true;
        }
    }
	return false;
}

}}} // end namespace Shared::Sound::Xa2
