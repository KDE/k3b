#ifndef K3BMP3MODULE_H
#define K3BMP3MODULE_H


#include "../k3baudiomodule.h"

extern "C" {
#include "libmad/mad.h"
}

#include <qfile.h>

class QTimer;


class K3bMp3Module : public K3bAudioModule
{
  Q_OBJECT

 public:
  K3bMp3Module( QObject* parent = 0, const char* name = 0 );
  ~K3bMp3Module();

  bool canDecode( const KURL& url );

 public slots:
  void cancel();

 private slots:
  void slotDecodeNextFrame();
  void slotCountFrames();
  void slotConsumerReady();
  void startDecoding();
  void analyseTrack();
  void stopAnalysingTrack();
 
 private:
  inline unsigned short linearRound( mad_fixed_t fixed );
  void initializeDecoding();
  void fillInputBuffer();
  void clearingUp();
  void createPcmSamples( mad_synth* );
  unsigned int resampleBlock( mad_fixed_t const *source, 
			      unsigned int nsamples, 
			      mad_fixed_t* target,
			      mad_fixed_t& last,
			      mad_fixed_t& step );

  bool m_bDecodingInProgress;
  bool m_bCountingFramesInProgress;
  bool m_bEndOfInput;
  bool m_bOutputFinished;

  QTimer* m_decodingTimer;
  QTimer* m_analysingTimer;

  unsigned long m_rawDataLengthToStream;
  unsigned long m_rawDataAlreadyStreamed;

  mad_stream*   m_madStream;
  mad_frame*    m_madFrame;
  mad_header*   m_madHeader;
  mad_synth*    m_madSynth;
  mad_timer_t*  m_madTimer;

  // needed for resampling
  // ----------------------------------
  bool m_bFrameNeedsResampling;
  mad_fixed_t m_madResampledRatio;

  // left channel
  mad_fixed_t m_madResampledStepLeft;
  mad_fixed_t m_madResampledLastLeft;
  mad_fixed_t* m_madResampledLeftChannel;

  // right channel
  mad_fixed_t m_madResampledStepRight;
  mad_fixed_t m_madResampledLastRight;
  mad_fixed_t* m_madResampledRightChannel;
  // ----------------------------------

  unsigned long m_frameCount;

  unsigned char* m_inputBuffer;
  unsigned char* m_outputBuffer;
  unsigned char* m_outputPointer;
  unsigned char* m_outputBufferEnd;

  QFile m_inputFile;

  static const int INPUT_BUFFER_SIZE = 5*8192;
  static const int OUTPUT_BUFFER_SIZE = 20*8192;
};


#endif
