

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "execlib.h"
#include "disk.h"
#include "rommgr.h"
#include "uae.h"
#include "threaddep/thread.h"
#include "keybuf.h"

#include "consolehook.h"

static uaecptr beginio;

void consolehook_config (struct uae_prefs *p)
{
	int roms[] = { 15, 31, 16, 46, -1 };

	default_prefs (p, 0);
	//p->headless = 1;
	p->produce_sound = 0;
	p->gfx_resolution = 0;
	p->gfx_vresolution = 0;
	p->gfx_scanlines = false;
	p->gfx_framerate = 10;
	p->immediate_blits = 1;
	p->collision_level = 0;
	configure_rom (p, roms, 0);
	p->cpu_model = 68020;
	p->fpu_model = 68882;
	p->m68k_speed = -1;
#ifdef JIT
	p->cachesize = 8192;
#endif
	p->cpu_compatible = 0;
	p->address_space_24 = 0;
	p->chipmem_size = 0x00200000;
	p->fastmem_size = 0x00800000;
	p->bogomem_size = 0;
	p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->floppy_speed = 0;
	p->start_gui = 0;
	p->gfx_size_win.width = 320;
	p->gfx_size_win.height = 256;
	p->turbo_emulation = 0;
	//p->win32_automount_drives = 2;
	//p->win32_automount_cddrives = 2;

#ifdef FILESYS
	add_filesys_config (p, -1, "DH0", "CLIBOOT", ".", 1, 0, 0, 0, 0, 15, NULL, 0, 0);
#endif
}

static void *console_thread (void *v)
{
	uae_set_thread_priority (2);
	for (;;) {
		TCHAR wc = ""; //console_getch ();
		char c[2];

		write_log ("*");
		c[0] = 0;
		c[1] = 0;
		ua_copy (c, 1, &wc);
		record_key_direct ((0x10 << 1) | 0);
		record_key_direct ((0x10 << 1) | 1);
	}
}

int consolehook_activate (void)
{
	return console_emulation;
}

void consolehook_ret (uaecptr condev, uaecptr oldbeginio)
{
	beginio = oldbeginio;
	write_log ("console.device at %08X\n", condev);

	uae_start_thread ("consolereader", console_thread, NULL, NULL);
}

uaecptr consolehook_beginio (uaecptr request)
{
	uae_u32 io_data = get_long (request + 40); // 0x28
	uae_u32 io_length = get_long (request + 36); // 0x24
	uae_u32 io_actual = get_long (request + 32); // 0x20
	uae_u32 io_offset = get_long (request + 44); // 0x2c
	uae_u16 cmd = get_word (request + 28);

	if (cmd == CMD_WRITE) {
		TCHAR *buf;
		const char *src = (char*)get_real_address (io_data);
		int len = io_length;
		if (io_length == -1)
			len = strlen (src);
		buf = xmalloc (TCHAR, len + 1);
		au_copy (buf, len, src);
		buf[len] = 0;
		//f_out ("%s", buf);
		xfree (buf);
	} else if (cmd == CMD_READ) {

		write_log ("%08x: CMD=%d LEN=%d OFF=%d ACT=%d\n", request, cmd, io_length, io_offset, io_actual);
	}
	return beginio;
}
