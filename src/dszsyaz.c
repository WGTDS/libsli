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

    This code segment has been transcribed into C from the Assembler
    source, "slidec.s", and modified/adapted to better suit the
    implementation of this library.

    "slidec.s" is Assembly code for decoding version 1.10 of the
    SLI format, "Yay0", and is a part of the Nintendo 64 SDK.

    Additionally, the SLI format is to be regarded as the intellectual
    property of << Nintendo EAD >> and << "Melody-Yoshi" >>.
---------------------------------------------------------------------------*/



#include "libsli.h"
#include <stdio.h>
#include <stdlib.h>



static u8 *_process_yaz0( u8 *src, u8 **dst )
{
  u32 word;
  u32 declen;
  u32 dsp;
  u32 qty;
  u32 flags;
  u8 *dest;
  u8 *prev;
  i32 sign;

  word = _swap32( *(u32 *)src );

  if ( word != (u32)Yaz )
  {
    goto err;
  }

  declen = _swap32( *(u32 *)(src + 0x4U) );

  if ( declen == 0U )
  {
    goto err;
  }

  dest = (*dst + declen);

  /*---------------------------------------------------------------------
    TODO:
    Consider addressing the usage of the reserved words @ 0x8 & 0xC.
    There have been claims that Yaz0 headers in newer binaries released
    by Nintendo contain alignment information within a reserved word.
    The N64 and GCN do not use these words, however.
  ---------------------------------------------------------------------*/

  flags = 0;
  sign  = 0;
  src  += 0x10U;

  do
  {
    if ( flags == 0 )
    {
      sign   = (i32)*src;
      sign <<= 24;
      flags  = 8U;
      src++;
    }
    else
    {
      if ( sign < 0 )
      {
        *(*dst)++ = *src++;
      }
      else
      {
        dsp  = (u32)_swap16( *(u16 *)src );
        src += 2U;
        qty  = ((dsp >> 12) ? ((dsp >> 12) + 2U) : ((u32)*src++ + 18U));
        prev = *dst - ((dsp & 0xFFF) + 1U);

        while ( *(*dst)++ = *prev++, --qty );
      }

      sign <<= 1;
      flags--;
    }
  }
  while ( *dst < dest );

  return *dst -= declen;

err:

  free( *dst );
  printf( "Bad Header!\n"
          "0x%8X | 0x%8X | 0x%8X | 0x%8X\n",
          _swap32( *(u32 *)(src        ) ),
          _swap32( *(u32 *)(src + 0x04U) ),
          _swap32( *(u32 *)(src + 0x08U) ),
          _swap32( *(u32 *)(src + 0x0CU) ) );
  return *dst = (u8 *)0;
}



u8 *_decode_yaz0( u8 *src, u8 **dst )
{
  return *dst = _process_yaz0( src, &*dst );
}
