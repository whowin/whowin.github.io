	page    60, 132
	title   A RAM Disk Device Driver
;************************************************
;*      This is a RAM Disk Device Driver        *
;*      Author: Hua Songqing                    *
;*      Date:   2008, 7, 17                     *
;*      PurPose:A RAM Disk with audible tones   *
;*      This is a RAM Disk Device Driver        *
;************************************************

;************************************************
;*      ASSEMBLER DIRECTIVES
;************************************************
cseg            segment para public 'code'
		org     0
		assume  cs:cseg, es:cseg, ds:cseg
ramdisk         proc    far

;****Structures****
rh              struc           ;request header
rh_len          db      ?       ;length of packet
rh_unit         db      ?       ;unit code(block device only)
rh_cmd          db      ?       ;device driver command
rh_status       dw      ?       ;returned by device driver
rh_res1         dd      ?       ;reserved
rh_res2         dd      ?
rh              ends

rh0             struc           ;Initialization (Command 0)
rh0_rh          db      size rh dup(?)  ;Fixed portion
rh0_nunits      db      ?       ;number of units
rh0_brk_ofs     dw      ?       ;offset address for break
rh0_brk_seg     dw      ?       ;segment address for break
rh0_bpb_tbo     dw      ?       ;offset address of pointer to BPB array
rh0_bpb_tbs     dw      ?       ;segment address of pointer to BPB array
rh0_drv_ltr     db      ?       ;first available drive(block device only)
rh0             ends

rh1             struc           ;Media Check(Command 1)
rh1_rh          db      size rh dup(?)  ;Fixed portion
rh1_media       db      ?       ;media descriptor from DPB
rh1_md_stat     db      ?       ;media status returned by device driver
rh1             ends

rh2             struc           ;Get BPB(Command 2)
rh2_rh          db      size rh dup(?)  ;Fixed portion
rh2_media       db      ?       ;media descriptor from BPB
rh2_buf_ofs     dw      ?       ;offset address of data transfer area
rh2_buf_seg     dw      ?       ;segment address of data transfer area
rh2_pbpbo       dw      ?       ;offset address of pointer to BPB
rh2_pbpbs       dw      ?       ;segment address of pointer to BPB
rh2             ends

rh4             struc           ;INPUT(Command 4)
rh4_rh          db      size rh dup(?)  ;Fixed portion
rh4_media       db      ?       ;media descriptor
rh4_buf_ofs     dw      ?       ;offset address of data transfer area
rh4_buf_seg     dw      ?       ;segment address of data transfer area
rh4_count       dw      ?       ;transfer count(sectors for block)(bytes for character)
rh4_start       dw      ?       ;start sector number(block only)
rh4             ends

rh8             struc           ;OUTPUT(Command 8)
rh8_rh          db      size rh dup(?)
rh8_media       db      ?       ;same as rh4
rh8_buf_ofs     dw      ?       ;
rh8_buf_seg     dw      ?       ;
rh8_count       dw      ?       ;
rh8_start       dw      ?       ;
rh8             ends

rh9             struc           ;OUTPUT VERIFY(Command 9)
rh9_rh          db      size rh dup(?)
rh9_media       db      ?       ;same as rh4
rh9_buf_ofs     dw      ?       ;
rh9_buf_seg     dw      ?       ;
rh9_count       dw      ?       ;
rh9_start       dw      ?       ;
rh9             ends

rh15            struc           ;Removable(Command 15)
rh15_len        db      ?       ;length of packet
rh15_unit       db      ?       ;unit code
rh15_cmd        db      ?       ;device driver command
rh15_status     dw      ?       ;returned by device driver
rh15_res1       dd      ?       ;reserved
rh15_res2       dd      ?
rh15            ends

;Command that do not have unique portions to the request header
;       INPUT_STATUS    (command 6)
;       INPUT_FLUSH     (command 7)
;       OUTPUT_STATUS   (command 10)
;       OUTPUT_FLUSH    (command 11)
;       OPEN            (command 13)
;       CLOSE           (command 14)
;       REMOVABLE       (command 15)
;

;****************************************
;*      MAIN PROCEDURE CODE             *
;****************************************
begin:
start_address   EQU     $

;****************************************
;*      DEVICE HEADER REQUESTED BY DOS  *
;****************************************
next_dev        dd      -1              ;no device driver after this
attribute       dw      2000h           ;block device. non ibm format
stratrgy        dw      dev_strategy    ;address of strategy routine
interrupt       dw      dev_interrupt   ;address of interrupt routine
dev_name        db      1               ;number of block device
		db      7 dup(20h)      ;7 byte filter

;************************************************
;*      WORK SPACE FOR OUR DEVICE DRIVER        *
;************************************************
rh_ofs          dw      ?               ;offset address of request header
rh_seg          dw      ?               ;segment address of request header

boot_rec        equ     $               ;dummy DOS boot record
		db      3 dup(0)        ;not a jump instruction
		db      'HSQ  1.0'      ;vendor id

bpb             EQU     $               ;this is the Bios Parameter Block
bpb_ss          dw      512             ;512 byte sector size
bpb_au          db      1               ;cluster size is 1 sector
bpb_rs          dw      1               ;1 (boot) reserved sector
bpb_nf          db      1               ;1 FAT only
bpb_ds          dw      48              ;files in the file directory
bpb_ts          dw      205             ;sectors=100KB + 5 overhead
bpb_md          db      0feh            ;media descriptor
bpb_fs          dw      1               ;FAT sectors in each FAT

bpb_ptr         dw      bpb             ;bios parameter block pointer array(1 entry)

;current RAM disc information
total           dw      ?               ;transfer sector count
verify          db      0               ;verify 1=yes 0=no
start           dw      0               ;start sector number
disk            dw      0               ;RAM Disk start address
buf_ofs         dw      ?               ;data transfer offset address
buf_seg         dw      ?               ;data transfer segment address

res_cnt         dw      5               ;reserved sectors
ram_par         dw      6560            ;paragraphs of memory

;****************************************
;*      THE STRATEGY PROCEDURE          *
;****************************************
dev_strategy:
	mov     cs:rh_seg, es           ;save the segment address
	mov     cs:rh_ofs, bx           ;save the offset address
	ret

;****************************************
;*      THE INTERRUPT PROCEDURE         *
;****************************************

;device interrupt handler - 2nd call from DOS

dev_interrupt:
	cld
	push    ds
	push    es
	push    ax
	push    bx
	push    cx
	push    dx
	push    di
	push    si

	mov     ax, cs:rh_seg           ;restore segment address of request header
	mov     es, ax
	mov     bx, cs:rh_ofs           ;restore offset address of request header

;jump to appropriate routine to process command

	mov     al, es:[bx].rh_cmd      ;get request header command
	rol     al, 1                   ;times 2 for index into word table
	lea     di, cmdtab              ;command table address
	mov     ah, 0                   ;clear high order
	add     di, ax                  ;add the index to start of table
	jmp     word ptr [di]           ;jump indirect

;CMDTAB is the command table that contains the word address
;for each command. The request header will contain the
;command desired. The INTERRUPT routine will jump through an
;address corresponding to the requested command to get to
;the appropriate command processing routine

MAXCMD          equ     10h
cmdtab          label   byte
		dw      initialization  ;Command 0
		dw      media_check     ;1
		dw      get_bpb         ;2
		dw      ioctl_input     ;3
		dw      input           ;4
		dw      nd_input        ;5
		dw      input_status    ;6
		dw      input_flush     ;7
		dw      output          ;8
		dw      output_verify   ;9
		dw      output_status   ;0ah
		dw      output_flush    ;0bh
		dw      ioctl_out       ;0ch
		dw      open            ;0dh
		dw      close           ;0eh
		dw      removable       ;0fh
		dw      output_busy     ;10h

;********************************************
;*        Local Procedures
;********************************************

save    proc    near
;*****************************************
;       Save Data from Request Header
;       Called from INPUT, OUTPUT
;*****************************************
	mov     ax, es:[bx].rh4_buf_seg
	mov     cs:buf_seg, ax          ;Segment
	mov     ax, es:[bx].rh4_buf_ofs
	mov     cs:buf_ofs, ax          ;Offset
	mov     ax, es:[bx].rh4_start   ;Get start sector number
	mov     cs:start, ax
	mov     ax, es:[bx].rh4_count   ;Sectors to transfer
	mov     ah, 0                   ;clear high order
	mov     cs:total, ax
	ret
save    endp

cvt2seg proc    near            ;calculates momory address
;
;       requires cs:start       starting sector
;                cs:total       total sector count
;                cs:disk        RAM DISK start address
;
;       returns  ds             segment address
;                cx             count of total bytes
;                si             =0 for paragraph boundary
;       uses     ax
;                cx
;                si
;                ds
;
	mov     ax, cs:start    ;get starting sector number
	mov     cl, 5           ;multiply by 32 
	shl     ax, cl          ; 
	mov     cx, cs:disk     ;get start segment of RAM DISK
	add     cx, ax          ;add to initial segment
	
	mov     ds, cx          ;ds has start segment
	mov     si, 0           ;make it on a paragraph boundary
	mov     ax, cs:total    ;total number of sectors
	mov     cx, 512         ;bytes per sector
	mul     cx              ;multiply to get transfer length
	or      ax, ax          ;too large?
	jnz     calc1           ;no(less than 64kb)
	mov     ax, 0ffffh      ;yes - make it 64kb
calc1:
	mov     cx, ax          ;move length to cx
	ret
cvt2seg endp

;*****************************************
;*  DOS Command Processing
;*****************************************
;Command 0      Initialization
initialization:
;Get drive letter of RAM DISK
	mov     al, es:[bx].rh0_drv_ltr
	and     al, 7fh
	add     al, 41h
	mov     cs:drive, al            ;get drive letter
	call    initial                 ;display console message
	push    cs
	pop     dx
;Calculate end segment of RAM DISK
	lea     ax, cs:start_disk       ;start address of RAM DISK
	mov     cl, 1
	ror     ax, cl                  ;divide by 16 = paragraphs
	add     dx, ax                  ;add to current CS value
	mov     cs:disk, dx             ;RAM DISK Start address
	mov     ax, ram_par             ;add RAM Disk paragraphs
	add     dx, ax                  ;to start segment of RAM disk

;return the break address to DOS
	mov     es:[bx].rh0_brk_ofs, 0
	mov     es:[bx].rh0_brk_seg, dx

;return number of units for a block device
	mov     es:[bx].rh0_nunits, 1   ;only one RAM disk

;return address of array of BIOS Parameter Blocks (1 only)
	lea     dx, bpb_ptr             ;address of bpb pointer array
	mov     es:[bx].rh0_bpb_tbo, dx ;return offset
	mov     es:[bx].rh0_bpb_tbs, cs ;retuen segment

;initialize boot. FAT. Directory to zeroes.
	push    ds                      ;cvt2seg destroys ds
	mov     cs:start, 0             ;start sector=0
	mov     ax, cs:res_cnt          ;reserved sectors
	mov     cs:total, ax            ;
	call    cvt2seg
	mov     al, 0
	push    ds
	pop     es
	mov     di, si
	rep     stosb                   ;clear boot.FAT.Dirextory to zeroes

;mov boot record to sector 0
	pop     ds
	mov     es, cs:disk
	mov     di, 0
	lea     si, cs:boot_rec
	mov     cx, 24                  ;BPB:13bytes.Jump instruction:3bytes
	rep     movsb                   ;Vendor id: 8bytes

;build one and only one FAT
	mov     cs:start, 1             ;Logical sector 1
	mov     cs:total, 1             ;
	call    cvt2seg
	mov     ds:byte ptr [si], 0feh  ;media descriptor
	mov     ds:byte ptr 1[si], 0ffh ;
	mov     ds:byte ptr 2[si], 0ffh ;

;end of initialization - restore es:bx exit
	mov     ax, cs:rh_seg
	mov     es, ax
	mov     bx, cs:rh_ofs
	jmp     done

;Command 1      Media_Check
media_check:                            ;Block device only
	mov     es:[bx].rh1_media, 1    ;media is unchanged
	jmp     done

;Command 2      Get_BPB
get_bpb:                                ;Read boot record
	push    es
	push    bx
	mov     cs:start, 0             ;boot record=sector 0
	mov     cs:total, 1             ;1 sector
	call    cvt2seg                 ;converter to RAM DISK address
	push    cs
	pop     es
	lea     di, cs:bpb
	add     si, 11                  ;address of bios param block
	mov     cx, 13                  ;length of bpb
	rep     movsb                   ;move
	pop     bx
	pop     es
	mov     dx, cs:bpb_ptr          ;pointer to BPB array
	mov     es:[bx].rh2_pbpbo, dx
	mov     es:[bx].rh2_pbpbs, cs
	jmp     done

;Command 3      IOCTL_INPUT
ioctl_input:
	jmp     unknown

;Command 4      Input   Read RAM DISK and returned data to DOS
input:
	call    save                    ;save request header data
	call    cvt2seg
	mov     es, cs:buf_seg          ;set destination seg&ofs to es:di
	mov     di, cs:buf_ofs
	mov     ax, di
	add     ax, cx  
	jnc     input1                  ;overflow?
	mov     ax, 0ffffh              ;yes - use max transfer length
	sub     ax, di                  ;subtract offset from max
	add     cx, ax                  ;new transfer count

input1:
	rep     movsb                   ;Read RAM DISK to data area
	mov     ax, cs:rh_seg           ;restore es:bx
	mov     es, ax
	mov     bx, cs:rh_ofs
	jmp     done

;Command 5      ND Input
nd_input:
	jmp     busy

;Command 6      Input Status
input_status:
	jmp     done

;Command 7      Input Flush
input_flush:
	jmp     done

;Command 8      Output  Write data to RAM DISK
output:
	call    save                    ;save request header data
	call    cvt2seg
	push    ds
	pop     es
	mov     di, si                  ;es:di points to destination address
	mov     ds, cs:buf_seg
	mov     si, cs:buf_ofs          ;ds:si points to source
	rep     movsb                   ;transfer data
	mov     bx, cs:rh_ofs           ;restore es:bx
	mov     es, cs:rh_seg
	cmp     cs:verify, 0            ;verify?
	jz      out1                    ;no
	mov     cs:verify, 0            ;yes - clear verify flag
	jmp     input                   ;Input data
out1:
	mov     ax, cs:rh_seg           ;restore es:bx
	mov     es, ax
	mov     bx, cs:rh_ofs
	jmp     done

;Command 9      Output Verify
output_verify:
	mov     cs:verify, 1            ;set the verify flag
	jmp     output                  ;goto output

;Command 10     Output Status
output_status:
	jmp     done

;Command 11     Output Flush
output_flush:
	jmp     done

;Command 12     IOCTL Output
ioctl_out:
	jmp     unknown

;Command 13     Open
open:
	jmp     done

;Command 14     Close
close:
	jmp     done

;Command 15     Removable
removable:
	mov     es:[bx].rh_status, 0200h
	jmp     done

;Command 16     Output till busy
output_busy:
	jmp     unknown

;***********************************
;*      Error Exit                 *
;***********************************
unknown:
	or      es:[bx].rh_status, 8003h        ;set error bit and error code
	jmp     done

;***********************************
;*      Common Exit                *
;***********************************
busy:
	or      es:[bx].rh_status, 0200h        ;set busy bit
done:
	or      es:[bx].rh_status, 0100h        ;set done

	pop     si                      ;restore all registers
	pop     di
	pop     dx
	pop     cx
	pop     bx
	pop     ax
	pop     es
	pop     ds
	ret
;*************************************************
;*      End of Program                           *
;*************************************************
end_of_program:
;org to paragraph boundary for start of RAM DISK
	if      ($-start_address) mod 16
	org     ($-start_address)+16-(($-start_address) mod 16)
	endif

start_disk      equ     $

initial proc    near
	lea     dx, msg1
	mov     ah,09
	int     21h
	ret
initial endp

msg1    db      0dh, 0ah, 'The Whowin Group 100k RAM Disk', 0dh, 0ah, '  Drive = '
drive   db      ?
	db      ':', 0dh, 0ah, 0ah, '$'

ramdisk endp
cseg    ends
	end     begin

