#ifndef __CAPTURE_H__
#define __CAPTURE_H__

typedef struct krad_decklink_capture_St krad_decklink_capture_t;

#ifdef __cplusplus

#include "DeckLinkAPI.h"


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
	unsigned long frameCount;
};

#ifndef KRAD_DECKLINK_H
struct krad_decklink_capture_St {

	IDeckLink *deckLink;
	IDeckLinkInput *deckLinkInput;
	IDeckLinkIterator *deckLinkIterator;
	IDeckLinkDisplayModeIterator *displayModeIterator;
	IDeckLinkDisplayMode *displayMode;
	BMDDisplayMode selectedDisplayMode;
	BMDPixelFormat pixelFormat;
	int inputFlags;
	DeckLinkCaptureDelegate *delegate;

	HRESULT result;

};
#endif


extern "C" {
#endif


krad_decklink_capture_t *krad_decklink_capture_start();
void krad_decklink_capture_stop(krad_decklink_capture_t *krad_decklink_capture);
void krad_decklink_info ();

#ifdef __cplusplus
}

#endif

#endif
