/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2014 - Daniel De Matteis
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "d3d.hpp"
#include "d3d_wrapper.h"
#include "render_chain.hpp"

void d3d_swap(void *data, LPDIRECT3DDEVICE dev)
{
   d3d_video_t *d3d = (d3d_video_t*)data;

   if (!d3d)
	   return;

#if defined(_XBOX1)
   D3DDevice_Swap(0);
#elif defined(_XBOX360)
   D3DDevice_Present(dev);
#else
   if (dev->Present(NULL, NULL, NULL, NULL) != D3D_OK)
   {
      RARCH_ERR("[D3D]: Present() failed.\n");
      d3d->needs_restore = true;
   }
#endif
}

HRESULT d3d_create_vertex_buffer(LPDIRECT3DDEVICE dev,
      unsigned length, unsigned usage, unsigned fvf,
      D3DPOOL pool, LPDIRECT3DVERTEXBUFFER vert_buf, void *handle)
{
#if defined(_XBOX1)
   return IDirect3DDevice8_CreateVertexBuffer(dev, length, usage, fvf, pool,
         &vert_buf);
#elif defined(_XBOX360)
   return IDirect3DDevice9_CreateVertexBuffer(dev, length, usage, fvf, pool,
         &vert_buf, NULL);
#else
   return dev->CreateVertexBuffer(length, usage, fvf, pool, &vert_buf, NULL);
#endif
}

void d3d_set_stream_source(LPDIRECT3DDEVICE dev, unsigned stream_no,
      LPDIRECT3DVERTEXBUFFER stream_vertbuf, unsigned offset_bytes,
      unsigned stride)
{
#if defined(_XBOX1)
   IDirect3DDevice8_SetStreamSource(dev, stream_no, stream_vertbuf, stride);
#elif defined(_XBOX360)
   D3DDevice_SetStreamSource_Inline(dev, stream_no, stream_vertbuf,
         offset_bytes, stride);
#else
   dev->SetStreamSource(stream_no, stream_vertbuf, offset_bytes, stride);
#endif
}

void d3d_set_sampler_address_u(LPDIRECT3DDEVICE dev,
      unsigned sampler, unsigned value)
{
#if defined(_XBOX1)
   D3D__DirtyFlags |= (D3DDIRTYFLAG_TEXTURE_STATE_0 << sampler);
   D3D__TextureState[sampler][D3DTSS_ADDRESSU] = value;
#elif defined(_XBOX360)
   D3DDevice_SetSamplerState_AddressU_Inline(dev, sampler, value);
#else
   dev->SetSamplerState(sampler, D3DSAMP_ADDRESSU, value);
#endif
}

void d3d_set_sampler_address_v(LPDIRECT3DDEVICE dev,
      unsigned sampler, unsigned value)
{
#if defined(_XBOX1)
   D3D__DirtyFlags |= (D3DDIRTYFLAG_TEXTURE_STATE_0 << sampler);
   D3D__TextureState[sampler][D3DTSS_ADDRESSV] = value;
#elif defined(_XBOX360)
   D3DDevice_SetSamplerState_AddressV_Inline(dev, sampler, value);
#else
   dev->SetSamplerState(sampler, D3DSAMP_ADDRESSV, value);
#endif
}

void d3d_set_sampler_minfilter(LPDIRECT3DDEVICE dev,
      unsigned sampler, unsigned value)
{
#if defined(_XBOX1)
   D3D__DirtyFlags |= (D3DDIRTYFLAG_TEXTURE_STATE_0 << sampler);
   D3D__TextureState[sampler][D3DTSS_MINFILTER] = value;
#elif defined(_XBOX360)
   D3DDevice_SetSamplerState_MinFilter(dev, sampler, value);
#else
   dev->SetSamplerState(sampler, D3DSAMP_MINFILTER, value);
#endif
}

void d3d_set_sampler_magfilter(LPDIRECT3DDEVICE dev,
      unsigned sampler, unsigned value)
{
#if defined(_XBOX1)
   D3D__DirtyFlags |= (D3DDIRTYFLAG_TEXTURE_STATE_0 << sampler);
   D3D__TextureState[sampler][D3DTSS_MAGFILTER] = value;
#elif defined(_XBOX360)
   D3DDevice_SetSamplerState_MagFilter(dev, sampler, value);
#else
   dev->SetSamplerState(sampler, D3DSAMP_MAGFILTER, value);
#endif
}

void d3d_draw_primitive(LPDIRECT3DDEVICE dev,
      D3DPRIMITIVETYPE type, unsigned start, unsigned count)
{
#if defined(_XBOX1)
   D3DDevice_DrawVertices(type, start, D3DVERTEXCOUNT(type, count));
#elif defined(_XBOX360)
   D3DDevice_DrawVertices(dev, type, start, D3DVERTEXCOUNT(type, count));
#else
   if (SUCCEEDED(dev->BeginScene()))
   {
      dev->DrawPrimitive(type, start, count);
      dev->EndScene();
   }
#endif
}

void d3d_lockrectangle_clear(LPDIRECT3DTEXTURE tex,
      unsigned tex_width, unsigned tex_height,
      unsigned level, D3DLOCKED_RECT lock_rect, RECT rect,
      unsigned flags)
{
#if defined(_XBOX)
   D3DTexture_LockRect(tex, level, &lock_rect, &rect, flags);
   memset(lock_rect.pBits, 0, tex_height * lock_rect.Pitch);
#else
   if (SUCCEEDED(tex->LockRect(level, &lock_rect, &rect, flags)))
   {
      memset(lock_rect.pBits, level, tex_height * lock_rect.Pitch);
      tex->UnlockRect(0);
   }
#endif
}

void d3d_textureblit(void *data, void *renderchain_data,
      LPDIRECT3DTEXTURE tex, D3DSURFACE_DESC desc,
      D3DLOCKED_RECT lr, const void *frame,
      unsigned width, unsigned height, unsigned pitch)
{
	d3d_video_t *d3d = (d3d_video_t*)d3d;
   (void)desc;
   (void)d3d;

   if (!d3d)
	   return;

#if defined(_XBOX360)
   tex->GetLevelDesc(0, &desc);
   XGCopySurface(lr.pBits, lr.Pitch, width, height, desc.Format, NULL,
      frame, pitch, desc.Format, NULL, 0, 0);
#elif defined(_XBOX1)
   for (unsigned y = 0; y < height; y++)
   {
      const uint8_t *in = (const uint8_t*)frame + y * pitch;
      uint8_t *out = (uint8_t*)lr.pBits + y * lr.Pitch;
      memcpy(out, in, width * d3d->pixel_size);
   }
#else
   renderchain_t *chain = (renderchain_t*)renderchain_data;

   if (!chain)
	   return;

   if (SUCCEEDED(tex->LockRect(0, &lr, NULL, D3DLOCK_NOSYSLOCK)))
   {
      for (unsigned y = 0; y < height; y++)
      { 
         const uint8_t *in = (const uint8_t*)frame + y * pitch;
         uint8_t *out = (uint8_t*)lr.pBits + y * lr.Pitch;
         memcpy(out, in, width * chain->pixel_size);
      }
      tex->UnlockRect(0);
   }
#endif
}