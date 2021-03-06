/****************************************************************************
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <https://unlicense.org>
 ***************************************************************************/
/*---------------------------------------------------------------------------
    Lib SLI v1.00

    Author  : White Guy That Don't Smile
    Date    : 2021/11/27, Saturday, November 27th; 1140 HOURS
    License : UnLicense | Public Domain

    This is a C library for processing Nintendo's SLI data.
    It is compliant with the "MIO0", "CMPR"/"SMSR00", "Yay0",
    and "Yaz0" formats generated by Nintendo for the Nintendo 64.
    Second and third party developers may have made alterations
    to the SLI algorithm which aren't replicated by this software.
---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
    DISCLAIMER

    This code segment has been both procured and cross-referenced
    through the use of decompilers/disassemblers, and modified/adapted
    to better suit the implementation of this library.

    The sampled executable binary, "sliencw11.exe", was reverse-software
    engineered using "Ghidra" and the x86 decompiler, "Snowman".

    "sliencw11.exe" is an encoder for version 1.10 of the SLI format,
    "Yay0", and is a part of the Nintendo 64 SDK.

    Additionally, the SLI format is to be regarded as the intellectual
    property of << Nintendo EAD >> and << "Melody-Yoshi" >>.
---------------------------------------------------------------------------*/



#include "libsli.h"
#include <stdio.h>
#include <stdlib.h>



static void _null_yaz0_block( u8 *def )
{
  if ( def != (u8 *)0 )
  {
    free( def );
    def = (u8 *)0;
  }

  return;
}



static u8 *_produce_yaz0( u8 *src, u8 **dst,
                          const u32 srclen, u32 **dstlen )
{
  u8  *def;
  u32  dp;
  u32  cp;
  u32  match_length;
  i32  lookahead_position;
  u32  lookahead_length;
  u32  mask;
  i32  match_position;
  register u32 srcpos;
  u32  ndp;
  u32  i;
  u8   cmds;
  u8   bytes[3];

  cp  = 0;
  dp  = cp + 1U;
  ndp = 0x1000U << ENC_GREED;
  def = malloc( (ndp << 2U) );
  srcpos = 0;
  cmds   = 0;
  mask   = 0x80U;

  while ( srcpos < srclen )
  {
    _search( src, srcpos, srclen,
             &match_position, &match_length,
             0x111U );

    if ( match_length < 3U )
    {
      cmds |= mask;
      def[dp++] = src[srcpos++];

      if ( ndp == dp )
      {
        ndp += ((0x1000U << 2) << ENC_GREED);
        def  = realloc( def, ndp );

        if ( def == (u8 *)0 )
        {
          goto err;
        }
      }
    }
    else
    {
      _search( src, (srcpos + 1U), srclen,
               &lookahead_position, &lookahead_length,
               0x111U );

      if ( (match_length + 1U) < lookahead_length )
      {
        cmds |= mask;
        def[dp++] = src[srcpos++];

        if ( ndp == dp )
        {
          ndp += ((0x1000U << 2) << ENC_GREED);
          def  = realloc( def, ndp );

          if ( def == (u8 *)0 )
          {
            goto err;
          }
        }

        mask >>= 1;

        if ( mask == 0 )
        {
          mask = 0x80U;
          def[cp] = cmds;
          cp = dp++;

          if ( ndp == dp )
          {
            ndp += ((0x1000U << 2) << ENC_GREED);
            def  = realloc( def, ndp );

            if ( def == (u8 *)0 )
            {
              goto err;
            }
          }

          cmds = 0;
        }

        match_length   = lookahead_length;
        match_position = lookahead_position;
      }

      match_position = ((srcpos - match_position) + -1);
      i = 0;

      if ( match_length < 0x12U )
      {
        bytes[0] = ((((i8)match_length + -2) * 0x10) | (u8)(match_position >> 8));
        bytes[1] = (i8)match_position;

        while ( i < 2U )
        {
          def[dp++] = bytes[i++];

          if ( ndp == dp )
          {
            ndp += ((0x1000U << 2) << ENC_GREED);
            def  = realloc( def, ndp );

            if ( def == (u8 *)0 )
            {
              goto err;
            }
          }
        }
      }
      else
      {
        bytes[0] = (i8)(match_position >> 8);
        bytes[1] = (i8)match_position;
        bytes[2] = (i8)(match_length + -0x12);

        while ( i < 3U )
        {
          def[dp++] = bytes[i++];

          if ( ndp == dp )
          {
            ndp += ((0x1000U << 2) << ENC_GREED);
            def  = realloc( def, ndp );

            if ( def == (u8 *)0 )
            {
              goto err;
            }
          }
        }
      }

      srcpos += match_length;
    }

    mask >>= 1;

    if ( mask == 0 )
    {
      mask = 0x80U;
      def[cp] = cmds;
      cp = dp++;

      if ( ndp == dp )
      {
        ndp += ((0x1000U << 2) << ENC_GREED);
        def  = realloc( def, ndp );

        if ( def == (u8 *)0 )
        {
          goto err;
        }
      }

      cmds = 0;
    }
  }

  if ( mask != 0x80U )
  {
    def[cp] = cmds;
  }
  else
  {
    --dp;
  }

  **dstlen = (u32)(dp + 0x10U);
  *dst     = (u8 *)calloc(**dstlen, sizeof(u8));

  if ( *dst == (u8 *)0 )
  {
    goto err;
  }

  *((u32 *)(*dst + 0x00U)) = _swap32( (u32)Yaz );
  *((u32 *)(*dst + 0x04U)) = _swap32( srclen );
  *((u32 *)(*dst + 0x08U)) = _swap32( (u32)0 );
  *((u32 *)(*dst + 0x0CU)) = _swap32( (u32)0 );

  for ( srcpos = 0; srcpos < dp; srcpos++ )
  {
    *(*dst + 0x10U + srcpos) = def[srcpos];
  }

  _null_yaz0_block( def );
  return *dst;

err:

  printf( "RAM Unavailable!\n" );

  if ( *dst != (u8 *)0 )
  {
    free( *dst );
  }

  _null_yaz0_block( def );
  return *dst = (u8 *)0;
}



u8 *_encode_yaz0( u8 *src, u8 **dst,
                  const u32 srclen, u32 *dstlen )
{
  return *dst = _produce_yaz0( src, &*dst, srclen, &dstlen );
}
