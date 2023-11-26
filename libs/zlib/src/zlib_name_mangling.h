/* zlib_name_mangling.h has been automatically generated from
 * zlib_name_mangling.h.in because ZLIB_SYMBOL_PREFIX was set.
 */

#ifndef ZLIB_NAME_MANGLING_H
#define ZLIB_NAME_MANGLING_H

/* all linked symbols and init macros */
#define _dist_code            _dist_code
#define _length_code          _length_code
#define _tr_align             _tr_align
#define _tr_flush_bits        _tr_flush_bits
#define _tr_flush_block       _tr_flush_block
#define _tr_init              _tr_init
#define _tr_stored_block      _tr_stored_block
#define _tr_tally             _tr_tally
#define adler32               adler32
#define adler32_combine       adler32_combine
#define adler32_combine64     adler32_combine64
#define adler32_z             adler32_z
#ifndef Z_SOLO
#  define compress              compress
#  define compress2             compress2
#  define compressBound         compressBound
#endif
#define crc32                 crc32
#define crc32_combine         crc32_combine
#define crc32_combine64       crc32_combine64
#define crc32_combine_gen     crc32_combine_gen
#define crc32_combine_gen64   crc32_combine_gen64
#define crc32_combine_op      crc32_combine_op
#define crc32_z               crc32_z
#define deflate               deflate
#define deflateBound          deflateBound
#define deflateCopy           deflateCopy
#define deflateEnd            deflateEnd
#define deflateGetDictionary  deflateGetDictionary
#define deflateInit           deflateInit
#define deflateInit2          deflateInit2
#define deflateInit2_         deflateInit2_
#define deflateInit_          deflateInit_
#define deflateParams         deflateParams
#define deflatePending        deflatePending
#define deflatePrime          deflatePrime
#define deflateReset          deflateReset
#define deflateResetKeep      deflateResetKeep
#define deflateSetDictionary  deflateSetDictionary
#define deflateSetHeader      deflateSetHeader
#define deflateTune           deflateTune
#define deflate_copyright     deflate_copyright
#define fill_window           fill_window
#define fixedtables           fixedtables
#define flush_pending         flush_pending
#define get_crc_table         get_crc_table
#ifndef Z_SOLO
#  define gz_error              gz_error
#  define gz_strwinerror        gz_strwinerror
#  define gzbuffer              gzbuffer
#  define gzclearerr            gzclearerr
#  define gzclose               gzclose
#  define gzclose_r             gzclose_r
#  define gzclose_w             gzclose_w
#  define gzdirect              gzdirect
#  define gzdopen               gzdopen
#  define gzeof                 gzeof
#  define gzerror               gzerror
#  define gzflush               gzflush
#  define gzfread               gzfread
#  define gzfwrite              gzfwrite
#  define gzgetc                gzgetc
#  define gzgetc_               gzgetc_
#  define gzgets                gzgets
#  define gzoffset              gzoffset
#  define gzoffset64            gzoffset64
#  define gzopen                gzopen
#  define gzopen64              gzopen64
#  ifdef _WIN32
#    define gzopen_w              gzopen_w
#  endif
#  define gzprintf              gzprintf
#  define gzputc                gzputc
#  define gzputs                gzputs
#  define gzread                gzread
#  define gzrewind              gzrewind
#  define gzseek                gzseek
#  define gzseek64              gzseek64
#  define gzsetparams           gzsetparams
#  define gztell                gztell
#  define gztell64              gztell64
#  define gzungetc              gzungetc
#  define gzvprintf             gzvprintf
#  define gzwrite               gzwrite
#endif
#define inflate               inflate
#define inflateBack           inflateBack
#define inflateBackEnd        inflateBackEnd
#define inflateBackInit       inflateBackInit
#define inflateBackInit_      inflateBackInit_
#define inflateCodesUsed      inflateCodesUsed
#define inflateCopy           inflateCopy
#define inflateEnd            inflateEnd
#define inflateGetDictionary  inflateGetDictionary
#define inflateGetHeader      inflateGetHeader
#define inflateInit           inflateInit
#define inflateInit2          inflateInit2
#define inflateInit2_         inflateInit2_
#define inflateInit_          inflateInit_
#define inflateMark           inflateMark
#define inflatePrime          inflatePrime
#define inflateReset          inflateReset
#define inflateReset2         inflateReset2
#define inflateResetKeep      inflateResetKeep
#define inflateSetDictionary  inflateSetDictionary
#define inflateSync           inflateSync
#define inflateSyncPoint      inflateSyncPoint
#define inflateUndermine      inflateUndermine
#define inflateValidate       inflateValidate
#define inflate_copyright     inflate_copyright
#define inflate_ensure_window inflate_ensure_window
#define inflate_fast          inflate_fast
#define inflate_table         inflate_table
#define read_buf              read_buf
#ifndef Z_SOLO
#  define uncompress            uncompress
#  define uncompress2           uncompress2
#endif
#define zError                zError
#ifndef Z_SOLO
#  define zcalloc               zcalloc
#  define zcfree                zcfree
#endif
#define zlibCompileFlags      zlibCompileFlags
#define zlibVersion           zlibVersion

/* all zlib typedefs in zlib.h and zconf.h */
#define Byte                  Byte
#define Bytef                 Bytef
#define alloc_func            alloc_func
#define charf                 charf
#define free_func             free_func
#ifndef Z_SOLO
#  define gzFile                gzFile
#endif
#define gz_header             gz_header
#define gz_headerp            gz_headerp
#define in_func               in_func
#define intf                  intf
#define out_func              out_func
#define uInt                  uInt
#define uIntf                 uIntf
#define uLong                 uLong
#define uLongf                uLongf
#define voidp                 voidp
#define voidpc                voidpc
#define voidpf                voidpf

/* all zlib structs in zlib.h and zconf.h */
#define gz_header_s           gz_header_s
#define internal_state        internal_state

/* all zlib structs in zutil.h */
#define z_errmsg              z_errmsg
#define z_vstring             z_vstring
#define zlibng_version        zlibng_version

/* zlib-ng specific symbols */
#define zng_alloc_aligned     zng_alloc_aligned
#define zng_free_aligned      zng_free_aligned

#endif /* ZLIB_NAME_MANGLING_H */
