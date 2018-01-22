#include "zlib_wrapper.h"

#define CHUNK 20000

#include <stdio.h>
#include <zlib.h>

/*********** Methods declarations *************/
// source: http://docs.biicode.com/c++/examples/zlib.html

/* Compress from file source to file dest until EOF on source.
def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
allocated for processing, Z_STREAM_ERROR if an invalid compression
level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
version of the library linked do not match, or Z_ERRNO if there is
an error reading or writing the files. */
int def(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
         return ret;

    /* compress until end of file */
    do {
     strm.avail_in = fread(in, 1, CHUNK, source);
     if (ferror(source)) {
       (void)deflateEnd(&strm);
       return Z_ERRNO;
     }
     flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
     strm.next_in = in;

    /* run deflate() on input until output buffer not full, finish
     compression if all of source has been read in */
     do {
       strm.avail_out = CHUNK;
       strm.next_out = out;
       ret = deflate(&strm, flush); /* no bad return value */
       if (ret == Z_STREAM_ERROR)
       {
           (void)deflateEnd(&strm);
           throw ZlibWrapperException("not a valid compression level");
       }
       have = CHUNK - strm.avail_out;

       if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
         (void)deflateEnd(&strm);
         return Z_ERRNO;
        }
      } while (strm.avail_out == 0);
      if (strm.avail_in != 0)
      {
          (void)deflateEnd(&strm);
          throw ZlibWrapperException("not all input will be used");
      }

    /* done when last data in file processed */
    } while (flush != Z_FINISH);
    if (ret != Z_STREAM_END)
    {
        (void)deflateEnd(&strm);
        throw ZlibWrapperException("stream of compression is not completed");
    }

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
allocated for processing, Z_DATA_ERROR if the deflate data is
invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
the version of the library linked do not match, or Z_ERRNO if there
is an error reading or writing the files. */
int inf(FILE *source, FILE* dest)
{
   int ret;
   unsigned have;
   z_stream strm;
   unsigned char in[CHUNK];
   unsigned char out[CHUNK];

  /* allocate inflate state */
   strm.zalloc = Z_NULL;
   strm.zfree = Z_NULL;
   strm.opaque = Z_NULL;
   strm.avail_in = 0;
   strm.next_in = Z_NULL;
   ret = inflateInit(&strm);
   if (ret != Z_OK)
     return ret;

  /* decompress until deflate stream ends or end of file */
   do {
     strm.avail_in = fread(in, 1, CHUNK, source);
     if (ferror(source)) {
       (void)inflateEnd(&strm);
       return Z_ERRNO;
     }
     if (strm.avail_in == 0)
       break;
     strm.next_in = in;

    /* run inflate() on input until output buffer not full */
     do {
       strm.avail_out = CHUNK;
       strm.next_out = out;
       ret = inflate(&strm, Z_NO_FLUSH);
       if (ret == Z_STREAM_ERROR)
       {
           (void)inflateEnd(&strm);
           throw ZlibWrapperException("not a valid compression level");
       }
       switch (ret) {
         case Z_NEED_DICT:
         ret = Z_DATA_ERROR; /* and fall through */
         case Z_DATA_ERROR:
         case Z_MEM_ERROR:
        (void)inflateEnd(&strm);
         return ret;
       }
       have = CHUNK - strm.avail_out;
       if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
         (void)inflateEnd(&strm);
         return Z_ERRNO;
       }
      } while (strm.avail_out == 0);

     /* done when inflate() says it's done */
     } while (ret != Z_STREAM_END);

  /* clean up and return */
   (void)inflateEnd(&strm);
   return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

void compress(const std::string &src, const std::string &dest, int level)
{
    FILE *src_file=NULL;    //original file
    FILE *dest_file=NULL;   //file to compress or decompress
    src_file = fopen(src.c_str(),"r");
    if(!src_file)
        throw ZlibWrapperException(std::string("File for compression does not exist: ") + src);
    dest_file = fopen(dest.c_str(),"wb");   //wb because write to binary format
    if(!dest_file)
        throw ZlibWrapperException(std::string("Can't open file for writting: ") + dest);

    int ret = def(src_file, dest_file, level);
    fclose(src_file);
    fclose(dest_file);
    if (ret != Z_OK)
        throw ZlibWrapperException(ret);
}

void uncompress(const std::string &src, const std::string &dest)
{
    FILE *src_file=NULL;    //original file
    FILE *dest_file=NULL;   //file to compress or decompress
    src_file = fopen(src.c_str(),"rb");    //rb because read from binary format
    if(!src_file)
        throw ZlibWrapperException(std::string("File for uncompression does not exist: ") + src);
    dest_file = fopen(dest.c_str(),"w");
    if(!dest_file)
        throw ZlibWrapperException(std::string("Can't open file for writting: ") + dest);

    int ret = inf(src_file, dest_file);
    fclose(src_file);
    fclose(dest_file);
    if (ret != Z_OK)
        throw ZlibWrapperException(ret);
}

ZlibWrapperException::ZlibWrapperException(int ret) {
    switch (ret) {
      case Z_ERRNO:
        if (ferror(stdin))
          msg.append("error reading stdin");
        if (ferror(stdout))
          msg.append("error writing stdout");
        break;
      case Z_STREAM_ERROR:
        msg.append("invalid compression level");
        break;
      case Z_DATA_ERROR:
        msg.append("invalid or incomplete deflate data");
        break;
      case Z_MEM_ERROR:
        msg.append("out of memory");
        break;
      case Z_VERSION_ERROR:
        msg.append("zlib version mismatch!");
    }
}
