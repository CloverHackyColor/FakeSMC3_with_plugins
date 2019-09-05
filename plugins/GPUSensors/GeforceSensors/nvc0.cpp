//
//  nvc0.c
//  HWSensors
//
//  Created by Kozlek on 07.08.12.
//
//

/*
 * Copyright 2012 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs
 */

#include "nvc0.h"

#include "nouveau.h"
#include "nv50.h"
#include "nv84.h"
#include "nva3.h"
#include "nvd0.h"
#include "nouveau_therm.h"

bool nvc0_identify(struct nouveau_device *device)
{
	switch (device->chipset) {
    case 0xc0:
      device->cname = "GF100";
      break;
    case 0xc4:
      device->cname = "GF104";
      break;
    case 0xc3:
      device->cname = "GF106";
      break;
    case 0xce:
      device->cname = "GF114";
      break;
    case 0xcf:
      device->cname = "GF116";
      break;
    case 0xc1:
      device->cname = "GF108";
      break;
    case 0xc8:
      device->cname = "GF110";
      break;
    case 0xd9:
      device->cname = "GF119";
      break;
    default:
      nv_fatal(device, "unknown Fermi chipset\n");
      return false;
  }
  
	return true;
}

void nvc0_init(struct nouveau_device *device)
{
  switch (device->chipset) {
      
    case 0xd9:
      device->gpio_sense = nvd0_gpio_sense;
      device->fan_rpm_get = nouveau_therm_fan_rpm_get;
      break;
    default:
      nva3_therm_init(device);
      device->gpio_sense = nv50_gpio_sense;
      device->fan_rpm_get = nva3_therm_fan_sense;
      break;
  }
  
  device->gpio_find = nouveau_gpio_find;
  device->gpio_get = nouveau_gpio_get;
  
  device->temp_get = nv84_temp_get;
  device->clocks_get = nvc0_clocks_get;
  //device->voltage_get = nouveau_voltage_get;
  device->pwm_get = nvd0_fan_pwm_get;
  device->fan_pwm_get = nouveau_therm_fan_pwm_get;
}

static u32 read_div(struct nouveau_device *, int, u32, u32);
static u32 read_pll(struct nouveau_device *, u32);

static u32 read_vco(struct nouveau_device *device, u32 dsrc)
{
	u32 ssrc = nv_rd32(device, dsrc);  //dsrc = 0x137160
  nv_debug(device, "read_vco ssrc=0x%x\n", ssrc); //read_vco ssrc=0x3
	if (!(ssrc & 0x00000100))
		return read_pll(device, 0x00e800);
	return read_pll(device, 0x00e820);
}

#if 0
int
nvkm_clk_read(struct nvkm_clk *clk, enum nv_clk_src src)
{
  return clk->func->read(clk, src);
}
struct nvkm_clk {
  const struct nvkm_clk_func *func;
//...
};
static const struct nvkm_clk_func
gf100_clk = {
  .read = gf100_clk_read,
  .calc = gf100_clk_calc,
  .prog = gf100_clk_prog,
  .tidy = gf100_clk_tidy,
  .domains = {
    { nv_clk_src_crystal, 0xff },
    { nv_clk_src_href   , 0xff },
    { nv_clk_src_hubk06 , 0x00 },
    { nv_clk_src_hubk01 , 0x01 },
    { nv_clk_src_copy   , 0x02 },
    { nv_clk_src_gpc    , 0x03, NVKM_CLK_DOM_FLAG_VPSTATE, "core", 2000 },
    { nv_clk_src_rop    , 0x04 },
    { nv_clk_src_mem    , 0x05, 0, "memory", 1000 },
    { nv_clk_src_vdec   , 0x06 },
    { nv_clk_src_pmu    , 0x0a },
    { nv_clk_src_hubk07 , 0x0b },
    { nv_clk_src_max }
  }
};
#endif


static u32 read_pll(struct nouveau_device *device, u32 pll)
{
	u32 ctrl = nv_rd32(device, pll + 0);
	u32 coef = nv_rd32(device, pll + 4);
	u32 P = (coef & 0x003f0000) >> 16;
	u32 N = (coef & 0x0000ff00) >> 8;
	u32 M = (coef & 0x000000ff) >> 0;
	u32 sclk, doff;
  u16 fN = 0xf000;
  
  nv_debug(device, "read pll=0x%x, ctrl=0x%x, coef=0x%x\n", pll, ctrl, coef);
  //read pll=0xe820, ctrl=0x1030005, coef=0x5063c01 => P=6 N=60 M=1
  //=>sclk=0x18b820 =
  //read pll=0xe800, ctrl=0x1030005, coef=0x5063c01
	if (!(ctrl & 0x00000001))
		return 0;

  if (P == 0) P = 1;
  if (M == 0) M = 1;

	switch (pll) {
    case 0x00e820:
      sclk = device->crystal;
      P = 1;
      break;
    case 0x00e800:
      sclk = device->crystal;
      P = 3;
      break;
    case 0x137000:
    case 0x137020:
    case 0x137040:
    case 0x1370e0:
      doff = (pll - 0x137000) / 0x20;
      sclk = read_div(device, doff, 0x137120, 0x137140);
      break;
    case 0x132000:
      sclk = read_pll(device, 0x132020);
      P = (coef & 0x10000000) ? 2 : 1;
      break;
    case 0x132020:
      sclk = read_div(device, 0, 0x137320, 0x137330);
      fN   = nv_rd32(device, pll + 0x10) >> 16;
      break;
    default:
      return 0;
	}

  sclk = (sclk * N) + (((u16)(fN + 4096) * sclk) >> 13);
	return sclk / (M * P);
}

static u32 read_div(struct nouveau_device *device, int doff, u32 dsrc, u32 dctl)
{
	u32 ssrc = nv_rd32(device, dsrc + (doff * 4));
  u32 sclk, sctl, sdiv = 2;
  u32 ret = 0;

  //read_div(device, 0, 0x137160, 0x1371d0); //ssrc=0x3
	switch (ssrc & 0x00000003) {
    case 0:
      if ((ssrc & 0x00030000) != 0x00030000) {
        ret = device->crystal;
        break;
      }
      ret = 108000;
      break;
    case 2:
      ret = 100000;
      break;
    case 3:
      sclk = read_vco(device, dsrc + (doff * 4)); //read_vco ssrc=0x103
      //read pll=0xe820, ctrl=0x1030005, coef=0x5063c01
      //return sclk=0x41eb0 = 270000
      //return read_div=0xd2f0
      //
      //read pll=0xe800, ctrl=0x1030005, coef=0x5063c01
      //return sclk=0x41eb0
      //return read_div=0x107ac
      /* Memclk has doff of 0 despite its alt. location */
      if (doff <= 2) {
        sctl = nv_rd32(device, dctl + (doff * 4));
        nv_debug(device, "sctl=0x%x\n", sctl); //sctl=0x81200606
        if (sctl & 0x80000000) {
          if (ssrc & 0x100)
            sctl >>= 8;

          sdiv = (sctl & 0x3f) + 2;
        }
        nv_debug(device, "sctl=0x%x sdiv=0x%x\n", sctl, sdiv); //sctl=0x81200606 sdiv=0x8
      }

      ret = (sclk * 2) / sdiv;
      break;
    default:
      break;
	}
  nv_debug(device, "return read_div=0x%x\n", ret); //return read_div=0x107ac = 67500
  return ret;
}

static u32 read_mem(struct nouveau_device *device)
{
  u32 base = 0;
	u32 ssel = nv_rd32(device, 0x1373f0);
  if (ssel & 0x00000002)
    base = read_pll(device, 0x132000);
  else
    base = read_div(device, 0, 0x137300, 0x137310); //return read_div=0xd2f0
  nv_debug(device, "return read_mem=%d\n", base); //return read_mem=54000
  return base / 4.0f;
}

static u32 read_clk(struct nouveau_device *device, int clk)
{
  u32 sctl = nv_rd32(device, 0x137250 + (clk * 4));
	u32 ssel = nv_rd32(device, 0x137100);
	u32 sclk, sdiv;
  
  nv_debug(device, "read_clk sctl=0x%x ssel=0x%x\n", sctl, ssel); //read_clk sctl=0x81200000 ssel=0x0
  
	if (ssel & (1 << clk)) {
		if (clk < 7)
			sclk = read_pll(device, 0x137000 + (clk * 0x20));
		else
			sclk = read_pll(device, 0x1370e0);
		sdiv = ((sctl & 0x00003f00) >> 8) + 2;
    //read pll=0xe800, ctrl=0x1030005, coef=0x5063c01
	} else {
		sclk = read_div(device, clk, 0x137160, 0x1371d0); //return read_div=0x107ac = 67500
		sdiv = ((sctl & 0x0000003f) >> 0) + 2;
	}
  nv_debug(device, "return read_clk=%d div=%d\n", sclk, sdiv/2); //return read_clk=67500 div=1
  
	if (sctl & 0x80000000)
		return (sclk * 2) / sdiv;
	return sclk;
}
//gf100_clk_read(struct nvkm_clk *base, enum nv_clk_src src)
int nvc0_clocks_get(struct nouveau_device *device, u8 source)
{
  switch (source) {
    case nouveau_clock_core:
      return read_clk(device, 0x00) / 2;
    case nouveau_clock_shader:
      return read_clk(device, 0x00);
    case nouveau_clock_memory:
      return read_mem(device);
    case nouveau_clock_rop:
      return read_clk(device, 0x01);
    case nouveau_clock_hub01:
      return read_clk(device, 0x08);
    case nouveau_clock_hub06:
      return read_clk(device, 0x07);
    case nouveau_clock_hub07:
      return read_clk(device, 0x02);
    case nouveau_clock_vdec:
      return read_clk(device, 0x0e);
    case nouveau_clock_daemon:
      return read_clk(device, 0x0c);
    case nouveau_clock_copy:
      return read_clk(device, 0x09);

    case nouveau_clock_mpllref:
      return read_div(device, 0, 0x137320, 0x137330);
    case nouveau_clock_mpllsrc:
      return read_pll(device, 0x132020);
    case nouveau_clock_mpll:
      return read_pll(device, 0x132000);

    default:
      return 0;
  }
}
