/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/devices/video/mda.c                                      *
 * Created:     2003-04-13 by Hampa Hug <hampa@hampa.ch>                     *
 * Copyright:   (C) 2003-2008 Hampa Hug <hampa@hampa.ch>                     *
 *****************************************************************************/

/*****************************************************************************
 * This program is free software. You can redistribute it and / or modify it *
 * under the terms of the GNU General Public License version 2 as  published *
 * by the Free Software Foundation.                                          *
 *                                                                           *
 * This program is distributed in the hope  that  it  will  be  useful,  but *
 * WITHOUT  ANY   WARRANTY,   without   even   the   implied   warranty   of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  General *
 * Public License for more details.                                          *
 *****************************************************************************/

/* $Id$ */


#include <stdio.h>

#include <lib/log.h>
#include <lib/hexdump.h>

#include "mda.h"


static unsigned long mda_col[17] = {
	0x000000, 0x000000, 0xe89050, 0xe89050,
	0xe89050, 0xe89050, 0xe89050, 0xe89050,
	0xfff0c8, 0xfff0c8, 0xfff0c8, 0xfff0c8,
	0xfff0c8, 0xfff0c8, 0xfff0c8, 0xfff0c8,
	0xfff0c8
};


static
void mda_get_colors (mda_t *mda, ini_sct_t *sct)
{
	unsigned i;
	char     str[16];

	for (i = 0; i < 17; i++) {
		sprintf (str, "color%u", i);
		ini_get_uint32 (sct, str, &mda->rgb[i], mda_col[i]);
	}
}

static
void mda_set_colors (mda_t *mda)
{
	unsigned i;
	unsigned r, g, b;

	for (i = 0; i < 16; i++) {
		r = (mda->rgb[i] >> 16) & 0xff;
		g = (mda->rgb[i] >> 8) & 0xff;
		b = mda->rgb[i] & 0xff;
		trm_old_set_map (&mda->trm, i, r | (r << 8), g | (g << 8), b | (b << 8));
	}
}

video_t *mda_new (terminal_t *trm, ini_sct_t *sct)
{
	unsigned      i;
	unsigned long iobase, membase, memsize;
	unsigned      w, h;
	mda_t         *mda;

	mda = (mda_t *) malloc (sizeof (mda_t));
	if (mda == NULL) {
		return (NULL);
	}

	pce_video_init (&mda->vid);

	mda->vid.ext = mda;
	mda->vid.del = (void *) mda_del;
	mda->vid.get_mem = (void *) mda_get_mem;
	mda->vid.get_reg = (void *) mda_get_reg;
	mda->vid.print_info = (void *) mda_prt_state;
	mda->vid.screenshot = (void *) mda_screenshot;
	mda->vid.clock = (void *) mda_clock;

	for (i = 0; i < 18; i++) {
		mda->crtc_reg[i] = 0;
	}

	ini_get_uint16 (sct, "w", &w, 640);
	ini_get_uint16 (sct, "h", &h, 400);

	ini_get_uint16 (sct, "mode_80x25_w", &mda->mode_80x25_w, w);
	ini_get_uint16 (sct, "mode_80x25_h", &mda->mode_80x25_h, h);

	ini_get_uint32 (sct, "io", &iobase, 0x03b4);
	ini_get_uint32 (sct, "membase", &membase, 0x000b0000);
	ini_get_uint32 (sct, "memsize", &memsize, 4096);

	if (memsize < 4096) {
		memsize = 4096;
	}

	mda_get_colors (mda, sct);

	pce_log_tag (MSG_INF,
		"VIDEO:", "MDA addr=0x%04x membase=0x%05x memsize=0x%05x\n",
		iobase, membase, memsize
	);

	mda->crtc_pos = 0;

	mda->mem = mem_blk_new (membase, memsize, 1);
	mda->mem->ext = mda;
	mda->mem->set_uint8 = (mem_set_uint8_f) &mda_mem_set_uint8;
	mda->mem->set_uint16 = (mem_set_uint16_f) &mda_mem_set_uint16;
	mem_blk_clear (mda->mem, 0x00);

	mda->reg = mem_blk_new (iobase, 16, 1);
	mda->reg->ext = mda;
	mda->reg->set_uint8 = (mem_set_uint8_f) &mda_reg_set_uint8;
	mda->reg->set_uint16 = (mem_set_uint16_f) &mda_reg_set_uint16;
	mda->reg->get_uint8 = (mem_get_uint8_f) &mda_reg_get_uint8;
	mda->reg->get_uint16 = (mem_get_uint16_f) &mda_reg_get_uint16;
	mem_blk_clear (mda->reg, 0x00);

	mda->trmnew = trm;
	mda->trmclk = 0;
	trm_old_init (&mda->trm, mda->trmnew);

	mda_set_colors (mda);

	trm_old_set_mode (&mda->trm, TERM_MODE_TEXT, 80, 25);
	trm_old_set_size (&mda->trm, mda->mode_80x25_w, mda->mode_80x25_h);

	return (&mda->vid);
}

void mda_del (mda_t *mda)
{
	if (mda != NULL) {
		mem_blk_del (mda->mem);
		mem_blk_del (mda->reg);
		free (mda);
	}
}

void mda_clock (mda_t *mda, unsigned long cnt)
{
	mda->trmclk += cnt;

	if (mda->trmclk > 16384) {
		mda->trmclk = 0;
		trm_old_update (&mda->trm);
	}
}

void mda_prt_state (mda_t *mda, FILE *fp)
{
	unsigned i;
	unsigned x, y;

	x = mda->crtc_pos % 80;
	y = mda->crtc_pos / 80;

	fprintf (fp, "MDA: 3B4=%02X  3B5=%02X  3B8=%02X  3BA=%02X  POS=%04X[%u/%u]\n",
		mda->reg->data[0], mda->reg->data[1], mda->reg->data[4], mda->reg->data[6],
		mda->crtc_pos, x, y
	);

	fprintf (fp, "CRTC=[%02X", mda->crtc_reg[0]);
	for (i = 1; i < 18; i++) {
		if ((i & 7) == 0) {
			fputs ("-", fp);
		}
		else {
			fputs (" ", fp);
		}
		fprintf (fp, "%02X", mda->crtc_reg[i]);
	}
	fputs ("]\n", fp);

	fflush (fp);
}

int mda_dump (mda_t *mda, FILE *fp)
{
	fprintf (fp, "# MDA dump\n");

	fprintf (fp, "\n# REGS:\n");
	pce_dump_hex (fp,
		mem_blk_get_data (mda->reg),
		mem_blk_get_size (mda->reg),
		mem_blk_get_addr (mda->reg),
		16, "# ", 0
	);

	fprintf (fp, "\n# CRTC:\n");
	pce_dump_hex (fp, mda->crtc_reg, 18, 0, 16, "# ", 0);

	fputs ("\n\n# RAM:\n", fp);
	pce_dump_hex (fp,
		mem_blk_get_data (mda->mem),
		mem_blk_get_size (mda->mem),
		mem_blk_get_addr (mda->mem),
		16, "", 1
	);

	return (0);
}

mem_blk_t *mda_get_mem (mda_t *mda)
{
	return (mda->mem);
}

mem_blk_t *mda_get_reg (mda_t *mda)
{
	return (mda->reg);
}

int mda_screenshot (mda_t *mda, FILE *fp, unsigned mode)
{
	unsigned i;
	unsigned x, y;

	if ((mode != 0) && (mode != 1)) {
		return (1);
	}

	i = 0;

	for (y = 0; y < 25; y++) {
		for (x = 0; x < 80; x++) {
			fputc (mda->mem->data[i], fp);
			i += 2;
		}

		fputs ("\n", fp);
	}

	return (0);
}

void mda_update (mda_t *mda)
{
	unsigned i;
	unsigned x, y;
	unsigned fg, bg;

	i = 0;

	for (y = 0; y < 25; y++) {
		for (x = 0; x < 80; x++) {
			fg = mda->mem->data[i + 1] & 0x0f;
			bg = (mda->mem->data[i + 1] & 0xf0) >> 4;

			trm_old_set_col (&mda->trm, fg, bg);
			trm_old_set_chr (&mda->trm, x, y, mda->mem->data[i]);

			i = (i + 2) & 0x3fff;
		}
	}
}

static
void mda_set_pos (mda_t *mda, unsigned pos)
{
	unsigned x, y;

	if (mda->crtc_pos == pos) {
		return;
	}

	mda->crtc_pos = pos;

	x = pos % 80;
	y = pos / 80;

	trm_old_set_pos (&mda->trm, pos % 80, pos / 80);
}

static
void mda_set_crs (mda_t *mda, unsigned y1, unsigned y2)
{
	if (y1 > 13) {
		trm_old_set_crs (&mda->trm, 0, 0, 0);
		return;
	}

	if ((y2 < y1) || (y2 > 13)) {
		y2 = 13;
	}

	y1 = (255 * y1 + 6) / 13;
	y2 = (255 * y2 + 6) / 13;

	trm_old_set_crs (&mda->trm, y1, y2, 1);
}

void mda_mem_set_uint8 (mda_t *mda, unsigned long addr, unsigned char val)
{
	unsigned      x, y;
	unsigned char c, a;

	if (mda->mem->data[addr] == val) {
		return;
	}

	mda->mem->data[addr] = val;

	if (addr & 1) {
		c = mda->mem->data[addr - 1];
		a = val;
	}
	else {
		c = val;
		a = mda->mem->data[addr + 1];
	}

	if (addr >= 4000) {
		return;
	}

	x = (addr >> 1) % 80;
	y = (addr >> 1) / 80;

	trm_old_set_col (&mda->trm, a & 0x0f, (a >> 4) & 0x0f);
	trm_old_set_chr (&mda->trm, x, y, c);
}

void mda_mem_set_uint16 (mda_t *mda, unsigned long addr, unsigned short val)
{
	unsigned      x, y;
	unsigned char c, a;

	if (addr & 1) {
		mda_mem_set_uint8 (mda, addr, val & 0xff);
		mda_mem_set_uint8 (mda, addr + 1, (val >> 8) & 0xff);
		return;
	}

	c = val & 0xff;
	a = (val >> 8) & 0xff;

	if ((mda->mem->data[addr] == c) && (mda->mem->data[addr] == a)) {
		return;
	}

	mda->mem->data[addr] = c;
	mda->mem->data[addr + 1] = a;

	if (addr >= 4000) {
		return;
	}

	x = (addr >> 1) % 80;
	y = (addr >> 1) / 80;

	trm_old_set_col (&mda->trm, a & 0x0f, (a >> 4) & 0x0f);
	trm_old_set_chr (&mda->trm, x, y, c);
}

static
void mda_crtc_set_reg (mda_t *mda, unsigned reg, unsigned char val)
{
	if (reg > 15) {
		return;
	}

	mda->crtc_reg[reg] = val;

	switch (reg) {
		case 0x0a:
		case 0x0b:
			mda_set_crs (mda, mda->crtc_reg[0x0a], mda->crtc_reg[0x0b]);
			break;

		case 0x0e:
			mda_set_pos (mda, (val << 8) | (mda->crtc_reg[0x0f] & 0xff));
			break;

		case 0x0f:
			mda_set_pos (mda, (mda->crtc_reg[0x0e] << 8) | val);
			break;
	}
}

static
unsigned char mda_crtc_get_reg (mda_t *mda, unsigned reg)
{
	if (reg > 15) {
		return (0xff);
	}

	return (mda->crtc_reg[reg]);
}

void mda_reg_set_uint8 (mda_t *mda, unsigned long addr, unsigned char val)
{
	mda->reg->data[addr] = val;

	switch (addr) {
		case 0x01:
			mda_crtc_set_reg (mda, mda->reg->data[0], val);
			break;
	}
}

void mda_reg_set_uint16 (mda_t *mda, unsigned long addr, unsigned short val)
{
	mda_reg_set_uint8 (mda, addr, val & 0xff);
	mda_reg_set_uint8 (mda, addr + 1, val >> 8);
}

unsigned char mda_reg_get_uint8 (mda_t *mda, unsigned long addr)
{
	static unsigned cnt = 0;

	switch (addr) {
		case 0x00:
			return (mda->reg->data[0]);

		case 0x01:
			return (mda_crtc_get_reg (mda, mda->reg->data[0]));

		case 0x06:
			cnt += 1;
			if ((cnt & 7) == 0) {
				cnt = 0;
				mda->reg->data[6] ^= 0x01;
			}
			mda->reg->data[6] ^= (cnt & 1) ? 0x08 : 0x00;
			return (mda->reg->data[6]);

		default:
			return (0xff);
	}
}

unsigned short mda_reg_get_uint16 (mda_t *mda, unsigned long addr)
{
	unsigned short ret;

	ret = mda_reg_get_uint8 (mda, addr);
	ret |= mda_reg_get_uint8 (mda, addr + 1) << 8;

	return (ret);
}
