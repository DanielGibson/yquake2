/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2015 Daniel Gibson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * The .m8 and .m32 texture formats as used by Heretic2 (and probably SoF)
 *
 * =======================================================================
 */

#include "../header/local.h"

static const int H2_MIPLEVELS = 16; // WTF, who needs 16 mipmap levels?!
static const int H2_PAL_SIZE = 256;

struct palette {
	byte r, g, b;
};

#define eprintf(...) fprintf(stderr, __VA_ARGS__) // TODO: replace this with the proper Q2 thing!

// TODO: will we need this somewhere else? for the animnanme etc maybe?
typedef struct m8tex_s
{
	int version; // H2 specific
	char name[32];
	//unsigned width, height; - no more!
	unsigned widths[H2_MIPLEVELS]; // H2 specific
	unsigned heights[H2_MIPLEVELS]; // ditto
	unsigned offsets[H2_MIPLEVELS]; // more elements than Q2
	char animname[32];           /* next frame in animation chain */
	struct palette	palette[H2_PAL_SIZE]; // H2 specific
	int flags;
	int contents;
	int value;
} m8tex_t;

// from quakesrc, see http://www.quake-1.com/docs/quakesrc.org/21.html
typedef struct m32tex_s
{

	int		version;
	char	name[128];
	char	altname[128];			// texture substitution
	char	animname[128];			// next frame in animation chain
	char	damagename[128];		// image that should be shown when damaged

	unsigned	width[H2_MIPLEVELS];
	unsigned	height[H2_MIPLEVELS];
	unsigned	offsets[H2_MIPLEVELS];

	int		flags;
	int		contents;
	int		value;
	float	scale_x, scale_y;
	int		mip_scale;


	// detail texturing info
	char		dt_name[128];		// detailed texture name
	float		dt_scale_x, dt_scale_y;
	float		dt_u, dt_v;
	float		dt_alpha;
	int		dt_src_blend_mode, dt_dst_blend_mode;
	int		flags2;
	float		damage_health;

	int		unused[18];				// future expansion to maintain compatibility with h2

} m32tex_t;

qboolean
LoadM8(const char* origname, byte** pic, int* outWidth, int* outHeight)
{
	char filename[256] = {0};
	int i=0;

	assert(sizeof(struct palette) == 3);

	Q_strlcpy(filename, origname, sizeof(filename));

	/* Add the extension */
	if (strcmp(COM_FileExtension(filename), "m8") != 0)
	{
		Q_strlcat(filename, ".m8", sizeof(filename));
	}

	if(pic != NULL) *pic = NULL;
	if(outWidth != NULL) *outWidth = 0;
	if(outHeight != NULL) *outHeight = 0;

	m8tex_t* texData = NULL;
	int rawsize = FS_LoadFile(filename, (void **)&texData);
	if (texData == NULL)
	{
		printf("## loading file %s failed!\n", filename);
		return false;
	}

	const int w = LittleLong(texData->widths[0]);
	const int h = LittleLong(texData->heights[0]);
	const int offs = LittleLong(texData->offsets[0]);
	if(outWidth != NULL) *outWidth = w;
	if(outHeight != NULL) *outHeight = h;

	//printf("# %s w: %d h: %d offs: %d\n", filename, w, h, offs);

	if(pic != NULL)
	{
		// Note: I only read the first (biggest) mipmap and ignore the others.
		byte* imgData = ((byte*)texData)+offs;

		int numPixels = w*h;
		// screw the palette, I convert the texture to 32bit RGBA on load
		byte* outData = malloc(numPixels*4); // 4 for RGBA
		*pic = outData;
		byte* cur = outData;

		for(i=0; i<numPixels; ++i)
		{
			struct palette* palEntry = &texData->palette[imgData[i]];
			*cur++ = palEntry->r;
			*cur++ = palEntry->g;
			*cur++ = palEntry->b;
			*cur++ = 255; // for alpha, so we get opaque RGBA
		}
	}

	FS_FreeFile(texData);
	return true;
}

qboolean
LoadM32(const char *origname, byte **pic, /* byte **palette,*/ int *outWidth, int *outHeight)
{
	char filename[256];

	Q_strlcpy(filename, origname, sizeof(filename));

	/* Add the extension */
	if (strcmp(COM_FileExtension(filename), "m32") != 0)
	{
		Q_strlcat(filename, ".m32", sizeof(filename));
	}

	if(pic != NULL) *pic = NULL;
	if(outWidth != NULL) *outWidth = 0;
	if(outHeight != NULL) *outHeight = 0;

	m32tex_t* texData = NULL;
	int rawsize = FS_LoadFile(filename, (void **)&texData);
	if (texData == NULL)
	{
		printf("## loading file %s failed!\n", filename);
		return false;
	}

	const int w = texData->width[0];
	const int h = texData->height[0];
	const int offs = texData->offsets[0];

	if(outWidth != NULL) *outWidth = w;
	if(outHeight != NULL) *outHeight = h;

	if(texData->dt_name[0] != '\0')
	{
		printf("## Texture %s has detail texture: %s\n", filename, texData->dt_name);
	}

	if(pic != NULL)
	{
		// Note: I only read the first (biggest) mipmap and ignore the others.
		byte* imgData = ((byte*)texData)+offs;

		int numPixels = w*h;
		byte* outData = malloc(numPixels*4); // 4 for RGBA
		*pic = outData;
		memcpy(outData, imgData, numPixels*4);
	}

	FS_FreeFile(texData);
	return true;
}

void
GetM8Info(char *name, int *width, int *height)
{
	LoadM8(name, NULL, width, height);
}

void
GetM32Info(char *name, int *width, int *height)
{
	LoadM32(name, NULL, width, height);
}

