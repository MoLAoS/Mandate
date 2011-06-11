// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "sound_file_loader.h"

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "sound.h"
#include "leak_dumper.h"
#include "profiler.h"
#include "FSFactory.hpp"

using std::ios_base;
using namespace Shared::Platform;

namespace Shared{ namespace Sound{

// =====================================================
//	class WavSoundFileLoader
// =====================================================

void WavSoundFileLoader::open(const string &path, SoundInfo *soundInfo) {
    char chunkId[]={'-', '-', '-', '-', '\0'};
    uint32 size32= 0;
    uint16 size16= 0;
    int count;

	f = FSFactory::getInstance()->getFileOps();
	try {
		f->openRead(path.c_str());
	} catch (const runtime_error &e) {
		throw runtime_error("Error opening wav file: "+ string(path) + "\n" + e.what());
	}

	//RIFF chunk - Id
	f->read(chunkId, 4, 1);

	if (strcmp(chunkId, "RIFF") != 0) {
		throw runtime_error("Not a valid wav file (first four bytes are not RIFF):" + path);
	}

	//RIFF chunk - Size
	f->read((char*)&size32, 4, 1);

	//RIFF chunk - Data (WAVE string)
	f->read(chunkId, 4, 1);

	if (strcmp(chunkId, "WAVE") != 0) {
		throw runtime_error("Not a valid wav file (wave data don't start by WAVE): " + path);
	}

	// === HEADER ===

	//first sub-chunk (header) - Id
	f->read(chunkId, 4, 1);

	if (strcmp(chunkId, "fmt ") != 0) {
		throw runtime_error("Not a valid wav file (first sub-chunk Id is not fmt): "+ path);
	}

	//first sub-chunk (header) - Size
	f->read((char*)&size32, 4, 1);

	//first sub-chunk (header) - Data (encoding type) - Ignore
	f->read((char*)&size16, 2, 1);

	//first sub-chunk (header) - Data (nChannels)
	f->read((char*)&size16, 2, 1);
	soundInfo->setChannels(size16);

	//first sub-chunk (header) - Data (nsamplesPerSecond)
	f->read((char*)&size32, 4, 1);
	soundInfo->setsamplesPerSecond(size32);

	//first sub-chunk (header) - Data (nAvgBytesPerSec)  - Ignore
	f->read((char*)&size32, 4, 1);

	//first sub-chunk (header) - Data (blockAlign) - Ignore
	f->read((char*)&size16, 2, 1);

	//first sub-chunk (header) - Data (nsamplesPerSecond)
	f->read((char*)&size16, 2, 1);
	soundInfo->setBitsPerSample(size16);

	if (soundInfo->getBitsPerSample() != 8 && soundInfo->getBitsPerSample() != 16) {
		throw runtime_error("Bits per sample must be 8 or 16: " + path);
	}
	bytesPerSecond = soundInfo->getBitsPerSample()*8*soundInfo->getSamplesPerSecond()*soundInfo->getChannels();

	count=0;
	do {
		count++;

		// === DATA ===
		//second sub-chunk (samples) - Id
		f->read(chunkId, 4, 1);
		if(strncmp(chunkId, "data", 4)!=0){
			continue;
		}

		//second sub-chunk (samples) - Size
		f->read((char*) &size32, 4, 1);
		dataSize= size32;
		soundInfo->setSize(dataSize);
	} while(strncmp(chunkId, "data", 4)!=0 && count<maxDataRetryCount);

	dataOffset = f->tell();
}

uint32 WavSoundFileLoader::read(int8 *samples, uint32 size){
	return f->read(samples, size, 1) * size;
}

void WavSoundFileLoader::close(){
	delete f;
}

void WavSoundFileLoader::restart() {
	f->seek(dataOffset, SEEK_SET);
}

// =======================================
//        Ogg Sound File Loader
// =======================================

void OggSoundFileLoader::open(const string &path, SoundInfo *soundInfo){
	FileOps *fops = FSFactory::getInstance()->getFileOps();
	fops->openRead(path.c_str());
	ov_callbacks callbacks = {FSFactory::cb_read, FSFactory::cb_seek, FSFactory::cb_close, FSFactory::cb_tell};
	vf = new OggVorbis_File;
	if(ov_open_callbacks(fops, vf, NULL, 0, callbacks)){  // fops is deleted by cb_close
		delete fops;
		throw runtime_error("ov_open_callback failed on ogg file: " + path);
	}
	vorbis_info *vi= ov_info(vf, -1);

	soundInfo->setChannels(vi->channels);
	soundInfo->setsamplesPerSecond(vi->rate);
	soundInfo->setBitsPerSample(16);
	soundInfo->setSize(static_cast<uint32>(ov_pcm_total(vf, -1))*2);
}

uint32 OggSoundFileLoader::read(int8 *samples, uint32 size){
	//_PROFILE_FUNCTION();
	int section;
	int totalBytesRead= 0;

	while(size>0){
		int bytesRead= ov_read(vf, reinterpret_cast<char*> (samples), size,
							   0, 2, 1, &section);
		if(bytesRead==0){
			break;
		}
		size-= bytesRead;
		samples+= bytesRead;
		totalBytesRead+= bytesRead;
	}
	return totalBytesRead;
}

void OggSoundFileLoader::close(){
	if(vf){
		ov_clear(vf);
		delete vf;
		vf = NULL;
	}
}

void OggSoundFileLoader::restart(){
	ov_raw_seek(vf, 0);
}

// =====================================================
//	class SoundFileLoaderFactory
// =====================================================

SoundFileLoaderFactory::SoundFileLoaderFactory(){
	registerClass<WavSoundFileLoader>("wav");
	registerClass<OggSoundFileLoader>("ogg");
}

SoundFileLoaderFactory *SoundFileLoaderFactory::getInstance(){
	static SoundFileLoaderFactory soundFileLoaderFactory;
	return &soundFileLoaderFactory;
}

}}//end namespace
