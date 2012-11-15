#ifndef __CAPTURE_H__
#define __CAPTURE_H__

typedef struct krad_decklink_capture_St krad_decklink_capture_t;

#ifdef __cplusplus
extern "C" {
#include "krad_system.h"
}
#endif

#ifdef __cplusplus

#include "DeckLinkAPI.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

class DeckLinkCaptureDelegate : public IDeckLinkInputCallback
{
public:
	DeckLinkCaptureDelegate();
	~DeckLinkCaptureDelegate();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE  Release(void);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*);

	krad_decklink_capture_t *krad_decklink_capture;

private:
	ULONG				m_refCount;
	pthread_mutex_t		m_mutex;
};

#ifndef KRAD_DECKLINK_H
struct krad_decklink_capture_St {

	IDeckLink *deckLink;
	IDeckLinkInput *deckLinkInput;
	IDeckLinkConfiguration *deckLinkConfiguration;
	IDeckLinkIterator *deckLinkIterator;
	IDeckLinkDisplayModeIterator *displayModeIterator;
	IDeckLinkDisplayMode *displayMode;
	int inputFlags;
	DeckLinkCaptureDelegate *delegate;
	HRESULT result;
	
	int verbose;

	int skip_frame;
	int skip_frames;
	
	BMDDisplayMode display_mode;
	BMDPixelFormat pixel_format;
	BMDAudioSampleRate audio_sample_rate;
	BMDAudioConnection audio_input;
	BMDVideoConnection video_input;	
	int audio_channels;
	int audio_bit_depth;
	
	uint64_t video_frames;
	uint64_t audio_frames;
	
	int (*video_frame_callback)(void *, void *, int);
	int (*audio_frames_callback)(void *, void *, int);
	void *callback_pointer;


	int width;
	int height;
	int fps_numerator;
	int fps_denominator;
	
	int device;


};
#endif


extern "C" {
#endif

void krad_decklink_capture_set_video_input(krad_decklink_capture_t *krad_decklink_capture, char *video_input);
void krad_decklink_capture_set_audio_input(krad_decklink_capture_t *krad_decklink_capture, char *audio_input);

void krad_decklink_capture_set_video_mode(krad_decklink_capture_t *krad_decklink_capture, int width, int height,
										  int fps_numerator, int fps_denominator);

krad_decklink_capture_t *krad_decklink_capture_create (int device);
void krad_decklink_capture_start(krad_decklink_capture_t *krad_decklink_capture);
void krad_decklink_capture_stop(krad_decklink_capture_t *krad_decklink_capture);
void krad_decklink_capture_info ();

void krad_decklink_capture_set_verbose(krad_decklink_capture_t *krad_decklink_capture, int verbose);
void krad_decklink_capture_set_video_callback(krad_decklink_capture_t *krad_decklink_capture, int video_frame_callback(void *, void *, int));
void krad_decklink_capture_set_audio_callback(krad_decklink_capture_t *krad_decklink_capture, int audio_frames_callback(void *, void *, int));
void krad_decklink_capture_set_callback_pointer(krad_decklink_capture_t *krad_decklink_capture, void *callback_pointer);

int krad_decklink_cpp_detect_devices ();
void krad_decklink_cpp_get_device_name (int device_num, char *device_name);

#ifdef __cplusplus
}

#endif

#endif
