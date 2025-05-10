#include <SI4735.h>

class SI4735_fixed: public SI4735
{
  public:
    // Fixing SI4735::getRdsPI() bug where it only returns BLOCKAL
    uint16_t getRdsPI(void)
    {
      if(getRdsReceived() && getRdsNewBlockA())
        return (currentRdsStatus.resp.BLOCKAH << 8) + currentRdsStatus.resp.BLOCKAL;
      else
        return 0x0000;
    }

    // Fixing SI4735::getRdsProgramType() bug where it only returns three lower bits
    uint8_t getRdsProgramTypeX(void)
    {
      uint16_t blockB = (currentRdsStatus.resp.BLOCKBH << 8) + currentRdsStatus.resp.BLOCKBL;
      return (blockB >> 5) & 0x1F;
    }

    // Fixing SI4735::getRdsText2A() which does not follow version bit
    char *getRdsText2A(void)
    {
      return getRdsVersionCode()? NULL : SI4735::getRdsText2A();
    }

    // Fixing SI4735::getRdsText2B() which does not follow version bit
    char *getRdsText2B(void)
    {
      return getRdsVersionCode()? SI4735::getRdsText2B() : NULL;
    }

    // Only one kind of text is available
    inline char *getRdsProgramInformation(void)
    {
      return getRdsVersionCode()? SI4735::getRdsText2B() : SI4735::getRdsText2A();
    }

    // Only one kind of text is available
    inline char *getRdsStationInformation(void)
    {
      return getRdsVersionCode()? SI4735::getRdsText2B() : SI4735::getRdsText2A();
    }

    // Copy-pasted from the parent class just to override setAM()
    void setAM(uint16_t fromFreq, uint16_t toFreq, uint16_t initialFreq, uint16_t step)
    {
      currentMinimumFrequency = fromFreq;
      currentMaximumFrequency = toFreq;
      currentStep = step;

      if (initialFreq < fromFreq || initialFreq > toFreq)
        initialFreq = fromFreq;

      setAM();
      currentWorkFrequency = initialFreq;
      setFrequency(currentWorkFrequency);
    }

    void setAM()
    {
      // Force powerDown and radioPowerUp, fixes https://github.com/esp32-si4732/ats-mini/issues/13#issuecomment-2742054451
      powerDown();
      setPowerUp(this->ctsIntEnable, 0, 0, this->currentClockType, AM_CURRENT_MODE, this->currentAudioMode);
      radioPowerUp();
      setAvcAmMaxGain(currentAvcAmMaxGain); // Set AM Automatic Volume Gain (default value is DEFAULT_CURRENT_AVC_AM_MAX_GAIN)
      setVolume(volume);                    // Set to previus configured volume
      currentSsbStatus = 0;
      lastMode = AM_CURRENT_MODE;
    }

#if 0
    // Speeding up SI4735::downloadPatch() function
    bool downloadPatch(const uint8_t *ssb_patch_content, const uint16_t ssb_patch_content_size)
    {
      for(uint16_t offset=0 ; offset<ssb_patch_content_size ; offset+=8)
      {
        Wire.beginTransmission(deviceAddress);

        for(uint16_t i=0 ; i<8 ; i++)
          Wire.write(pgm_read_byte_near(ssb_patch_content + (i + offset)));

        Wire.endTransmission();
        waitToSend();
      }

      delayMicroseconds(250);
      return true;
    }

    // Using the new downloadPatch() function here
    void loadPatch(const uint8_t *ssb_patch_content, const uint16_t ssb_patch_content_size, uint8_t ssb_audiobw)
    {
      queryLibraryId();                                                                                                       patchPowerUp();
      delay(50);                                                                                                              downloadPatch(ssb_patch_content, ssb_patch_content_size);                                                               // Parameters                                                                                                           // AUDIOBW - SSB Audio bandwidth; 0 = 1.2kHz (default); 1=2.2kHz; 2=3kHz; 3=4kHz; 4=500Hz; 5=1kHz;
      // SBCUTFLT SSB - side band cutoff filter for band passand low pass filter ( 0 or 1)                                    // AVC_DIVIDER  - set 0 for SSB mode; set 3 for SYNC mode.
      // AVCEN - SSB Automatic Volume Control (AVC) enable; 0=disable; 1=enable (default).
      // SMUTESEL - SSB Soft-mute Based on RSSI or SNR (0 or 1).
      // DSP_AFCDIS - DSP AFC Disable or enable; 0=SYNC MODE, AFC enable; 1=SSB MODE, AFC disable.
      setSSBConfig(ssb_audiobw, 1, 0, 0, 0, 1);
      delay(25);
    }
#endif
};
