#define  CS5536_VENDOR         0X1022    // Vendor of 5536
#define  CS5536_DEVICE         0X2093    // Device of AC97
#define  CS5536_BASEADDR_REG   0X10

// ACC NATIVE REGISTERS
#define  ACC_GPIO_STATUS       0X00
#define  ACC_GPIO_CNTL         0X04
#define  ACC_CODEC_STATUS      0X08
#define  ACC_CODEC_CNTL        0X0c
#define  ACC_IRQ_STATUS        0X12
#define  ACC_IRQ_CNTL          0X14
#define  ACC_BM0_CMD           0X20
#define  ACC_BM0_STATUS        0X21
#define  ACC_BM0_PRD           0X24
#define  ACC_BM1_CMD           0X28
#define  ACC_BM1_STATUS        0X29
#define  ACC_BM1_PRD           0X2c
#define  ACC_BM2_CMD           0X30
#define  ACC_BM2_STATUS        0X31
#define  ACC_BM2_PRD           0X34
#define  ACC_BM3_CMD           0X38
#define  ACC_BM3_STATUS        0X39
#define  ACC_BM3_PRD           0X3c
#define  ACC_BM4_CMD           0X40
#define  ACC_BM4_STATUS        0X41
#define  ACC_BM4_PRD           0X44
#define  ACC_BM5_CMD           0X48
#define  ACC_BM5_STATUS        0X49
#define  ACC_BM5_PRD           0X4c
#define  ACC_BM6_CMD           0X50
#define  ACC_BM6_STATUS        0X51
#define  ACC_BM6_PRD           0X54
#define  ACC_BM7_CMD           0X58
#define  ACC_BM7_STATUS        0X59
#define  ACC_BM7_PRD           0X5c
#define  ACC_BM0_PNTR          0X60
#define  ACC_BM1_PNTR          0X64
#define  ACC_BM2_PNTR          0X68
#define  ACC_BM3_PNTR          0X6c
#define  ACC_BM4_PNTR          0X70
#define  ACC_BM5_PNTR          0X74
#define  ACC_BM6_PNTR          0X78
#define  ACC_BM7_PNTR          0X7c
// For ACC_CODEC_STATUS
#define  PRM_RDY_STS           0X00800000     // BIT 23
#define  STS_NEW               0X00020000     // BIT 17
// For ACC_CODEC_CNTL
#define  RW_CMD                0X80000000     // bit 31
#define  CMD_ADD               0X7f000000     // bits 24-30
#define  COMM_SEL              0X00c00000     // bit 22 23
#define  LNK_SHUTDOWN          0X00040000     // bit 18
#define  LNK_WARM_RST          0X00020000     // bit 17
#define  CMD_NEW               0X00010000     // bit 16
#define  CMD_DATA              0X0000ffff     // bits 0-15

// Mixer Registers of Codec
#define  CODEC_RESET           0X00
#define  MASTER_VOLUME         0X02
#define  HEADPHONE_VOLUME      0X04
#define  MONO_VOLUME           0X06
#define  PCBEEP_VOLUME         0X0A
#define  PHONE_VOLUME          0X0C
#define  MIC_VOLUME            0X0E
#define  LINEIN_VOLUME         0X10
#define  CD_VOLUME             0X12
#define  VIDEO_VOLUME          0X14
#define  AUX_VOLUME            0X16
#define  PCMOUT_VOLUME         0X18
#define  RECORD_SELECT         0X1A
#define  RECORD_GAIN           0X1C
#define  GENERAL_PURPOS        0X20
#define  D3_CONTROL            0X22
#define  POWERDOWN_STATUS      0X26
#define  EXTAUDIO_ID           0X28
#define  EXTAUDIO_STATUS       0X2A
#define  PCMOUT_SAMPLERATE     0X2C
#define  PCMIN_SAMPLERATE      0X32
#define  SPDIF_CTL             0X3A
#define  GPIO_SETUP            0X76
#define  GPIO_STATUS           0X78
#define  VENDOR_ID1            0X7C
#define  VENDOR_ID2            0X7E

// Volume
#define  PCBEEP_VOLUME_VALUE    0X0000
#define  MONO_VOLUME_VALUE      0X8000
#define  HEADPHONE_VOLUME_VALUE 0X0
#define  PCMOUT_VOLUME_VALUE    0X0           // Max Volume
#define  MASTER_VOLUME_VALUE    0X0           // Max Volume

#define WAVE_HEAD_LENGTH        44      // length of head of wav file

#define  EOT                   0X80000000    // PRD control bits
#define  EOP                   0X40000000

struct wavHead {
  char flag1[4];
  unsigned long int fileSize;
  char flag2[4];
  char flag3[4];
  char reserve[4];
  unsigned short int format;
  unsigned short int channel;
  unsigned short int rate;
  unsigned long int speed;
  unsigned short int dataBlock;
  unsigned short int dataBits;
  char flag4[4];
  unsigned long int dataLength;
};

struct PRD {
  char *baseAddr;
  unsigned long int ctrl;
};

DOS_MEM wavPRD(8 * 8);    // 8 PDRs. 8 bytes per PRD.
DOS_MEM wavBuf[8];

class MYSOUND {
  protected:
    __dpmi_regs  r;
    unsigned int busNo;          // Bus Number
    unsigned int devNo;          // Device Number
    unsigned int funcNo;         // Function Number
    unsigned int devFunc;        //
    unsigned long int baseAddr;  // Base address of mixer
    FILE *wavFile;               // Handle of wave file
    unsigned long int filePos;   // Position of WAV file
    int volume;                  // Volume

    wavHead *fileHead;

    int status;                  // 0--not play  1--playing

     unsigned long int i, j, k;  // temp var
     char accValue;
     
    /**************************************
     * CheckPCIBios
     * Return Value: 1--Support PCI
     *               0--Not Support PCI
     **************************************/
    unsigned int CheckPCIBios(void) {
      unsigned int i;

      // Check BIOS Support PCI or not
      r.x.ax = 0xb101;
      __dpmi_int(0x1a, &r);
      i = r.x.flags;
      if ((i & 0x01) == 0) return 1;
      else return 0;
    }
    /****************************************
     * Find CS5536's PCI Configuration space
     * Return : 0--Found CS5536
     *          1--No CS5536
     ****************************************/
    unsigned int FindCS5536(void) {
      r.x.ax = 0xb102;
      r.x.cx = CS5536_DEVICE;  // Device ID
      r.x.dx = CS5536_VENDOR;  // Vendor ID
      r.x.si = 0;              // Device index 0--n
      __dpmi_int(0x1a, &r);    // Find PCI device
      busNo = r.h.bh;          // Bus nnmber
      devFunc = r.h.bl;        // device/function no(bits 7-3 device, bits 2-0 function)
      return r.h.ah;
    }
    /**********************************************
     * Read a byte fromPCI Configuration register
     * Input : Register Index
     * Return : byte in the register
     **********************************************/
    unsigned int ReadConfigByte(unsigned int reg) {
      // Read Configure Byte
      r.x.ax = 0xb108;
      r.h.bl = devFunc;
      r.h.bh = busNo;           // Bus number
      r.x.di = reg;             // register no
      __dpmi_int(0x1a, &r);
      return r.h.cl;
    }
    /**********************************************
     * Read a word fromPCI Configuration register
     * Input : Register Index
     * Return : word in the register
     **********************************************/
    unsigned int ReadConfigWord(unsigned int reg) {
      // Read Configure Byte
      r.x.ax = 0xb10a;
      r.h.bl = devFunc;
      r.h.bh = busNo;           // Bus number
      r.x.di = reg;             // register no
      __dpmi_int(0x1a, &r);
      return r.x.cx;
    }
    /**********************************************
     * Read a dword fromPCI Configuration register
     * Input : Register Index
     * Return : dword in the register
     **********************************************/
    unsigned long int ReadConfigDword(unsigned int reg) {
      // Read Configure Byte
      r.x.ax = 0xb10a;
      r.h.bl = devFunc;
      r.h.bh = busNo;           // Bus number
      r.x.di = reg;             // register no
      __dpmi_int(0x1a, &r);
      return r.d.ecx;
    }
    /**********************************************
     * Open Wave File
     * Input : filename of WAV file
     * Return : if success, FILE handle
     *          if Fail, NULL
     **********************************************/
    FILE *OpenWavFile(char *filename) {
      return fopen(filename, "rb");
    }
    /**********************************************
     * Close Wave File
     **********************************************/
    void CloseWavFile() {
      fclose(wavFile);
    }
    /**********************************************
     * Check the format of wave file
     * Return : 0 if WAV file
     *         -1 if not WAV file
     **********************************************/
    int CheckWavFormat(void) {
      int fileLen;

      fileLen = fread(fileHead, 1, WAVE_HEAD_LENGTH, wavFile);

      if (fileLen != WAVE_HEAD_LENGTH) {
        printf("\nReading WAV head fail!");
        return -1;
      }

      if (fileHead->flag1[0] != 'R' | fileHead->flag1[1] != 'I' |
          fileHead->flag1[2] != 'F' | fileHead->flag1[3] != 'F') {
        return -1;
      }
      if (fileHead->flag2[0] != 'W' | fileHead->flag2[1] != 'A' |
          fileHead->flag2[2] != 'V' | fileHead->flag2[3] != 'E') {
        return -1;
      }
      if (fileHead->flag3[0] != 'f' | fileHead->flag3[1] != 'm' |
          fileHead->flag3[2] != 't') {
        return -1;
      }
      if (fileHead->flag4[0] != 'd' | fileHead->flag4[1] != 'a' |
          fileHead->flag4[2] != 't' | fileHead->flag4[3] != 'a') {
        return -1;
      }
      return 0;
    }
    /*********************************************
     * Create a PRD table
     * Input : LoadWavFile ok
     * Return : 0
     *********************************************/
    int CreatePRD() {
      DWORD longInt, k, p;
      int n;
      unsigned int i, j, m;
      char *wavBuffer;

      // format is single channel, 8 bits.
      // i have to fill buffer with 2 channels and 16bits.
      // so i can only read 16382 bytes to fill buffer of 65536;
      if (fileHead->dataBits == 8 && fileHead->channel == 1)
        m = 16382;
      // format if 2 channels and 16 bits
      else if (fileHead->dataBits == 16 && fileHead->channel == 2)
        m = 65530;
      // format is maybe 2 channels and 8 bits or single channel and 16 bits.
      else m = 32756;
      k = fileHead->dataLength;
      i = 0;
      wavBuffer = (char *)malloc(65536);     // Get a buffer for wave file
      while (k > 0) {
        if (k > m) longInt = m;
        else longInt = k;
        wavBuf[i].AllocateMem(65531);
        printf("\nAllocate memeory---%d", i);
        fseek(wavFile, filePos, SEEK_SET);
        p = fread(wavBuffer, 1, longInt, wavFile);
        filePos += p;
        printf("\nMove Data ---- %ld---%ld", longInt, p);

        k -= p;
        longInt = 0;
        if (fileHead->dataBits == 16 && fileHead->channel == 2)
          // 2 channels and 16 bits
          for (j = 0; j < p; j += 2) {
            wavBuf[i].SetBYTE(j, wavBuffer[j]);
            wavBuf[i].SetBYTE(j + 1, wavBuffer[j + 1]);
            longInt += 2;
          }
        else if (fileHead->dataBits == 16 && fileHead->channel == 1)
          // single channel and 16 bits
          for (j = 0; j < p; j += 2) {
            wavBuf[i].SetBYTE(j * 2, wavBuffer[j]);
            wavBuf[i].SetBYTE(j * 2 + 1, wavBuffer[j + 1]);
            wavBuf[i].SetBYTE(j * 2 + 2, wavBuffer[j]);
            wavBuf[i].SetBYTE(j * 2 + 3, wavBuffer[j + 1]);
            longInt += 4;
          }
        else if (fileHead->dataBits == 8 && fileHead->channel == 1)
          // single channel and 8 bits
          for (j = 0; j < p; j++) {
            n = wavBuffer[j];
            n = ((n - 128) * 32767) / 128;
            wavBuf[i].SetWORD(j * 4, n);
            wavBuf[i].SetWORD(j * 4 + 2, n);
            longInt += 4;
          }
        else if (fileHead->dataBits == 8 && fileHead->channel == 2)
          // 2 channels and 8 bits
          for (j = 0; j < p; j++) {
            n = wavBuffer[j];
            n = ((n - 128) * 32767) / 128;
            wavBuf[i].SetWORD(j * 2, n);
            longInt += 2;
          }
        if (p == m) {
          longInt = longInt | EOP;
        } else {
          longInt = longInt | EOT;     // set EOT
        }
        wavPRD.SetDWORD(i * 8, wavBuf[i].GetAddress());
        wavPRD.SetDWORD(i * 8 + 4, longInt);
        i++;
      }
      free(wavBuffer);
      return 0;
    }
    /*********************************************
     * Read a dword from codec register
     * Input : Register index of Codec
     * Return : Dword of register if success
     *          0xffffffff else
     *********************************************/
    unsigned long int ReadCodec(unsigned int reg) {
      unsigned int i;
      unsigned long int longInt;

      longInt = inportl(baseAddr + ACC_CODEC_STATUS);
      while ((longInt & PRM_RDY_STS) == 0) {
        printf("\nPRM_RDY_SYS0 == 0 reg = 0X%x", reg);
        delay(100);
        longInt = inportl(baseAddr + ACC_CODEC_STATUS);
      }
      // Wait for CMD_NEW is clear
      longInt = inportl(baseAddr + ACC_CODEC_CNTL);
      while ((longInt & CMD_NEW) != 0) {
        //printf("\nCMD_NEW0 == 0  reg = 0X%02x  value = 0X%08x", reg, longInt);
        delay(100);
        longInt = inportl(baseAddr + ACC_CODEC_CNTL);
      }
      // make ACC_CODEC_CNTL
      longInt = reg;
      longInt = longInt << 24;            // CMD Address
      longInt = longInt | RW_CMD;
      outportl(baseAddr + ACC_CODEC_CNTL, longInt);
      longInt = longInt | CMD_NEW;
      outportl(baseAddr + ACC_CODEC_CNTL, longInt);

      for (i = 0; i < 20; i++) {
        longInt = inportl(baseAddr + ACC_CODEC_STATUS);
        if ((longInt & STS_NEW) != 0) {
          return (longInt & 0Xffff);
        }
        delay(100);
      }
      return 0Xffffffff;
    }
    /*********************************************
     * Write a word to codec register
     *********************************************/
    void WriteCodec(unsigned int reg, unsigned short int value) {
      unsigned long int longInt;

      // wait for PRM_RDY_STS
      longInt = inportl(baseAddr + ACC_CODEC_STATUS);
      while ((longInt & PRM_RDY_STS) == 0) {
        printf("\nPRM_RDY_SYS0 == 0 reg = 0X%x", reg);
        delay(100);
        longInt = inportl(baseAddr + ACC_CODEC_STATUS);
      }
      //Wait for CMD_NEW is clear
      longInt = inportl(baseAddr + ACC_CODEC_CNTL);
      while ((longInt & CMD_NEW) != 0) {
        printf("\nCMD_NEW0 == 0  reg = 0X%02x  value = 0X%08x", reg, longInt);
        delay(100);
        longInt = inportl(baseAddr + ACC_CODEC_CNTL);
      }
      // make ACC_CODEC_CNTL
      longInt = reg;
      longInt = longInt << 24;            // CMD Address
      longInt += value;                   // CMD DATA
      longInt = longInt & (~RW_CMD);      // WRITE COMMAND
      if (reg == PCMOUT_SAMPLERATE) {
        printf("\nlongInt = 0X%0x  reg = 0X%02x  value=0X%04x", longInt, reg, value);
      }
      outportl(baseAddr + ACC_CODEC_CNTL, longInt);
      longInt = longInt | CMD_NEW;        // CMD NEW
      outportl(baseAddr + ACC_CODEC_CNTL, longInt);
    }


  public:
    MYSOUND(void) {
      // Check BIOS Support PCI or not
      if (CheckPCIBios() == 0) {
        printf("\nNot Support PCI BIOS!");
        return;
      }
      // Find CS5536. After this Step I will get Bus Number, Device Number
      // and Function Number
      if (FindCS5536() != 0) {
        printf("\nCould not find CS5536!");
        return;
      }
      devNo = devFunc >> 3;     // Device Number
      funcNo = devFunc & 07;    // Function Number
      // Get CS5536 Base address from PCI configuration space
      // Configuration register index is 0x10
      baseAddr = ReadConfigDword(CS5536_BASEADDR_REG);
      baseAddr = baseAddr & 0Xfffffff0;
      filePos = 44;
      status = 0;
    }
    /*********************************************
     * Initial CS5536 AND CODEC
     * initial ACC Native Registers as follow:
     * ACC_CODEC_STATUS
     * ACC_CODEC_CNTL
     * ACC_BM0_CMD
     * ACC_BM0_PRD       BIT 2-31  PRD POINTER
     *
     * Input : NONE
     * Return : NONE
     *********************************************/
    void InitialAC97(void) {
      unsigned long int longInt;
      char tempS[20];

      // set PRD pointer to ACC_BM0_PRD
      longInt = wavPRD.GetAddress();
      outportl(baseAddr + ACC_BM0_PRD, longInt);

      WriteCodec(EXTAUDIO_STATUS, 0X01);
      WriteCodec(PCMOUT_SAMPLERATE, fileHead->rate);  // set sample rate
      WriteCodec(PCMOUT_VOLUME, PCMOUT_VOLUME_VALUE);  // set head phone volume
      WriteCodec(MASTER_VOLUME, volume); // set master volume
    }
    /**********************************************
     * Load wav file
     * Input : filename
     * Return : 0 if file exist and format ok and
     *               Creating PRD ok
     *          1 else
     **********************************************/
    int LoadWavFile(char * filename) {
      wavFile = OpenWavFile(filename);  // Open .Wav File
      if (wavFile == NULL) {
        printf("\nFile does not exist.");
        return 1;
      }
      fileHead = (wavHead *)malloc(sizeof(wavHead));
      printf("\nCheck file format!");
      if (CheckWavFormat() == -1) {         // Check File Format of wave file
        printf("\nInvalid Format of wave file.");
        free(fileHead);
        CloseWavFile();
        return 1;
      }
      printf("\nCreate PRD table!");
      if (CreatePRD() == -1) {    // Create PRD Table
        printf("\nWAV File is too long.");
        return 1;
      }
      return 0;
    }
    /**********************************************
     * Start Play
     **********************************************/
    void StartPlay() {
      unsigned long int longInt;
  
      // wait for PRM_RDY_STS
      longInt = inportl(baseAddr + ACC_CODEC_STATUS);
      while ((longInt & PRM_RDY_STS) == 0) {
        printf("\nPRM_RDY_SYS1 == 0");
        sleep(1);
        longInt = inportl(baseAddr + ACC_CODEC_STATUS);
      }
      outportl(baseAddr + ACC_BM0_CMD, 0X01);
      status = 1;
    }
    int GetStatus(void) {
      return status;
    }
    /**********************************
     * Setting var volume
     * Input : v -- Volume 0--99
     *         v = 99 if v > 99
     * Return : 0
     **********************************/
    int SetVolume(int v) {
      if (v > 99) v = 99;
      v = (99 - v) * 63 / 99;
      volume = v + v * 256;
      printf("\nVolume = %d", volume);
      return 0;
    }
    void Process(void) {
      if (status == 1) {
        accValue = inportb(baseAddr + ACC_BM0_STATUS);
        if ((accValue & 1) != 0) printf("\nCompletion of a PRD");
        accValue = inportb(baseAddr + ACC_BM0_CMD);
        accValue = accValue & 0X03;
        if (accValue == 0) {
          free(fileHead);
          status = 0;
        }
      }
    }
};

