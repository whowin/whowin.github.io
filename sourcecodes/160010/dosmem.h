/**********************************
 * Dos(Real Mode) Memory Class
 **********************************/
class DOS_MEM {
  protected:
    DWORD address;
    DWORD length;
    int selector;

  public:
    DOS_MEM(DWORD length_in_bytes) {
      AllocateMem(length_in_bytes);
    }
    DOS_MEM(void) { };
    ~DOS_MEM(void) {
      if (length != 0)
        __dpmi_free_dos_memory(selector);
    }
    /****************************************
     * Allocate Dos Memory
     ****************************************/
    void AllocateMem(DWORD length_in_bytes) {
      int vbe_info;

      length = (length_in_bytes + 15) & 0xFFFF0;   // Round to nearest 16 bytes
      if ((vbe_info = __dpmi_allocate_dos_memory(length / 16, &selector)) == -1) {
        printf("ERROR allocating DOS memory\n");
        exit(10);
      }

      address = ((vbe_info << 4) & 0xffff0) + ((vbe_info >> 16) & 0xffff);
    }
    /***********************************
     * Get offset of memory
     ***********************************/
    int GetOffset(void) {
      return address & 0xF;
    }
    /***********************************
     * Get segment of memory
     ***********************************/
    int GetSegment(void) {
      return (address >> 4);
    }
    /***********************************
     * Get address of memory
     ***********************************/
    DWORD GetAddress(void) {
      return address;
    }
    /***********************************
     * Get selector of memory
     ***********************************/
    unsigned int GetSelector(void) {
      return selector;
    }
    DWORD GetDWORD(DWORD offset) {
      return _farpeekl(selector, offset);
    }
    WORD GetWORD(DWORD offset) {
      return _farpeekw(selector, offset);
    }
    BYTE GetBYTE(DWORD offset) {
      return _farpeekb(selector, offset);
    }
    void SetDWORD(DWORD offset, DWORD value) {
      _farpokel(selector, offset, value);
    }
    void SetWORD(DWORD offset, WORD value) {
      _farpokew(selector, offset, value);
    }
    void SetBYTE(DWORD offset, BYTE value) {
      _farpokeb(selector, offset, value);
    }
};
