#include <minix/syslib.h>
#include <minix/drivers.h>
#include <machine/int86.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "vbe.h"
#include "video.h"
#include "lmlib.h"
#include "utils.h"
#include "video_gr.h"

/* Private global variables */

static char *video_mem;		/* Process address to which VRAM is mapped */

static unsigned h_res;		/* Horizontal screen resolution in pixels */
static unsigned v_res;		/* Vertical screen resolution in pixels */
static unsigned bits_per_pixel; /* Number of VRAM bits per pixel */

int vg_exit() {
  struct reg86u reg86;

  reg86.u.b.intno = 0x10; /* BIOS video services */

  reg86.u.b.ah = 0x00;    /* Set Video Mode function */
  reg86.u.b.al = 0x03;    /* 80x25 text mode*/

  if( sys_int86(&reg86) != OK ) {
      printf("\tvg_exit(): sys_int86() failed \n");
      return -1;
  } else
      return 0;
}

void* vg_init(unsigned short mode){
	lm_init();
	int r1;
	struct mem_range mr;
	unsigned int vram_base;
	unsigned int vram_size;

	struct reg86u r;
	r.u.w.ax = VBE_CALL | VBE_SET_MODE; // VBE call, function 02 -- set VBE mode
	r.u.w.bx = BIT(14) | mode; // set bit 14: linear framebuffer
	r.u.b.intno = INT10;

	if( sys_int86(&r) != OK ) {
		printf("set_vbe_mode: sys_int86() failed \n");
		return NULL;
	}

	vbe_mode_info_t current_mode_information;
	if(vbe_get_mode_info(mode, &current_mode_information) != OK)
			return NULL;

	vram_base = current_mode_information.PhysBasePtr;
	h_res = current_mode_information.XResolution;
	v_res = current_mode_information.YResolution;
	bits_per_pixel = current_mode_information.BitsPerPixel;
	vram_size = h_res*v_res*bits_per_pixel/8;

	/* Allow memory mapping */
	mr.mr_base = (phys_bytes) vram_base;
	mr.mr_limit = mr.mr_base + vram_size;

	if( OK != (r1 = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr)))
		panic("sys_privctl (ADD_MEM) failed: %d\n", r1);
	/* Map memory */
	video_mem = vm_map_phys(SELF, (void *)mr.mr_base, vram_size);
	if(video_mem == MAP_FAILED)
		panic("couldn’t map video memory");

	printf("Consegui %x \n", video_mem);
	return (void *)video_mem;
}

unsigned int vg_get_h_res(){
	return h_res;
}

unsigned int vg_get_v_res(){
	return v_res;
}

unsigned int vg_get_bytes_per_pixel(){
	return bits_per_pixel/8;
}

char* vg_get_video_mem(){
	return video_mem;
}

unsigned int vg_get_window_size(){
	return h_res*v_res*bits_per_pixel/8;
}
