/*
 * Copyright © 2012 Mozilla Foundation
 *
 * This program is made available under an ISC-style license.  See the
 * accompanying file LICENSE for details.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>
#include <stddef.h>
#include <inttypes.h>


#define DEFAULT_HOST "kradradio.com"
#define DEFAULT_PORT 9040
#define DEFAULT_PASSWORD "secretkode"
#define HELP -1337
#define DEFAULT_SAMPLERATE 44100
#define DEFAULT_BITRATE 666
#define DEFAULT_CHANNELS 2
#define DEFAULT_STREAMNAME "A Krad EBML Stream"
#define DEFAULT_URL "http://kradradio.com"
#define DEFAULT_DESCRIPTION "Krad EBML Streaming Test"
#define DEFAULT_PUBLIC 0
#define TIMESLICE 4
#define KRADEBML_VERSION "0.31"

#define LITERALU64(n) n##LLU
#define off_t long long

#define NESTEGG_TRACK_VIDEO 0 /**< Track is of type video. */
#define NESTEGG_TRACK_AUDIO 1 /**< Track is of type audio. */
#define NESTEGG_TRACK_SUBTITLE 2 /**< Track is of type subtitle. */

#define NESTEGG_CODEC_VP8    0 /**< Track uses Google On2 VP8 codec. */
#define NESTEGG_CODEC_VORBIS 1 /**< Track uses Xiph Vorbis codec. */
#define NESTEGG_CODEC_OPUS 2 /**< Track uses Xiph Opus codec. */
#define NESTEGG_CODEC_FLAC 3 /**< Track uses Xiph FLAC codec. */
#define NESTEGG_CODEC_DIRAC 4 /**< Track uses Dirac codec. */
#define NESTEGG_CODEC_THEORA 5 /**< Track uses Xiph Theora codec. */

#define NESTEGG_VIDEO_MONO              0 /**< Track is mono video. */
#define NESTEGG_VIDEO_STEREO_LEFT_RIGHT 1 /**< Track is side-by-side stereo video.  Left first. */
#define NESTEGG_VIDEO_STEREO_BOTTOM_TOP 2 /**< Track is top-bottom stereo video.  Right first. */
#define NESTEGG_VIDEO_STEREO_TOP_BOTTOM 3 /**< Track is top-bottom stereo video.  Left first. */
#define NESTEGG_VIDEO_STEREO_RIGHT_LEFT 11 /**< Track is side-by-side stereo video.  Right first. */

#define NESTEGG_SEEK_SET 0 /**< Seek offset relative to beginning of stream. */
#define NESTEGG_SEEK_CUR 1 /**< Seek offset relative to current position in stream. */
#define NESTEGG_SEEK_END 2 /**< Seek offset relative to end of stream. */

#define NESTEGG_LOG_DEBUG    1     /**< Debug level log message. */
#define NESTEGG_LOG_INFO     10    /**< Informational level log message. */
#define NESTEGG_LOG_WARNING  100   /**< Warning level log message. */
#define NESTEGG_LOG_ERROR    1000  /**< Error level log message. */
#define NESTEGG_LOG_CRITICAL 10000 /**< Critical level log message. */

#define KRAD_EBML_CLUSTER_BYTE1 0x1F
#define KRAD_EBML_CLUSTER_BYTE2 0x43
#define KRAD_EBML_CLUSTER_BYTE3 0xB6
#define KRAD_EBML_CLUSTER_BYTE4 0x75


typedef enum {
	ICECAST2,
	KXPDR,
} server_type_t;

typedef enum {
	KRAD_VORBIS,
	KRAD_OPUS,
	KRAD_FLAC,
	KRAD_VP8,
	KRAD_DIRAC,
	KRAD_THEORA,
} krad_codec_type_t;

enum mkv
{
    EBML = 0x1A45DFA3,
    EBMLVersion = 0x4286,
    EBMLReadVersion = 0x42F7,
    EBMLMaxIDLength = 0x42F2,
    EBMLMaxSizeLength = 0x42F3,
    DocType = 0x4282,
    DocTypeVersion = 0x4287,
    DocTypeReadVersion = 0x4285,
//  CRC_32 = 0xBF,
    Void = 0xEC,
    SignatureSlot = 0x1B538667,
    SignatureAlgo = 0x7E8A,
    SignatureHash = 0x7E9A,
    SignaturePublicKey = 0x7EA5,
    Signature = 0x7EB5,
    SignatureElements = 0x7E5B,
    SignatureElementList = 0x7E7B,
    SignedElement = 0x6532,
    //segment
    Segment = 0x18538067,
    //Meta Seek Information
    SeekHead = 0x114D9B74,
    Seek = 0x4DBB,
    SeekID = 0x53AB,
    SeekPosition = 0x53AC,
    //Segment Information
    Info = 0x1549A966,
//  SegmentUID = 0x73A4,
//  SegmentFilename = 0x7384,
//  PrevUID = 0x3CB923,
//  PrevFilename = 0x3C83AB,
//  NextUID = 0x3EB923,
//  NextFilename = 0x3E83BB,
//  SegmentFamily = 0x4444,
//  ChapterTranslate = 0x6924,
//  ChapterTranslateEditionUID = 0x69FC,
//  ChapterTranslateCodec = 0x69BF,
//  ChapterTranslateID = 0x69A5,
    TimecodeScale = 0x2AD7B1,
    Segment_Duration = 0x4489,
    DateUTC = 0x4461,
//  Title = 0x7BA9,
    MuxingApp = 0x4D80,
    WritingApp = 0x5741,
    //Cluster
    Cluster = 0x1F43B675,
    Timecode = 0xE7,
//  SilentTracks = 0x5854,
//  SilentTrackNumber = 0x58D7,
	Position = 0xA7,
    PrevSize = 0xAB,
    BlockGroup = 0xA0,
    Block = 0xA1,
//  BlockVirtual = 0xA2,
//  BlockAdditions = 0x75A1,
//  BlockMore = 0xA6,
//  BlockAddID = 0xEE,
//  BlockAdditional = 0xA5,
    BlockDuration = 0x9B,
//  ReferencePriority = 0xFA,
    ReferenceBlock = 0xFB,
//  ReferenceVirtual = 0xFD,
//  CodecState = 0xA4,
//  Slices = 0x8E,
//  TimeSlice = 0xE8,
    LaceNumber = 0xCC,
//  FrameNumber = 0xCD,
//  BlockAdditionID = 0xCB,
//  MkvDelay = 0xCE,
//  Cluster_Duration = 0xCF,
    SimpleBlock = 0xA3,
//  EncryptedBlock = 0xAF,
    //Track
    Tracks = 0x1654AE6B,
    TrackEntry = 0xAE,
    TrackNumber = 0xD7,
    TrackUID = 0x73C5,
    TrackType = 0x83,
    FlagEnabled = 0xB9,
    FlagDefault = 0x88,
    FlagForced = 0x55AA,
    FlagLacing = 0x9C,
//  MinCache = 0x6DE7,
//  MaxCache = 0x6DF8,
    DefaultDuration = 0x23E383,
//  TrackTimecodeScale = 0x23314F,
//  TrackOffset = 0x537F,
//  MaxBlockAdditionID = 0x55EE,
    Name = 0x536E,
    Language = 0x22B59C,
    CodecID = 0x86,
    CodecPrivate = 0x63A2,
    CodecName = 0x258688,
//  AttachmentLink = 0x7446,
//  CodecSettings = 0x3A9697,
//  CodecInfoURL = 0x3B4040,
//  CodecDownloadURL = 0x26B240,
//  CodecDecodeAll = 0xAA,
//  TrackOverlay = 0x6FAB,
//  TrackTranslate = 0x6624,
//  TrackTranslateEditionUID = 0x66FC,
//  TrackTranslateCodec = 0x66BF,
//  TrackTranslateTrackID = 0x66A5,
    //video
    Video = 0xE0,
    FlagInterlaced = 0x9A,
    StereoMode = 0x53B8,
    PixelWidth = 0xB0,
    PixelHeight = 0xBA,
    PixelCropBottom = 0x54AA,
    PixelCropTop = 0x54BB,
    PixelCropLeft = 0x54CC,
    PixelCropRight = 0x54DD,
    DisplayWidth = 0x54B0,
    DisplayHeight = 0x54BA,
    DisplayUnit = 0x54B2,
    AspectRatioType = 0x54B3,
//  ColourSpace = 0x2EB524,
//  GammaValue = 0x2FB523,
    FrameRate = 0x2383E3,
    //end video
    //audio
    Audio = 0xE1,
    SamplingFrequency = 0xB5,
    OutputSamplingFrequency = 0x78B5,
    Channels = 0x9F,
//  ChannelPositions = 0x7D7B,
    BitDepth = 0x6264,
    //end audio
    //content encoding
//  ContentEncodings = 0x6d80,
//  ContentEncoding = 0x6240,
//  ContentEncodingOrder = 0x5031,
//  ContentEncodingScope = 0x5032,
//  ContentEncodingType = 0x5033,
//  ContentCompression = 0x5034,
//  ContentCompAlgo = 0x4254,
//  ContentCompSettings = 0x4255,
//  ContentEncryption = 0x5035,
//  ContentEncAlgo = 0x47e1,
//  ContentEncKeyID = 0x47e2,
//  ContentSignature = 0x47e3,
//  ContentSigKeyID = 0x47e4,
//  ContentSigAlgo = 0x47e5,
//  ContentSigHashAlgo = 0x47e6,
    //end content encoding
    //Cueing Data
    Cues = 0x1C53BB6B,
    CuePoint = 0xBB,
    CueTime = 0xB3,
    CueTrackPositions = 0xB7,
    CueTrack = 0xF7,
    CueClusterPosition = 0xF1,
    CueBlockNumber = 0x5378,
//  CueCodecState = 0xEA,
//  CueReference = 0xDB,
//  CueRefTime = 0x96,
//  CueRefCluster = 0x97,
//  CueRefNumber = 0x535F,
//  CueRefCodecState = 0xEB,
    //Attachment
//  Attachments = 0x1941A469,
//  AttachedFile = 0x61A7,
//  FileDescription = 0x467E,
//  FileName = 0x466E,
//  FileMimeType = 0x4660,
//  FileData = 0x465C,
//  FileUID = 0x46AE,
//  FileReferral = 0x4675,
    //Chapters
//  Chapters = 0x1043A770,
//  EditionEntry = 0x45B9,
//  EditionUID = 0x45BC,
//  EditionFlagHidden = 0x45BD,
//  EditionFlagDefault = 0x45DB,
//  EditionFlagOrdered = 0x45DD,
//  ChapterAtom = 0xB6,
//  ChapterUID = 0x73C4,
//  ChapterTimeStart = 0x91,
//  ChapterTimeEnd = 0x92,
//  ChapterFlagHidden = 0x98,
//  ChapterFlagEnabled = 0x4598,
//  ChapterSegmentUID = 0x6E67,
//  ChapterSegmentEditionUID = 0x6EBC,
//  ChapterPhysicalEquiv = 0x63C3,
//  ChapterTrack = 0x8F,
//  ChapterTrackNumber = 0x89,
//  ChapterDisplay = 0x80,
//  ChapString = 0x85,
//  ChapLanguage = 0x437C,
//  ChapCountry = 0x437E,
//  ChapProcess = 0x6944,
//  ChapProcessCodecID = 0x6955,
//  ChapProcessPrivate = 0x450D,
//  ChapProcessCommand = 0x6911,
//  ChapProcessTime = 0x6922,
//  ChapProcessData = 0x6933,
    //Tagging
//  Tags = 0x1254C367,
//  Tag = 0x7373,
//  Targets = 0x63C0,
//  TargetTypeValue = 0x68CA,
//  TargetType = 0x63CA,
//  Tagging_TrackUID = 0x63C5,
//  Tagging_EditionUID = 0x63C9,
//  Tagging_ChapterUID = 0x63C4,
//  AttachmentUID = 0x63C6,
//  SimpleTag = 0x67C8,
//  TagName = 0x45A3,
//  TagLanguage = 0x447A,
//  TagDefault = 0x4484,
//  TagString = 0x4487,
//  TagBinary = 0x4485,
};

typedef struct EbmlGlobal EbmlGlobal;

typedef struct kradebml_St kradebml_t;

typedef enum stereo_format
{
    STEREO_FORMAT_MONO       = 0,
    STEREO_FORMAT_LEFT_RIGHT = 1,
    STEREO_FORMAT_BOTTOM_TOP = 2,
    STEREO_FORMAT_TOP_BOTTOM = 3,
    STEREO_FORMAT_RIGHT_LEFT = 11
} stereo_format_t;


typedef off_t EbmlLoc;

struct cue_entry
{
    unsigned int time;
    uint64_t     loc;
};

struct EbmlGlobal
{
    int debug;

       EbmlLoc tracks;
	//kradsource_t *kradsource;

	char buffer[8192 * 1024 * 16];
	uint64_t buffer_pos;

    FILE    *stream;
    int64_t last_pts_ms;
    //vpx_rational_t  framerate;

    /* These pointers are to the start of an element */
    off_t    position_reference;
    off_t    seek_info_pos;
    off_t    segment_info_pos;
    off_t    track_pos;
    off_t    cue_pos;
    off_t    cluster_pos;

    /* This pointer is to a specific element to be serialized */
    off_t    track_id_pos;

    /* These pointers are to the size field of the element */
    EbmlLoc  startSegment;
    EbmlLoc  startCluster;

    uint32_t cluster_timecode;
    int      cluster_open;

    struct cue_entry *cue_list;
    unsigned int      cues;
    
    int video_frame_rate;
    int total_video_frames;
    int      tracks_open;
};


unsigned int murmur ( const void * key, int len, unsigned int seed );
void write_webm_file_footer(EbmlGlobal *glob, long hash);
//void write_webm_block(EbmlGlobal *glob, const vpx_codec_enc_cfg_t *cfg, const vpx_codec_cx_pkt_t  *pkt);
//void write_webm_file_header(EbmlGlobal *glob, const vpx_codec_enc_cfg_t *cfg, const struct vpx_rational *fps, stereo_format_t stereo_fmt);

void write_webm_header(EbmlGlobal *glob);

void write_webm_seek_info(EbmlGlobal *ebml);
void write_webm_seek_element(EbmlGlobal *ebml, unsigned long id, off_t pos);
void Ebml_EndSubElement(EbmlGlobal *glob, EbmlLoc *ebmlLoc);
void Ebml_StartSubElement(EbmlGlobal *glob, EbmlLoc *ebmlLoc, unsigned long class_id);
void Ebml_SerializeUnsigned32(EbmlGlobal *glob, unsigned long class_id, uint64_t ui);
void Ebml_Serialize(EbmlGlobal *glob, const void *buffer_in, int buffer_size, unsigned long len);
void Ebml_Write(EbmlGlobal *glob, const void *buffer_in, unsigned long len);
void Ebml_WriteLen(EbmlGlobal *glob, long long val);
void Ebml_WriteString(EbmlGlobal *glob, const char *str);
void Ebml_WriteID(EbmlGlobal *glob, unsigned long class_id);
void Ebml_SerializeUnsigned64(EbmlGlobal *glob, unsigned long class_id, uint64_t ui);
void Ebml_SerializeUnsigned(EbmlGlobal *glob, unsigned long class_id, unsigned long ui);
void Ebml_SerializeBinary(EbmlGlobal *glob, unsigned long class_id, unsigned long ui);
void Ebml_SerializeFloat(EbmlGlobal *glob, unsigned long class_id, double d);
void Ebml_SerializeString(EbmlGlobal *glob, unsigned long class_id, const char *s);
void Ebml_SerializeData(EbmlGlobal *glob, unsigned long class_id, unsigned char *data, unsigned long data_length);
void Ebml_SerializeData(EbmlGlobal *glob, unsigned long class_id, unsigned char *data, unsigned long data_length);
//void write_webm_audio_block(EbmlGlobal *glob, const vpx_codec_enc_cfg_t *cfg, unsigned char *buffer, int bufferlen, int timecode_pos);

typedef struct nestegg nestegg;               /**< Opaque handle referencing the stream state. */
typedef struct nestegg_packet nestegg_packet; /**< Opaque handle referencing a packet of data. */

/** User supplied IO context. */
typedef struct {

  /** User supplied write callback.
      @param buffer   Buffer to write data from.
      @param length   Length of supplied buffer in bytes.
      @param userdata The #userdata supplied by the user.
      @retval  1 write succeeded. (this is wrong)
      @retval  0 End of stream.
      @retval -1 Error. */
  int (* write)(void * buffer, size_t length, void * userdata);


  /** User supplied read callback.
      @param buffer   Buffer to read data into.
      @param length   Length of supplied buffer in bytes.
      @param userdata The #userdata supplied by the user.
      @retval  1 Read succeeded.
      @retval  0 End of stream.
      @retval -1 Error. */
  int (* read)(void * buffer, size_t length, void * userdata);

  /** User supplied seek callback.
      @param offset   Offset within the stream to seek to.
      @param whence   Seek direction.  One of #NESTEGG_SEEK_SET,
                      #NESTEGG_SEEK_CUR, or #NESTEGG_SEEK_END.
      @param userdata The #userdata supplied by the user.
      @retval  0 Seek succeeded.
      @retval -1 Error. */
  int (* seek)(int64_t offset, int whence, void * userdata);

  /** User supplied tell callback.
      @param userdata The #userdata supplied by the user.
      @returns Current position within the stream.
      @retval -1 Error. */
  int64_t (* tell)(void * userdata);

  int64_t (* open)(void * userdata);
  int64_t (* close)(void * userdata);

  /** User supplied pointer to be passed to the IO callbacks. */
  void * userdata;
} nestegg_io;

/** User supplied IO context. */
typedef struct {
  /** User supplied read callback.
      @param buffer   Buffer to read data into.
      @param length   Length of supplied buffer in bytes.
      @param userdata The #userdata supplied by the user.
      @retval  1 Read succeeded.
      @retval  0 End of stream.
      @retval -1 Error. */

  void (* segment_len_at)(int64_t offset, void * userdata);
  void (* cluster_at)(int64_t offset, void * userdata);


  /** User supplied pointer to be passed to the IO callbacks. */
  void * userdata;
} nestegg_info;

/** Parameters specific to a video track. */
typedef struct {
  unsigned int stereo_mode;    /**< Video mode.  One of #NESTEGG_VIDEO_MONO,
                                    #NESTEGG_VIDEO_STEREO_LEFT_RIGHT,
                                    #NESTEGG_VIDEO_STEREO_BOTTOM_TOP, or
                                    #NESTEGG_VIDEO_STEREO_TOP_BOTTOM. */
  unsigned int width;          /**< Width of the video frame in pixels. */
  unsigned int height;         /**< Height of the video frame in pixels. */
  unsigned int display_width;  /**< Display width of the video frame in pixels. */
  unsigned int display_height; /**< Display height of the video frame in pixels. */
  unsigned int crop_bottom;    /**< Pixels to crop from the bottom of the frame. */
  unsigned int crop_top;       /**< Pixels to crop from the top of the frame. */
  unsigned int crop_left;      /**< Pixels to crop from the left of the frame. */
  unsigned int crop_right;     /**< Pixels to crop from the right of the frame. */
} nestegg_video_params;

/** Parameters specific to an audio track. */
typedef struct {
  double rate;           /**< Sampling rate in Hz. */
  unsigned int channels; /**< Number of audio channels. */
  unsigned int depth;    /**< Bits per sample. */
} nestegg_audio_params;

/** Logging callback function pointer. */
typedef void (* nestegg_log)(nestegg * context, unsigned int severity, char const * format, ...);

/** Initialize a nestegg context.  During initialization the parser will
    read forward in the stream processing all elements until the first
    block of media is reached.  All track metadata has been processed at this point.
    @param context  Storage for the new nestegg context.  @see nestegg_destroy
    @param io       User supplied IO context.
    @param callback Optional logging callback function pointer.  May be NULL.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_init(nestegg ** context, nestegg_io io, nestegg_log callback, nestegg_info *info);

/** Destroy a nestegg context and free associated memory.
    @param context #nestegg context to be freed.  @see nestegg_init */
void nestegg_destroy(nestegg * context);

/** Query the duration of the media stream in nanoseconds.
    @param context  Stream context initialized by #nestegg_init.
    @param duration Storage for the queried duration.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_duration(nestegg * context, uint64_t * duration);

/** Query the tstamp scale of the media stream in nanoseconds.
    Timecodes presented by nestegg have been scaled by this value
    before presentation to the caller.
    @param context Stream context initialized by #nestegg_init.
    @param scale   Storage for the queried scale factor.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_tstamp_scale(nestegg * context, uint64_t * scale);

/** Query the number of tracks in the media stream.
    @param context Stream context initialized by #nestegg_init.
    @param tracks  Storage for the queried track count.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_track_count(nestegg * context, unsigned int * tracks);

/** Seek @a track to @a tstamp.  Stream seek will terminate at the earliest
    key point in the stream at or before @a tstamp.  Other tracks in the
    stream will output packets with unspecified but nearby timestamps.
    @param context Stream context initialized by #nestegg_init.
    @param track   Zero based track number.
    @param tstamp  Absolute timestamp in nanoseconds.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_track_seek(nestegg * context, unsigned int track, uint64_t tstamp);

/** Query the type specified by @a track.
    @param context Stream context initialized by #nestegg_init.
    @param track   Zero based track number.
    @retval #NESTEGG_TRACK_VIDEO Track type is video.
    @retval #NESTEGG_TRACK_AUDIO Track type is audio.
    @retval -1 Error. */
int nestegg_track_type(nestegg * context, unsigned int track);

/** Query the codec ID specified by @a track.
    @param context Stream context initialized by #nestegg_init.
    @param track   Zero based track number.
    @retval #NESTEGG_CODEC_VP8    Track codec is VP8.
    @retval #NESTEGG_CODEC_VORBIS Track codec is Vorbis.
    @retval -1 Error. */
int nestegg_track_codec_id(nestegg * context, unsigned int track);

/** Query the number of codec initialization chunks for @a track.  Each
    chunk of data should be passed to the codec initialization functions in
    the order returned.
    @param context Stream context initialized by #nestegg_init.
    @param track   Zero based track number.
    @param count   Storage for the queried chunk count.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_track_codec_data_count(nestegg * context, unsigned int track,
                                   unsigned int * count);

/** Get a pointer to chunk number @a item of codec initialization data for
    @a track.
    @param context Stream context initialized by #nestegg_init.
    @param track   Zero based track number.
    @param item    Zero based chunk item number.
    @param data    Storage for the queried data pointer.
                   The data is owned by the #nestegg context.
    @param length  Storage for the queried data size.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_track_codec_data(nestegg * context, unsigned int track, unsigned int item,
                             unsigned char ** data, size_t * length);

/** Query the video parameters specified by @a track.
    @param context Stream context initialized by #nestegg_init.
    @param track   Zero based track number.
    @param params  Storage for the queried video parameters.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_track_video_params(nestegg * context, unsigned int track,
                               nestegg_video_params * params);

/** Query the audio parameters specified by @a track.
    @param context Stream context initialized by #nestegg_init.
    @param track   Zero based track number.
    @param params  Storage for the queried audio parameters.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_track_audio_params(nestegg * context, unsigned int track,
                               nestegg_audio_params * params);

/** Read a packet of media data.  A packet consists of one or more chunks of
    data associated with a single track.  nestegg_read_packet should be
    called in a loop while the return value is 1 to drive the stream parser
    forward.  @see nestegg_free_packet
    @param context Context returned by #nestegg_init.
    @param packet  Storage for the returned nestegg_packet.
    @retval  1 Additional packets may be read in subsequent calls.
    @retval  0 End of stream.
    @retval -1 Error. */
int nestegg_read_packet(nestegg * context, nestegg_packet ** packet);

/** Destroy a nestegg_packet and free associated memory.
    @param packet #nestegg_packet to be freed. @see nestegg_read_packet */
void nestegg_free_packet(nestegg_packet * packet);

/** Query the track number of @a packet.
    @param packet Packet initialized by #nestegg_read_packet.
    @param track  Storage for the queried zero based track index.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_packet_track(nestegg_packet * packet, unsigned int * track);

/** Query the time stamp in nanoseconds of @a packet.
    @param packet Packet initialized by #nestegg_read_packet.
    @param tstamp Storage for the queried timestamp in nanoseconds.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_packet_tstamp(nestegg_packet * packet, uint64_t * tstamp);

/** Query the number of data chunks contained in @a packet.
    @param packet Packet initialized by #nestegg_read_packet.
    @param count  Storage for the queried timestamp in nanoseconds.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_packet_count(nestegg_packet * packet, unsigned int * count);

/** Get a pointer to chunk number @a item of packet data.
    @param packet  Packet initialized by #nestegg_read_packet.
    @param item    Zero based chunk item number.
    @param data    Storage for the queried data pointer.
                   The data is owned by the #nestegg_packet packet.
    @param length  Storage for the queried data size.
    @retval  0 Success.
    @retval -1 Error. */
int nestegg_packet_data(nestegg_packet * packet, unsigned int item,
                        unsigned char ** data, size_t * length);



int
nestegg_write_track_data(nestegg * ctx, char *buffer);

typedef enum {
	KRADEBML_FILE,
	KRADEBML_STREAM,
	KRADEBML_BUFFER,
} kradebml_intput_type_t;


typedef struct server_St server_t;
typedef struct base64_St base64_t;

struct base64_St {
	int len;
	int chunk;
	char *out;
	char *result;	
};

struct server_St {
	
	int header_pos;
	char headers[1024];
	char auth_base64[512];
	char audio_info[256];
	char auth[512];
	char mount[512];
	char password[128];
	int public;
	char content_type[128];
	
	server_type_t type;

	struct sockaddr_in serveraddr;
	struct hostent *hostp;
	char host[256];
	int port;	
	int sd;
	int sent;
	int connected;
	unsigned long total_bytes_sent;
	
};

struct kradebml_St {
	
	server_t *server;
	
	FILE *fp;
	nestegg *ctx;
	nestegg_audio_params aparams;
	nestegg_packet *pkt;
	nestegg_video_params vparams;
	size_t length, size;
	uint64_t duration, tstamp, pkt_tstamp;
	unsigned char * codec_data, * ptr;
	unsigned int cnt, i, j, track, tracks, pkt_cnt, pkt_track;
	unsigned int data_items;
	int type;
	nestegg_io io;
	nestegg_info info;
	char filename[512];
	EbmlGlobal *ebml;
	nestegg_log log_callback;
	
	int total_audio_frames;
	float audio_sample_rate;
	int audio_channels;
	
	int kludgecount;	
	
	
	//jack_ringbuffer_t *input_ring;
	//jack_ringbuffer_t *output_ring;
	char *input_buffer;
	pthread_t processing_thread;
	int shutdown;
	int synced;
	uint64_t total_bytes_read;
	uint64_t total_bytes_wrote;
	uint64_t last_cluster_pos;
	
	
	kradebml_intput_type_t input_type;
	char input_name[1024];
	
	kradebml_intput_type_t output_type;
	char output_name[1024];
	
	int audio_track;
	int video_track;
	
	
	unsigned char vorbis_header1[8192];
	int vorbis_header1_len;

	unsigned char vorbis_header2[8192];
	int vorbis_header2_len;
	
	unsigned char vorbis_header3[8192];
	int vorbis_header3_len;
	
	unsigned char theora_header1[8192];
	int theora_header1_len;

	unsigned char theora_header2[8192];
	int theora_header2_len;
	
	unsigned char theora_header3[8192];
	int theora_header3_len;
	
	krad_codec_type_t audio_codec;
	krad_codec_type_t video_codec;
	
	int next_tracknumber;
	int total_tracks;
	
	char input_info[4096];
	int input_info_pos;

	int audio_frames_since_cluster;

};

char *kradebml_version();

int krad_ebml_find_first_cluster(char *buffer, int len);
unsigned char get_next_match_byte(unsigned char match_byte, uint64_t position, uint64_t *matched_byte_num, uint64_t *winner);

char *kradebml_clone_tracks(kradebml_t *kradebml, kradebml_t *kradebml_output);

char *kradebml_input_info(kradebml_t *kradebml);
char *krad_ebml_track_codec_string(kradebml_t *kradebml, unsigned int track);
int krad_ebml_track_codec(kradebml_t *kradebml, unsigned int track);

void server_send(server_t *server, char *buffer, int length);
server_t *server_create(char *host, int port, char *mount, char *password);
void server_destroy(server_t *server);
void server_disconnect(server_t *server);
int server_connect(server_t *server);
int kradebml_open_input_stream(kradebml_t *kradebml, char *host, int port, char *mount);
int kradebml_open_input_advanced(kradebml_t *kradebml);
int kradebml_open_output_stream(kradebml_t *kradebml, char *host, int port, char *mount, char *password);
void kradebml_debug(kradebml_t *kradebml);
int kradebml_read_video(kradebml_t *kradebml, unsigned char *buffer);
int kradebml_read_audio(kradebml_t *kradebml, unsigned char *buffer);
int kradebml_read_audio_header(kradebml_t *kradebml, int tracknum, unsigned char *buffer);
int kradebml_open_input_file(kradebml_t *kradebml, char *filename);
void kradebml_cluster(kradebml_t *kradebml, int timecode);
void kradebml_add_audio(kradebml_t *kradebml, int track_num, unsigned char *buffer, int bufferlen, int frames);
kradebml_t *kradebml_create();
int kradebml_add_subtitle_track(kradebml_t *kradebml, char *codec_id);
int kradebml_add_audio_track(kradebml_t *kradebml, char *codec_id, float sample_rate, int channels, unsigned char *private_data, int private_data_size);
void kradebml_start_segment(kradebml_t *kradebml, char *appversion);
void kradebml_end_segment(kradebml_t *kradebml);

kradebml_t *kradebml_create_feedbuffer();
size_t kradebml_read_space(kradebml_t *kradebml);
int kradebml_read(kradebml_t *kradebml, char *buffer, int len);
int kradebml_last_was_sync(kradebml_t *kradebml);
char *kradebml_write_buffer(kradebml_t *kradebml, int len);
int kradebml_wrote(kradebml_t *kradebml, int len);
int kradebml_new_tracknumber(kradebml_t *kradebml);

int kradebml_feedbuffer_open(void *userdata);
int kradebml_feedbuffer_close(void *userdata);
int kradebml_feedbuffer_write(void *buffer, size_t length, void *userdata);
int kradebml_feedbuffer_read(void *buffer, size_t length, void *userdata);
int kradebml_feedbuffer_seek(int64_t offset, int whence, void *userdata);
int64_t kradebml_feedbuffer_tell(void *userdata);

int kradebml_stdio_write(void *buffer, size_t length, void *userdata);

int kradebml_stdio_read(void *buffer, size_t length, void *userdata);

int kradebml_stdio_seek(int64_t offset, int whence, void *userdata);

int64_t kradebml_stdio_tell(void *userdata);
void kradebml_close_output(kradebml_t *kradebml);
void kradebml_header(kradebml_t *kradebml, char *doctype, char *appversion);
int kradebml_write(kradebml_t *kradebml);
void kradebml_destroy(kradebml_t *kradebml);
int kradebml_kludge_init(kradebml_t *kradebml);
int kradebml_open_output_file(kradebml_t *kradebml, char *filename);
int kradebml_add_video_track_with_priv(kradebml_t *kradebml, char *codec_id, int frame_rate, int width, int height, unsigned char *private_data, int private_data_size);
int kradebml_add_video_track(kradebml_t *kradebml, char *codec_id, int frame_rate, int width, int height);
void kradebml_add_video(kradebml_t *kradebml, int track_num, unsigned char *buffer, int bufferlen, int keyframe);







#define KRAD_EBML_HEADER_MAX_SIZE 8192 * 4
#define KRAD_EBML_BUFFER_SIZE 8192 * 512

unsigned char get_next_match_byte(unsigned char match_byte, uint64_t position, uint64_t *matched_byte_num, uint64_t *winner);

typedef struct krad_ebml_St krad_ebml_t;

struct krad_ebml_St {
	
	char cluster_mark[4];
	int cluster_count;

	int ret;
	int b;
	uint64_t position;
	uint64_t read_position;
	
	int buffer_position_internal;
	uint64_t cluster_position;
	int header_read;
	
	int header_size;
	int header_position;
	int header_read_position;

	unsigned char *input_buffer;
	unsigned char *buffer;
	unsigned char *header;

	uint64_t found;
	uint64_t matched_byte_num;
	unsigned char match_byte;
	int endcut;
	
	int last_was_cluster_end;
	int this_was_cluster_start;
	
};

void krad_ebml_destroy(krad_ebml_t *krad_ebml);
krad_ebml_t *krad_ebml_create_feedbuffer();
size_t krad_ebml_read_space(krad_ebml_t *krad_ebml);
int krad_ebml_read(krad_ebml_t *krad_ebml, char *buffer, int len);
int krad_ebml_last_was_sync(krad_ebml_t *krad_ebml);
char *krad_ebml_write_buffer(krad_ebml_t *krad_ebml, int len);
int krad_ebml_wrote(krad_ebml_t *krad_ebml, int len);




#ifdef __cplusplus
}
#endif


