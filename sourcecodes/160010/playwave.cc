#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <dpmi.h>
#include <dos.h>
#include <pc.h>
#include <io.h>
#include <string.h>
#include <unistd.h>
#include <sys/farptr.h>
#include <sys/segments.h>

#include "typedef.h"
#include "dosmem.h"
#include "ac97.h"

MYSOUND mysound;
/**********************************************
 * Play .WAV File.
 * Format: palywav [filename] [volume]
 * volume: 0-63
 **********************************************/
int main(int argc, char *argv[]) {
  // Check Parameters in cmdline
  if (argc != 3) {
    printf("\nUsage : playwav [filename] [volume]");
    return 1;
  }
  if (mysound.LoadWavFile(argv[1]) != 0 ){
    printf("\nLoad WAV file fail!");
    return 1;
  }
  mysound.SetVolume(atoi(argv[2]));
  mysound.InitialAC97();
  mysound.StartPlay();
  while (mysound.GetStatus() == 1) {
    mysound.Process();
  }

  return 0;
}

