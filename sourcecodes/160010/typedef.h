#define FALSE 0
#define TRUE (!FALSE)

typedef long               LONG;
typedef unsigned long      ULONG;
typedef long               DWORD;
typedef unsigned long      UDWORD;
typedef short int          WORD;
typedef unsigned short int UWORD;
typedef char               BYTE;
typedef unsigned char      UBYTE;

typedef BYTE BOOL;

typedef union {
  struct {
    UDWORD edi;
    UDWORD esi;
    UDWORD ebp;
    UDWORD res;
    UDWORD ebx;
    UDWORD edx;
    UDWORD ecx;
    UDWORD eax;
  } d;
  struct {
    UWORD di, di_hi;
    UWORD si, si_hi;
    UWORD bp, bp_hi;
    UWORD res, res_hi;
    UWORD bx, bx_hi;
    UWORD dx, dx_hi;
    UWORD cx, cx_hi;
    UWORD ax, ax_hi;
    UWORD flags;
    UWORD es;
    UWORD ds;
    UWORD fs;
    UWORD gs;
    UWORD ip;
    UWORD cs;
    UWORD sp;
    UWORD ss;
  } x;
  struct {
    UBYTE edi[4];
    UBYTE esi[4];
    UBYTE ebp[4];
    UBYTE res[4];
    UBYTE bl, bh, ebx_b2, ebx_b3;
    UBYTE dl, dh, edx_b2, edx_b3;
    UBYTE cl, ch, ecx_b2, ecx_b3;
    UBYTE al, ah, eax_b2, eax_b3;
  } h;
} X86_REGS;
