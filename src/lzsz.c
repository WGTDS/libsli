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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include "libsli.h"



#define BAD_ARGS        1
#define FILE_SIZE_ERROR 2
#define RAM_UNAVAILABLE 3
#define FILE_READ_ERROR 4
#define BAD_FILE_MAGIC  5



static void _display_error( const u32 errcode, const void *data )
{
  switch ( errcode )
  {
    case BAD_ARGS:
      printf( "??? %s\n", (char *)data );
      break;
    case FILE_SIZE_ERROR:
      printf( "FILE SIZE ERROR! %d\n", *(signed *)data );
      break;
    case RAM_UNAVAILABLE:
      printf( "RAM UNAVAILABLE!\n" );
      break;
    case FILE_READ_ERROR:
      printf( "FILE READ ERROR!\n" );
      break;
    case BAD_FILE_MAGIC:
      printf( "BAD FILE MAGIC! [0x%X]\n", *(unsigned *)data );
      break;
    default:
      printf( "\n##  Lempel-Ziv SLI Zip v1.00 [2021/11/27]  ##\n"
              "\nUsage:  lzsz [mode] [type] [infile]\n"
              "\nModes:\n  e  : Encode\n  d  : Decode\n"
              "\nTypes:\n  m  : MIO0\n  s  : SMSR00\n"
              "  y  : Yay0\n  z  : Yaz0\n\n" );
      break;
  }

  return;
}



static u32 _check_format( u32 type, u32 magic )
{
  if ( (type == MIO0) && (magic != MIO) )
  {
    return 1;
  }
  else if ( (type == SMSR00) && (magic != CMPR) )
  {
    return 1;
  }
  else if ( (type == Yay0) && (magic != Yay) )
  {
    return 1;
  }
  else if ( (type == Yaz0) && (magic != Yaz) )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}



int main( int argc, char *argv[] )
{
  FILE *infile  = (FILE *)0;
  FILE *outfile = (FILE *)0;
  u8   *src = (u8 *)0;
  u8   *dst = (u8 *)0;
  char *s;
  char  o[240];
  u32   isize;
  u32   osize;
  u32   type;
  u32   magic;
  u32   lapse;
  int   slen;
  float rat;

  if ( argc != 4 )
  {
    _display_error( 0, (void *)0 );
    exit( EXIT_FAILURE );
  }

  if (    (s = argv[1], s[1] || strpbrk(  s,     "DEde" ) == (char *)0)
       || (s = argv[2], s[1] || strpbrk(  s, "MSYZmsyz" ) == (char *)0)
       || (s = argv[3], (infile  = fopen( s, "rb" )) == (FILE *)0) )
  {
    _display_error( BAD_ARGS, (void *)s );
    exit( EXIT_FAILURE );
  }

  strcpy( o, argv[3] );

  if ( toupper( *argv[1] ) == 'D' )
  {
    slen = (int)strlen( o );

    do
    {
      if ( o[slen] == '.' )
      {
        o[slen] = '\0';
        break;
      }
      else
      {
        if ( (o[slen] == '\\') || (o[slen] == '/') )
        {
          strcat( o, ".bin" );
          break;
        }
        else
        {
          if ( slen <= 2 )
          {
            strcat( o, "_decoded_.bin" );
            break;
          }
          else
          {
            slen--;
          }
        }
      }
    }
    while ( 1 );
  }
  else
  {
    if ( toupper( *argv[2] ) == 'Z' )
    {
      strcat( o, ".szs" );
    }
    else
    {
      strcat( o, ".szp" );
    }
  }

  if ( (outfile = fopen( o, "wb" )) == (FILE *)0 )
  {
    _display_error( BAD_ARGS, (void *)o );
    exit( EXIT_FAILURE );
  }

  fseek( infile, 0L, SEEK_END );
  isize = (u32)ftell( infile );
  fseek( infile, 0L, SEEK_SET );

  if ( (isize == (u32)EOF) || (isize == 0) || (isize >= 0x3FFFFFFF) )
  {
    _display_error( FILE_SIZE_ERROR, (void *)&isize );
    goto nil;
  }

  if ( (src = (u8 *)malloc( (sizeof(u8) * isize) )) == (u8 *)0 )
  {
    _display_error( RAM_UNAVAILABLE, (void *)0 );
    goto nil;
  }

  if ( (u32)fread( src, sizeof(u8), isize, infile ) != isize )
  {
    _display_error( FILE_READ_ERROR, (void *)0 );
    goto nil;
  }

  switch ( toupper( *argv[2] ) )
  {
    case 'M':
      type = MIO0;
      break;
    case 'S':
      type = SMSR00;
      break;
    case 'Y':
      type = Yay0;
      break;
    default:
      type = Yaz0;
      break;
  }

  if ( toupper( *argv[1] ) == 'E' )
  {
    process_sli( src, &dst, isize, &osize, type, ENCODE );
  }
  else
  {
    magic = _swap32( *(u32 *)src );

    if ( _check_format( type, magic ) != 0 )
    {
      _display_error( BAD_FILE_MAGIC, (void *)&magic );
      goto nil;
    }

    osize = _swap32( *(u32 *)(src + (( type != SMSR00 ) ? 4U : 8U)) );

    if ( (dst = (u8 *)calloc( osize, sizeof(u8) )) == (u8 *)0 )
    {
      _display_error( RAM_UNAVAILABLE, (void *)0 );
      goto nil;
    }

    process_sli( src, &dst, 0, &*(u32 *)0, type, DECODE );
  }

  if ( dst == (u8 *)0 )
  {
    goto nil;
  }

  fwrite( dst, sizeof(u8), osize, outfile );
  fflush( outfile );

nil:

  fclose( outfile );
  fclose( infile );

  if ( dst != (u8 *)0 )
  {
    free( dst );
    dst = (u8 *)0;
  }
  else
  {
    remove( o );
    osize = 0;
  }

  if ( src != (u8 *)0 )
  {
    free( src );
    src = (u8 *)0;
  }

  if ( (isize != 0) && (osize != 0) )
  {
    rat  = ( isize < osize ) ? ((float)isize / (float)osize)
                             : ((float)osize / (float)isize);
    rat *= 100.0;
    printf( ">>> IN: %u , OUT: %u , RATIO: %3.2f%%\n",
            (unsigned)isize, (unsigned)osize, rat );
  }

  lapse = (clock() / CLOCKS_PER_SEC);

  if ( lapse > 3599 )
  {
    printf( ">>> %u hours , %u minutes , %u seconds elapsed.\n",
            (u32)( lapse / 3600),
            (u32)((lapse / 60) % 60),
            (u32)( lapse % 60) );
  }
  else
  {
    if ( lapse > 59 )
    {
      printf( ">>> %u minutes , %u seconds elapsed.\n",
              (u32)(lapse / 60), (u32)(lapse % 60) );
    }
    else
    {
      printf( ">>> %u seconds elapsed.\n", lapse );
    }
  }

  exit( EXIT_SUCCESS );
}
