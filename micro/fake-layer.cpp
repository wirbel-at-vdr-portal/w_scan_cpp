/*
  Pure FAKE functions for linking (at compile time) "w_scan_cpp"
  without these libraries:

    - fontconfig
    - freetype2
    - jpeg
    
  This works because this tool is command line only, and it uses
  VDR and third party plugins. However, the graphical part is not
  used at all. Therefore, we can overcome entirelly this code.
  With a regular compilation the calls to these libraries exist,
  but in fact they never be called. So we can provide a fake
  implementation to link without these libraries.
  
  Note: You need the original headers to complete the compilation.
*/

#include <fontconfig/fontconfig.h>

#include <ft2build.h>
#include <freetype/freetype.h>

#include <jpeglib.h>

/* ************************************************************************ */
/* fontconfig.h */

FcPublic FcBool
FcInit (void)
{return 1;}

FcPublic FcObjectSet *
FcObjectSetBuild (const char *first, ...)
{return 0;}

FcPublic FcPattern *
FcPatternCreate (void)
{return 0;}

FcPublic FcFontSet *
FcFontList (FcConfig	*config,
	    FcPattern	*p,
	    FcObjectSet *os)
{return 0;}

FcPublic FcBool
FcPatternAddBool (FcPattern *p, const char *object, FcBool b)
{return 1;}

FcPublic FcChar8 *
FcNameUnparse (FcPattern *pat)
{return 0;}

FcPublic void
FcFontSetDestroy (FcFontSet *s)
{return ;}

FcPublic void
FcPatternDestroy (FcPattern *p)
{return ;}

FcPublic void
FcObjectSetDestroy (FcObjectSet *os)
{return ;}

FcPublic FcBool
FcPatternAddInteger (FcPattern *p, const char *object, int i)
{return 1;}

FcPublic FcBool
FcConfigSubstitute (FcConfig	*config,
		    FcPattern	*p,
		    FcMatchKind	kind)
{return 1;}

FcPublic void
FcDefaultSubstitute (FcPattern *pattern)
{return ;}

FcPublic FcFontSet *
FcFontSort (FcConfig	 *config,
	    FcPattern    *p,
	    FcBool	 trim,
	    FcCharSet    **csp,
	    FcResult	 *result)
{return 0;}

FcPublic FcResult
FcPatternGetBool (const FcPattern *p, const char *object, int n, FcBool *b)
{return (FcResult)0;}

FcPublic FcResult
FcPatternGetString (const FcPattern *p, const char *object, int n, FcChar8 ** s)
{return (FcResult)0;}

FcPublic FcPattern *
FcNameParse (const FcChar8 *name)
{return 0;}


/* ************************************************************************ */
/* ft2build.h */

  FT_EXPORT( FT_UInt )
  FT_Get_Char_Index( FT_Face   face,
                     FT_ULong  charcode )
{return 1;}

  FT_EXPORT( FT_Error )
  FT_Get_Kerning( FT_Face     face,
                  FT_UInt     left_glyph,
                  FT_UInt     right_glyph,
                  FT_UInt     kern_mode,
                  FT_Vector  *akerning )
{return 0;}

  FT_EXPORT( FT_Error )
  FT_Load_Glyph( FT_Face   face,
                 FT_UInt   glyph_index,
                 FT_Int32  load_flags )
{return 0;}
          
  FT_EXPORT( FT_Error )
  FT_Render_Glyph( FT_GlyphSlot    slot,
                   FT_Render_Mode  render_mode )
{return 0;}

  FT_EXPORT( FT_Error )
  FT_Init_FreeType( FT_Library  *alibrary )
{return 0;}

  FT_EXPORT( FT_Error )
  FT_New_Face( FT_Library   library,
               const char*  filepathname,
               FT_Long      face_index,
               FT_Face     *aface )
{return 0;}

  FT_EXPORT( FT_Error )
  FT_Set_Char_Size( FT_Face     face,
                    FT_F26Dot6  char_width,
                    FT_F26Dot6  char_height,
                    FT_UInt     horz_resolution,
                    FT_UInt     vert_resolution )               
{return 0;}

  FT_EXPORT( FT_Error )
  FT_Done_Face( FT_Face  face )
{return 0;}

  FT_EXPORT( FT_Error )
  FT_Done_FreeType( FT_Library  library )
{return 0;}


/* ************************************************************************ */
/* jpeglib.h */

EXTERN(struct jpeg_error_mgr *) jpeg_std_error(struct jpeg_error_mgr *err)
{return 0;}

EXTERN(void) jpeg_CreateCompress(j_compress_ptr cinfo, int version,
                                 size_t structsize)
{return ;}

EXTERN(void) jpeg_set_defaults(j_compress_ptr cinfo)
{return ;}

EXTERN(void) jpeg_set_quality(j_compress_ptr cinfo, int quality,
                              boolean force_baseline)
{return ;}

EXTERN(void) jpeg_start_compress(j_compress_ptr cinfo,
                                 boolean write_all_tables)
{return ;}

EXTERN(JDIMENSION) jpeg_write_scanlines(j_compress_ptr cinfo,
                                        JSAMPARRAY scanlines,
                                        JDIMENSION num_lines)
{return 0;}

EXTERN(void) jpeg_finish_compress(j_compress_ptr cinfo)
{return ;}

EXTERN(void) jpeg_destroy_compress(j_compress_ptr cinfo)
{return ;}


/* ************************************************************************ */
