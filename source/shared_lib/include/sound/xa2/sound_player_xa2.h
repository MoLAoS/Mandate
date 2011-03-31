// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2011	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _SHARED_SOUND_SOUNDPLAYERXA2_H_
#define _SHARED_SOUND_SOUNDPLAYERXA2_H_

#include "sound_player.h"
#include "timer.h"
#include "platform_util.h"
#include "thread.h"
#include <vector>

using std::vector;

namespace Shared { namespace Sound { namespace Xa2 {

class SoundCallback;

// =====================================================
//	class Xa2Voice
// =====================================================

/** wrapps an IXAudio2SourceVoice, analogous to SoundBuffer for other SoundPlayer derivatives */
class XaVoice {
protected:
	IXAudio2SourceVoice *m_xaVoice;
	Sound               *m_sound;
	DWORD                m_size;

public:
	XaVoice();
	virtual ~XaVoice() {};
	virtual void end() = 0;

	IXAudio2SourceVoice* getXaVoice() const      { return m_xaVoice;  }
	Sound *getSound() const						 { return m_sound;    }

	void setXaVoice(IXAudio2SourceVoice *voice)  { m_xaVoice = voice; }
	void setSound(Sound *sound)	                 { m_sound = sound;   }

	bool isFree();

protected:
	void createXaVoice(IXAudio2 *xaEngine, SoundCallback *callback);
};

// =====================================================
//	class StaticSoundBuffer
// =====================================================

class StaticXaVoice : public XaVoice {
public:
	StaticSound* getStaticSound() const	{ return static_cast<StaticSound*>(m_sound); }
	void init(IXAudio2 *xaEngine, Sound *sound, SoundCallback *callback);
	void end();
	void play();
};

// =====================================================
//	class StrSoundBuffer
// =====================================================

class StreamXaVoice : public XaVoice {
private:
	WRAPPED_ENUM( StreamState, FREE, READY, FADING_IN, PLAYING, FADING_OUT );

	static const unsigned bufferSize;
	static const unsigned numBuffers = 3;

private:
	StreamState m_state;    // stream state
	BYTE       *m_buffers[numBuffers];
	Mutex       m_flagsMutex;
	bool        m_bufferFlags[numBuffers];
	Chrono      m_timer;	// fade in/out timer
	int64       m_fade;		// fade in/out time (ms)

public:
	StreamXaVoice();
	~StreamXaVoice();

	StrSound   *getStrSound()    const { return static_cast<StrSound*>(m_sound); }
	StreamState getStreamState() const { return m_state; }

	void init(IXAudio2 *xaEngine, Sound *sound, SoundCallback *callback);
	void end();    /**< destroy buffers and xa voice */

	void play(int64 fadeIn);  /**< start playing @param fadeIn time in ms to fade the sound in */
	void update();            /**< refresh any free buffers and resubmit */
	void stop(int64 fadeOut); /**< stop playing @param fadeOut time in ms to fade the sound out */

	void onBufferEnd(void *context);

private:
	void fillBuffer(unsigned buffNdx);
};

class SoundPlayerXa2;

class SoundCallback : public IXAudio2VoiceCallback {
private:
	SoundPlayerXa2   *m_player;
	bool              m_stream;
	int               m_index;

public:
	SoundCallback(SoundPlayerXa2 *player, bool stream, int ndx)
		: m_player(player), m_stream(stream), m_index(ndx) { }

	~SoundCallback() {}

	// OnStreamEnd(): destroy voice and mark 'slot' free
	virtual void STDMETHODCALLTYPE OnStreamEnd() override;

	// OnBufferEnd(): if stream sound, reifill buffer and resubmit to voice
	virtual void STDMETHODCALLTYPE OnBufferEnd(void * pBufferContext) override;

	// no-implementation
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override { }
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 SamplesRequired) override { }
	virtual void STDMETHODCALLTYPE OnBufferStart(void * pBufferContext) override { }
	virtual void STDMETHODCALLTYPE OnLoopEnd(void * pBufferContext) override { }
	virtual void STDMETHODCALLTYPE OnVoiceError(void * pBufferContext, HRESULT Error) override { }
};

// =====================================================
//	class SoundPlayerXa2
//
///	SoundPlayer implementation using XAudio 2
// =====================================================

class SoundPlayerXa2: public SoundPlayer {
private:
	typedef pair<bool,int>   VoiceId;
	typedef vector<VoiceId>  VoiceIds;

private:
	IXAudio2                  *m_XAudioEngine;
	IXAudio2MasteringVoice    *m_masterVoice;
	vector<StaticXaVoice*>     m_staticVoiceSlots;
	vector<SoundCallback*>     m_staticVoiceCallbacks;
	vector<StreamXaVoice*>     m_streamVoiceSlots;
	vector<SoundCallback*>     m_streamVoiceCallbacks;
	SoundPlayerParams          m_params;

	Mutex                      m_callbackMutex;
	VoiceIds                   m_finishedVoices;

public:
	SoundPlayerXa2();
	virtual ~SoundPlayerXa2();
	virtual void init(const SoundPlayerParams *params);
	virtual void end();
	virtual void play(StaticSound *staticSound);
	virtual void play(StrSound *strSound, int64 fadeIn = 0);
	virtual void stop(StrSound *strSound, int64 fadeOut = 0);
	virtual void stopAllSounds();
	virtual void updateStreams(); // updates str buffers if needed

	void onFinished(bool stream, int index);
	void onBufferEnd(int index, void *context);

private:
	bool findStaticVoice(int &out_voiceIndex);
	bool findStreamVoice(int &out_voiceIndex);
};

}}} // end namespace Shared::Sound::Xa2

#endif
