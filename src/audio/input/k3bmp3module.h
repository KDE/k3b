#ifndef K3BMP3MODULE_H
#define K3BMP3MODULE_H


#include "k3baudiomodule.h"

#include <mad.h>
#include <stdio.h>

class QTimer;


class K3bMp3Module : public K3bAudioModule
{
  Q_OBJECT

 public:
  K3bMp3Module( K3bAudioTrack* track );
  ~K3bMp3Module();

  void start();

 public slots:
  void cancel();

 private slots:
  void slotDecodeNextFrame();
  void slotCountFrames();
  void slotConsumerReady();
 
 private:
  unsigned short madFixedToUshort( mad_fixed_t fixed );
  void initializeDecoding();
  void fillInputBuffer();

  bool m_bDecodingInProgress;
  bool m_bCountingFramesInProgress;
  bool m_bEndOfInput;
  bool m_bOutputFinished;

  QTimer* m_decodingTimer;

  unsigned long m_rawDataLengthToStream;
  unsigned long m_rawDataAlreadyStreamed;

  mad_stream*   m_madStream;
  mad_frame*    m_madFrame;
  mad_header*   m_madHeader;
  mad_synth*    m_madSynth;
  mad_timer_t*  m_madTimer;

  unsigned long m_frameCount;

  unsigned char* m_inputBuffer;
  unsigned char* m_outputBuffer;
  unsigned char* m_outputPointer;
  unsigned char* m_outputBufferEnd;

  FILE* m_inputFile;

  static const int INPUT_BUFFER_SIZE = 5*8192;
  static const int OUTPUT_BUFFER_SIZE = 10*8192;
};


#endif
