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
    Lib SLI v1.02

    Author  : White Guy That Don't Smile
    Date    : 2023/07/31, Monday, July 31st; 1222 HOURS
    License : UnLicense | Public Domain

    This is a C library for processing Nintendo's SLI data.

    Formats Supported:
        (Official)
            MIO0 "Mario"
            SMSR00 "Mario 2"
            Yay0 "Zelda"
            Yaz0 "Zelda 2"
        (Unofficial*)
            Rvl0 "Revolution"

    * This extension was observed and borrowed from the Revolution SDK Tool
    "ntcompress", but uses the more efficient SLI algorithm instead.

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>



typedef  int8_t  i8;
typedef uint8_t  u8;
typedef  int16_t i16;
typedef uint16_t u16;
typedef  int32_t i32;
typedef uint32_t u32;



#define BAD_ARGS        1
#define FILE_SIZE_ERROR 2
#define RAM_UNAVAILABLE 3
#define FILE_READ_ERROR 4

static void display_error(const int errcode, const void *data)
{
    switch (errcode) {
        case BAD_ARGS:
            printf("??? %s\n", (char *)data);
            break;
        case FILE_SIZE_ERROR:
            printf("FILE SIZE ERROR! %d\n", *(signed *)data);
            break;
        case RAM_UNAVAILABLE:
            printf("RAM UNAVAILABLE!\n");
            break;
        case FILE_READ_ERROR:
            printf("FILE READ ERROR!\n");
            break;
        default:
            printf("\n##  Lempel-Ziv SLI Zip v1.02 [2023/07/31]  ##\n"
              "\nUsage:  lzsz [mode] [type] [infile]\n"
              "\nModes:\n  e  : Encode\n  d  : Decode\n"
              "\nTypes:\n  m  : MIO0   \"Mario\"\n"
              "  g  : SMSR00 \"Mario 2\"\n"
              "  z  : Yay0   \"Zelda\"\n"
              "  i  : Yaz0   \"Zelda 2\"\n"
              "  r  : Rvl0   \"Revolution\"\n\n");
            break;
    }

    return;
}



/*---------------------------------------------------------------------------

                          String Inversion Section

---------------------------------------------------------------------------*/



static void strinv(void *p, size_t n)
{
    size_t rem = n >> 1;
    char *lo = (char *)p;
    char *hi = &lo[n - 1];
    char tmp;

    while (rem--) {
        tmp = *lo;
        *lo++ = *hi;
        *hi-- = tmp;
    }

    return;
}



/*---------------------------------------------------------------------------

                            Pseudo-Vector Section

---------------------------------------------------------------------------*/



#define NULL_VEC_INFO 0
#define NULL_VEC_DATA 1

typedef struct vec_s {
    size_t sz, /* volumetric size of the 'vector' */
           wd, /* width of the data type for the 'vector' */
           ct; /* count of elements stored in the 'vector' */
    void *org, /* origin pointer */
         *cur, /* current pointer address from the origin */
         *nxt; /* target pointer address relative to the volume */
} vec_t;

static const char *vsubject[2] =
{
    "information",
    "data"
};

static void verror(int status)
{
    printf("/// ERROR ///\n>>> Vector ");
    printf("%s", vsubject[status]);
    printf(" is NULL!\n");
    exit(1);
}

static vec_t *vfree(vec_t *vec)
{
    free(vec->org);
    free(vec);
    return NULL;
}

static vec_t *valloc(const size_t size)
{
    vec_t *vec = (vec_t *)calloc(1, sizeof(vec_t));

    if (vec == NULL) {
        verror(NULL_VEC_INFO);
    }
    else {
        vec->sz = 0x10000 * size;
        vec->wd = size;
        vec->org = malloc(vec->sz);

        if (vec->org == NULL) {
            verror(NULL_VEC_DATA);
        }
        else {
            vec->cur = vec->org;
            vec->nxt = (void *)((size_t)vec->cur + vec->sz);
        }
    }

    return vec;
}

static vec_t *vrealloc(vec_t *vec)
{
    if (vec == NULL) {
        verror(NULL_VEC_INFO);
    }
    else {
        char *p = NULL;

        vec->sz += (0x10000 * vec->wd);
        vec->org = realloc(vec->org, vec->sz);
        p = (char *)vec->org;

        if (vec->org == NULL) {
            verror(NULL_VEC_DATA);
        }
        else {
            vec->cur = (void *)&p[vec->ct * vec->wd];
            vec->nxt = (void *)&p[vec->sz];
        }
    }

    return vec;
}

static void vappend(vec_t *vec, const void *data)
{
    if (vec == NULL) {
        verror(NULL_VEC_INFO);
    }
    else {
        if (vec->cur < vec->nxt) {
            char *s = (char *)vec->cur;

            memcpy(s, data, vec->wd);
            vec->cur = (void *)&s[vec->wd];
            vec->ct++;
        }
        else {
            vec = vrealloc(vec);
            vappend(vec, data);
        }
    }

    return;
}



/*---------------------------------------------------------------------------

                                 SLI Section

---------------------------------------------------------------------------*/



typedef struct hdr_s {
    u32 m, /* FourCC */
        s, /* Decoded Size */
        h, /* Offset to HalfWords */
        b; /* Offset to Bytes */
} hdr_t;

hdr_t header;
static i16 values[0x100];

static void initskip(u8 *pat, const i32 patlen)
{
    i32 i;

    i = 0;

    while (values[i] = (i16)patlen, ++i < 0x100);

    i = 0;

    while (values[pat[i]] = ((i16)patlen - (i16)i) + -1, ++i < patlen);

    return;
}

static i32 mischarsearch(u8 *pat, const i32 patlen,
                         u8 *text, const i32 textlen)
{
    i32 c, j, p;
    u32 t;

    if (textlen < patlen) {
        return textlen;
    }

    initskip(pat, patlen);
    p = patlen + -1;

    do {
        while (text[p] != pat[patlen + -1]) {
            p += (u32)values[(u32)text[p]];
        }

        j = patlen + -2;
        c = p;

        while (1) {
            p = c + -1;

            if (j < 0) {
                return c;
            }

            if (text[p] != pat[j]) {
                break;
            }

            j += -1;
            c = p;
        }

        t = patlen - j;

        if ((patlen - j) <= (i32)(u32)values[(u32)text[p]]) {
            t = (u32)values[(u32)text[p]];
        }

        p += t;
    } while (1);
}

static void search(u8 *src, u8 *srcp, u8 *srcz, u32 *o, u32 *l, const u32 x)
{
    u32 cnt = srcz - srcp, mm = 3U, ms;
    u32 posp = 0;
    u8 *pos = (((srcp - src) < 0x1001U) ? src : &srcp[~0xFFF]);

    if ((x - 1U) < cnt) {
        cnt = x;
    }

    if (cnt < 3U) {
        *l = 0;
        *o = 0;
    }
    else {
        while ((pos < srcp) &&
               (ms = mischarsearch(srcp, mm, pos, &srcp[mm] - pos),
                ms < (srcp - pos))) {
            while ((mm < cnt) && (*&pos[mm + ms] == *&srcp[mm])) {
                ++mm;
            }

            if (cnt == mm) {
                *o = &srcp[-1] - &pos[ms];
                *l = cnt;
                return;
            }

            posp = &srcp[-1] - &pos[ms];
            ++mm;
            pos += (ms + 1U);
        }

        *o = posp;
        *l = (mm < 4U) ? 0 : (mm - 1U);
    }

    return;
}

static void assemble_tables(u8 *dstp, u8 *dstz,
                            vec_t *flags, vec_t *dicts, vec_t *bytes)
{
    u32 *c = (u32 *)&dstp[0], *cz = &c[flags->ct],
        *cmdp = flags->org, tmp;
    u16 *p = (u16 *)&dstp[flags->ct << 2], *pz = &p[dicts->ct],
        *polp = dicts->org;
    u8 *defp = bytes->org;

    do {
        dstp = &dstp[(flags->ct << 2) + (dicts->ct << 1)];

        while (c < cz) {
            tmp = *cmdp++;
            strinv(&tmp, 4);
            *c++ = tmp;
        }

        while (p < pz) {
            tmp = *polp++;
            strinv(&tmp, 2);
            *p++ = tmp;
        }

        while (dstp < dstz) {
            *dstp++ = *defp++;
        }
    } while (0);

    return;
}

static void assemble_groups(u8 *dstp, u8 *dstz,
                            vec_t *flags, vec_t *dicts, vec_t *bytes)
{
    u16 *h = (u16 *)&dstp[0], *hz = &h[flags->ct + dicts->ct],
        *cmdp = flags->org, *polp = dicts->org, n = 0, tmp;
    u8 *defp = bytes->org;
    i16 f;

    do {
        while (h < hz) {
            if (n != 0) {
                if ((f < 0) == 0) {
                    tmp = *polp++;
                    strinv(&tmp, 2);
                    *h++ = tmp;
                }

                f <<= 1;
                --n;
            }
            else {
                tmp = *cmdp++;
                f = (i16)tmp;
                strinv(&tmp, 2);
                *h++ = tmp;
                n = 16;
            }
        }

        dstp = &dstp[(flags->ct << 1) + (dicts->ct << 1)];

        while (dstp < dstz) {
            *dstp++ = *defp++;
        }
    } while (0);

    return;
}

static void assemble_stream(u8 *dstp, u8 *dstz,
                            vec_t *flags, vec_t *dicts, vec_t *bytes)
{
    u8 *cmdp = flags->org, *defp = bytes->org, n = 0, f;
    u16 *polp = dicts->org, tmp;

    do {
        while (dstp < dstz) {
            if (n != 0) {
                if ((f & 0x80) == 0) {
                    tmp = *polp;
                    strinv(&tmp, 2);
                    memcpy(dstp, &tmp, 2);
                    dstp = &dstp[2];

                    if ((*polp >> 12) == 0) {
                        *dstp++ = *defp++;
                    }

                    polp = &polp[1];
                }
                else {
                    *dstp++ = *defp++;
                }

                f <<= 1;
                --n;
            }
            else {
                f = *cmdp++;
                *dstp++ = f;
                n = 8;
            }
        }
    } while (0);

    return;
}

static void encode(u8 *src, u8 *srcz, FILE *ofile)
{
    void (*assemble)(u8 *, u8 *, vec_t *, vec_t *, vec_t *);
    u8 *srcp, *dst, *dstp, *dstz, b;
    u16 h;
    u32 mask, bitflags, o[2], l[2], size, T;
    vec_t *bytes, *dicts, *flags;
    u32 q[2][5] = { { 0x12, 0x12,
                      0x111, 0x111,
                      0x10110 },
                    { 0x4D494F30, 0x534D5352,
                      0x59617930, 0x59617A30,
                      0x52766C30 } },
        x = *&q[0][*srcz],
        m = *&q[1][*srcz];

    bitflags = 0x00000000U;
    bytes = valloc(1);
    dicts = valloc(2);

    switch (*srcz) {
        case 1:     /* Mario 2 */
            T = 0x8000U;
            flags = valloc(2);
            break;
        case 3:     /* Zelda 2 */
            T = 0x80U;
            flags = valloc(1);
            break;
        default:
            T = 0x80000000U;
            flags = valloc(4);
            break;
    }

    mask = T;
    srcp = src;

    while (srcp < srcz) {
        search(src, srcp, srcz, &o[0], &l[0], x);

        if (l[0] < 3U) {
            bitflags |= mask;
            vappend(bytes, srcp);
            srcp = &srcp[1];
        }
        else {
            search(src, &srcp[1], srcz, &o[1], &l[1], x);

            if ((l[0] + 1U) < l[1]) {
                bitflags |= mask;
                vappend(bytes, srcp);
                srcp = &srcp[1];

                if ((mask >>= 1) == 0) {
                    mask = T;
                    vappend(flags, &bitflags);
                    bitflags = 0x00000000U;
                }

                l[0] = l[1];
                o[0] = o[1];
            }
            
            switch (*srcz) {
                case 2:     /* Zelda */
                case 3:     /* Zelda 2 */
                    if (l[0] < 0x12U) {
                        h = ((((u16)l[0] - 2U) * 0x1000U) | (u16)o[0]);
                    }
                    else {
                        h = (u16)o[0];
                        b = (u8)(l[0] - 0x12U);
                        vappend(bytes, &b);
                    }
                    break;
                case 4:     /* Revolution */
                    if (l[0] < 0x11U) {
                        h = ((((u16)l[0] - 1U) * 0x1000U) | (u16)o[0]);
                    }
                    else if (l[0] < 0x111U) {
                        h = (u16)o[0];
                        b = (u8)(l[0] - 0x11U);
                        vappend(bytes, &b);
                    }
                    else {
                        h = 0x1000 | (u16)o[0];
                        vappend(dicts, &h);
                        h = (u16)(l[0] - 0x111U);
                    }
                    break;
                default:    /* Mario | Mario 2 */
                    h = ((((u16)l[0] - 3U) * 0x1000U) | (u16)o[0]);
                    break;
            }

            vappend(dicts, &h);
            srcp = &srcp[l[0]];
        }

        if ((mask >>= 1) == 0) {
            mask = T;
            vappend(flags, &bitflags);
            bitflags = 0x00000000U;
        }
    }

    if (mask != T) {
        vappend(flags, &bitflags);
    }

    header.m = m;
    header.s = srcz - src;

    switch (*srcz) {
        case 1:     /* Mario 2 */
            header.h = header.s;
            header.s = 0x30300000U;
            header.b = (flags->ct << 1) + (dicts->ct << 1);
            size = (u32)(header.b + bytes->ct + 0x10);
            break;
        case 3:     /* Zelda 2 */
            header.h = 0;
            header.b = 0;
            size = (u32)(0x10 + flags->ct + (dicts->ct << 1) + bytes->ct);
            break;
        default:
            header.h = (flags->ct << 2) + 0x10;
            header.b = header.h + (dicts->ct << 1);
            size = (u32)(header.b + bytes->ct);
            break;
    }

    dst = (u8 *)calloc(size, sizeof(u8));
    dstp = dst;
    dstz = &dst[size];

    if (dst == NULL) {
        display_error(RAM_UNAVAILABLE, NULL);
        goto err;
    }

    do {
        u32 *hdr = (u32 *)&dstp[0x00], *hdrz = &hdr[4],
            *g = (u32 *)&header;
        u32 tmp;

        while (hdr < hdrz) {
            tmp = *g++;
            strinv(&tmp, 4);
            *hdr++ = tmp;
        }
    } while (0);

    dstp = &dstp[0x10];

    switch (*srcz) {
        case 1:     /* Mario 2 */
            assemble = assemble_groups;
            break;
        case 3:     /* Zelda 2 */
            assemble = assemble_stream;
            break;
        default:
            assemble = assemble_tables;
            break;
    }

    assemble(dstp, dstz, flags, dicts, bytes);
    fwrite(dst, sizeof(u8), dstz - dst, ofile);
    fflush(ofile);
    free(dst);
    dst = NULL;

err:

    bytes = vfree(bytes);
    dicts = vfree(dicts);
    flags = vfree(flags);
    return;
}

static void decode(u8 *src, u8 *srcz, FILE *ofile)
{
    u8 *srcp, *dst, *dstp, *dstz, *w, *h, *b;
    i32 f;
    u32 n, tmp, T, t, tt;

    tmp = *(u32 *)&src[(*srcz == 1) ? 0x08 : 0x04];
    strinv(&tmp, 4);

    if ((dst = (u8 *)calloc(tmp, sizeof(u8))) == NULL) {
        display_error(RAM_UNAVAILABLE, NULL);
        goto end;
    }

    dstp = dst;
    dstz = &dstp[tmp];
    tmp = *(u32 *)&src[0x08];
    strinv(&tmp, 4);
    h = &src[tmp];
    tmp = *(u32 *)&src[0x0C];
    strinv(&tmp, 4);
    b = &src[tmp + ((*srcz == 1) ? 0x10 : 0)];
    n = 0;
    w = &src[0x10];
    srcp = &src[0x10];

    switch (*srcz) {
        case 1:
            T = 2;
            break;
        case 3:
            T = 1;
            break;
        default:
            T = 4;
            break;
    }

    t = (8 << (T >> 1));
    tt = (32 - (T << 3));

    do {
        if (n != 0) {
            if ((f < 0) == 0) {
                u16 d;
                u32 l;
                u8 *p;

                switch (*srcz) {
                    case 1:
                    case 3:
                        memcpy(&d, w, 2);
                        w = &w[2];
                        break;
                    default:
                        memcpy(&d, h, 2);
                        h = &h[2];
                        break;
                }

                strinv(&d, 2);
                l = d >> 12;
                d &= 0xFFF;
                p = &dstp[~d];
                srcp = &srcp[2];

                switch (*srcz) {
                    case 2:     /* Zelda */
                    case 3:     /* Zelda 2 */
                        if (l == 0) {
                            l = (u32)((*srcz == 3) ? *w++ : *b++);
                            l += 18U;
                            srcp = &srcp[1];
                        }
                        else {
                            l += 2U;
                        }
                        break;
                    case 4:     /* Revolution */
                        if (l == 0) {
                            l = (u32)*b++;
                            l += 17U;
                            srcp = &srcp[1];
                        }
                        else if (l == 1) {
                            memcpy(&l, h, 2);
                            h = &h[2];
                            strinv(&l, 2);
                            l += 273U;
                            srcp = &srcp[2];
                        }
                        else {
                            ++l;
                        }
                        break;
                    default:    /* Mario */
                        l += 3U;
                        break;
                }

                do {
                    *dstp++ = *p++;
                } while (--l);
            }
            else {
                *dstp++ = *b++;
                srcp = &srcp[1];
            }

            f <<= 1;
            --n;
        }
        else {
            memcpy(&f, w, T);
            w = &w[T];
            strinv(&f, T);
            f <<= tt;
            n = t;
            srcp = &srcp[T];
        }
    } while ((dstp < dstz) && (srcp < srcz));

    fwrite(dst, sizeof(u8), dstz - dst, ofile);
    fflush(ofile);
    free(dst);
    dst = NULL;

end:

    return;
}

static void time_elapsed(void)
{
    struct tm time;
    unsigned msec, sec;
    char fmt[12];

    msec = clock();
    sec = msec / CLOCKS_PER_SEC;
    memset(fmt, 0, 12);
    memset(&time, 0, sizeof(time));
    time.tm_yday = sec / 86400;
    time.tm_hour = sec / 3600;
    time.tm_min = (sec / 60) % 60;
    time.tm_sec = sec % 60;
    msec %= 1000;
    strftime(fmt, 12, "%Hh:%Mm:%Ss", &time);
    printf("Time Elapsed: %u day(s), %s;%.3ums\n",
           time.tm_yday, fmt, msec);
    return;
}

static float ratio(int b, float f1, float f2)
{
    float f[2] = { f1, f2 };
    int bb = f1 < f2;

    bb = !((bb == b) || (bb == !b));
    return (((bb == 1) ? (f[0] / f[1]) : (f[1] / f[0])) * 100.0f);
}

int main(int argc, char *argv[])
{
    void (*op)(u8 *, u8 *, FILE *);
    FILE *ifile = NULL, *ofile = NULL;
    u8 *src = NULL, *srcz;
    char *s, o[240];
    ssize_t isize, osize = 0;

    if (argc != 4) {
        display_error(0, NULL);
        exit(EXIT_FAILURE);
    }

    if ((s = argv[1], s[1] || strpbrk(s, "DEde") == NULL) ||
        (s = argv[2], s[1] || strpbrk(s, "MGZIRmgzir") == NULL) ||
        (s = argv[3], (ifile = fopen(s, "rb")) == NULL)) {
        display_error(BAD_ARGS, (void *)s);
        exit(EXIT_FAILURE);
    }

    strcpy(o, argv[3]);

    if (toupper(*argv[1]) == 'D') {
        char *a = o;
        char *z = &a[strlen(o)];

        a = &a[2];

        do {
            if (*z == '.') {
                *z = '\0';
                break;
            }
            else if ((*z == '\\') || (*z == '/')) {
                strcat(o, ".bin");
                break;
            }
            else if (z <= a) {
                strcat(o, "_decoded_.bin");
                break;
            }
            else {
                z = &z[-1];
            }
        } while (1);
    }
    else {
        switch (toupper(*argv[2])) {
            case 'I':
                strcat(o, ".szs");
                break;
            default:
                strcat(o, ".szp");
                break;
        }
    }

    if ((ofile = fopen(o, "wb")) == NULL) {
        display_error(BAD_ARGS, (void *)o);
        exit(EXIT_FAILURE);
    }

    fseek(ifile, 0L, SEEK_END);
    isize = (ssize_t)ftell(ifile);
    fseek(ifile, 0L, SEEK_SET);

    if ((isize <= 0) || (isize >= 0x3FFFFFFF)) {
        display_error(FILE_SIZE_ERROR, (void *)&isize);
        goto nil;
    }

/*  Note the "(isize + 1)". */
    if ((src = (u8 *)malloc((sizeof(u8) * (isize + 1)))) == NULL) {
        display_error(RAM_UNAVAILABLE, NULL);
        goto nil;
    }

    if (fread(src, sizeof(u8), isize, ifile) != (size_t)isize) {
        display_error(FILE_READ_ERROR, NULL);
        goto nil;
    }

    op = (toupper(*argv[1]) == 'E') ? encode : decode;
    srcz = &src[isize];
/*  With the suffix byte, we can send simple config information. */
    switch (toupper(*argv[2])) {
        case 'G':   /* Mario 2 */
            *srcz = 1;
            break;
        case 'Z':   /* Zelda */
            *srcz = 2;
            break;
        case 'I':   /* Zelda 2 */
            *srcz = 3;
            break;
        case 'R':   /* Revolution */
            *srcz = 4;
            break;
        default:    /* Mario */
            *srcz = 0;
            break;
    }

    op(src, srcz, ofile);
    osize = (ssize_t)ftell(ofile);

nil:

    fclose(ofile);
    fclose(ifile);

    if (src != NULL) {
        free(src);
        src = NULL;
    }

    if (osize <= 0) {
        remove(o);
    }

    if ((isize > 0) && (osize > 0)) {
        printf(">>> IN: %u , OUT: %u , RATIO: %3.2f%%\n",
               (unsigned)isize, (unsigned)osize,
               ratio(toupper(*argv[1]) == 'E', isize, osize));
    }

    time_elapsed();
    exit(EXIT_SUCCESS);
}
