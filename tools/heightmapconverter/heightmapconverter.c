
#include "quakedef.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "libs/stb_image.h"

#include "ini.h"

qboolean VARGS Q_snprintfz(char *dest, size_t size, const char *fmt, ...)
{
	va_list		argptr;
	size_t ret;

	va_start (argptr, fmt);
#ifdef _WIN32
	//doesn't null terminate.
	//returns -1 on truncation
	ret = _vsnprintf (dest, size, fmt, argptr);
	dest[size-1] = 0;	//shitty paranoia
#else
	//always null terminates.
	//returns length regardless of truncation.
	ret = vsnprintf (dest, size, fmt, argptr);
#endif
	va_end (argptr);
	//if ret is -1 (windows oversize, or general error) then it'll be treated as unsigned so really long. this makes the following check quite simple.
	return ret>=size;
}

void QDECL Q_strncpyz(char *d, const char *s, int n)
{
	int i;
	n--;
	if (n < 0)
		return;	//this could be an error

		for (i=0; *s; i++)
		{
			if (i == n)
				break;
			*d++ = *s++;
		}
		*d='\0';
}

typedef struct config {
	struct {
		char path[256];
		char name[256];
		float xscale;
		float yscale;
		float fog;
		char sky[256];
		char message[256];
		char defaultgroundtexture[256];
		float defaultgroundheight;
		char defaultwatertexture[256];
		float defaultwaterheight;
		int segmentsize;
		char exterior[256];
		float playerstartx;
		float playerstarty;
		float playerstartz;
		qboolean interpolate_weightmap;
		qboolean interpolate_lightmap;
	} output;
	struct {
		char ground[4][32];
		char heightmap[256];
		char weightmap[256];
		char lightmap[256];
	} input;
} config_t;

typedef struct context {
	config_t config;
	struct {
		uint16_t *pixels;
		int w;
		int h;
		int comp;
	} heightmap;
	struct {
		uint8_t *pixels;
		int w;
		int h;
		int comp;
	} weightmap;
	struct {
		uint8_t *pixels;
		int w;
		int h;
		int comp;
	} lightmap;
	int num_blocks_ew;
	int num_blocks_ns;
} context_t;

static qboolean atob(const char *str)
{
	if (strcasecmp(str, "true") == 0)
		return true;
	else
		return false;
}

static int config_handler(void *user, const char *section, const char *name, const char *value)
{
	config_t *config = (config_t *)user;

	if (Q_strcmp(section, "output") == 0)
	{
		if (Q_strcmp(name, "path") == 0)
			Q_strncpyz(config->output.path, value, sizeof(config->output.path));
		else if (Q_strcmp(name, "name") == 0)
			Q_strncpyz(config->output.name, value, sizeof(config->output.name));
		else if (Q_strcmp(name, "xscale") == 0)
			config->output.xscale = atof(value);
		else if (Q_strcmp(name, "yscale") == 0)
			config->output.yscale = atof(value);
		else if (Q_strcmp(name, "fog") == 0)
			config->output.fog = atof(value);
		else if (Q_strcmp(name, "sky") == 0)
			Q_strncpyz(config->output.sky, value, sizeof(config->output.sky));
		else if (Q_strcmp(name, "message") == 0)
			Q_strncpyz(config->output.message, value, sizeof(config->output.message));
		else if (Q_strcmp(name, "defaultgroundtexture") == 0)
			Q_strncpyz(config->output.defaultgroundtexture, value, sizeof(config->output.defaultgroundtexture));
		else if (Q_strcmp(name, "defaultgroundheight") == 0)
			config->output.defaultgroundheight = atof(value);
		else if (Q_strcmp(name, "defaultwatertexture") == 0)
			Q_strncpyz(config->output.defaultwatertexture, value, sizeof(config->output.defaultwatertexture));
		else if (Q_strcmp(name, "defaultwaterheight") == 0)
			config->output.defaultwaterheight = atof(value);
		else if (Q_strcmp(name, "segmentsize") == 0)
			config->output.segmentsize = atoi(value);
		else if (Q_strcmp(name, "exterior") == 0)
			Q_strncpyz(config->output.exterior, value, sizeof(config->output.exterior));
		else if (Q_strcmp(name, "playerstartx") == 0)
			config->output.playerstartx = atof(value);
		else if (Q_strcmp(name, "playerstarty") == 0)
			config->output.playerstarty = atof(value);
		else if (Q_strcmp(name, "playerstartz") == 0)
			config->output.playerstartz = atof(value);
		else if (Q_strcmp(name, "interpolate_weightmap") == 0)
			config->output.interpolate_weightmap = atob(value);
		else if (Q_strcmp(name, "interpolate_lightmap") == 0)
			config->output.interpolate_lightmap = atob(value);
	}
	else if (Q_strcmp(section, "input") == 0)
	{
		if (Q_strcmp(name, "ground0") == 0)
			Q_strncpyz(config->input.ground[0], value, sizeof(config->input.ground[0]));
		else if (Q_strcmp(name, "ground1") == 0)
			Q_strncpyz(config->input.ground[1], value, sizeof(config->input.ground[1]));
		else if (Q_strcmp(name, "ground2") == 0)
			Q_strncpyz(config->input.ground[2], value, sizeof(config->input.ground[2]));
		else if (Q_strcmp(name, "ground3") == 0)
			Q_strncpyz(config->input.ground[3], value, sizeof(config->input.ground[3]));
		else if (Q_strcmp(name, "heightmap") == 0)
			Q_strncpyz(config->input.heightmap, value, sizeof(config->input.heightmap));
		else if (Q_strcmp(name, "weightmap") == 0)
			Q_strncpyz(config->input.weightmap, value, sizeof(config->input.weightmap));
		else if (Q_strcmp(name, "lightmap") == 0)
			Q_strncpyz(config->input.lightmap, value, sizeof(config->input.lightmap));
	}

	return 1;
}

static float interpolate(float a, float b, float c, float d, float wx, float wy)
{
	return a * (1.0 - wx) * (1.0 - wy) + b * wx * (1.0 - wy) + c * wy * (1.0 - wx) + d * wx * wy;
}

static uint32_t get_weight(context_t *ctx, int sx, int sy, float fx, float fy)
{
	uint32_t ret = 0;
	uint8_t *arr = (uint8_t *)&ret;

	// scale to grid coordinates
	int gx = (sx * 16) + floorf(fx * 16);
	int gy = (sy * 16) + floorf(fy * 16);

	if (!ctx->config.output.interpolate_weightmap)
	{
		for (int i = 0; i < ctx->weightmap.comp; i++)
			arr[i] = ctx->weightmap.pixels[gy * (ctx->weightmap.w * ctx->weightmap.comp) + (gx * ctx->weightmap.comp) + i];
		if (ctx->weightmap.comp == 3)
			arr[3] = 255;
		return ret;
	}

	// get the diagonal corners
	uint8_t *h00 = &ctx->weightmap.pixels[gy * (ctx->weightmap.w * ctx->weightmap.comp) + (gx * ctx->weightmap.comp)];
	uint8_t *h10 = &ctx->weightmap.pixels[gy * (ctx->weightmap.w * ctx->weightmap.comp) + ((gx + 1) * ctx->weightmap.comp)];
	uint8_t *h01 = &ctx->weightmap.pixels[(gy + 1) * (ctx->weightmap.w * ctx->weightmap.comp) + (gx * ctx->weightmap.comp)];
	uint8_t *h11 = &ctx->weightmap.pixels[(gy + 1) * (ctx->weightmap.w * ctx->weightmap.comp) + ((gx + 1) * ctx->weightmap.comp)];

	fx = (fx * 16) - floorf(fx * 16);
	fy = (fy * 16) - floorf(fy * 16);

	for (int i = 0; i < ctx->weightmap.comp; i++)
		arr[i] = interpolate(h00[i], h10[i], h01[i], h11[i], fx, fy);

	if (ctx->weightmap.comp == 3)
		arr[3] = 255;

	return ret;
}

static uint8_t get_light(context_t *ctx, int sx, int sy, float fx, float fy)
{
	if (!ctx->lightmap.pixels)
		return 255;

	// scale to grid coordinates
	int gx = (sx * 16) + floorf(fx * 16);
	int gy = (sy * 16) + floorf(fy * 16);

	if (!ctx->config.output.interpolate_lightmap)
		return ctx->lightmap.pixels[gy * ctx->lightmap.w + gx];

	// get the diagonal corners
	uint8_t h00 = ctx->lightmap.pixels[gy * ctx->lightmap.w + gx];
	uint8_t h10 = ctx->lightmap.pixels[gy * ctx->lightmap.w + (gx + 1)];
	uint8_t h01 = ctx->lightmap.pixels[(gy + 1) * ctx->lightmap.w + gx];
	uint8_t h11 = ctx->lightmap.pixels[(gy + 1) * ctx->lightmap.w + (gx + 1)];

	fx = (fx * 16) - floorf(fx * 16);
	fy = (fy * 16) - floorf(fy * 16);

	return interpolate(h00, h10, h01, h11, fx, fy);
}

static float get_height(context_t *ctx, int sx, int sy, float fx, float fy)
{
	// scale to grid coordinates
	int gx = (sx * 16) + floorf(fx * 16);
	int gy = (sy * 16) + floorf(fy * 16);

	// get the diagonal corners
	uint16_t h00 = ctx->heightmap.pixels[gy * ctx->heightmap.w + gx];
	uint16_t h10 = ctx->heightmap.pixels[gy * ctx->heightmap.w + (gx + 1)];
	uint16_t h01 = ctx->heightmap.pixels[(gy + 1) * ctx->heightmap.w + gx];
	uint16_t h11 = ctx->heightmap.pixels[(gy + 1) * ctx->heightmap.w + (gx + 1)];

	fx = (fx * 16) - floorf(fx * 16);
	fy = (fy * 16) - floorf(fy * 16);

	return interpolate(h00, h10, h01, h11, fx, fy);
}

static void WriteU16LE(FILE *fp, uint16_t value)
{
	value = LittleShort(value);
	fwrite(&value, sizeof(uint16_t), 1, fp);
}

static void WriteU32LE(FILE *fp, uint32_t value)
{
	value = LittleLong(value);
	fwrite(&value, sizeof(uint32_t), 1, fp);
}

static void WriteU8(FILE *fp, uint8_t value)
{
	fwrite(&value, sizeof(uint8_t), 1, fp);
}

static void write_block(context_t *ctx, int bx, int by)
{
	char filename[2048];
	char filepath[2048];
	char xbx[3];
	char xby[3];
	uint32_t offsets[16][16];
	FILE *fp;

	Q_snprintfz(xbx, sizeof(xbx), "%02x", (uint8_t)bx);
	Q_snprintfz(xby, sizeof(xby), "%02x", (uint8_t)by);

	Q_snprintfz(filepath, sizeof(filepath), "%s/%s", ctx->config.output.path, ctx->config.output.name);
	Q_snprintfz(filename, sizeof(filename), "%s/%s/block_%s_%s.hms", ctx->config.output.path, ctx->config.output.name, xbx, xby);

	fp = fopen(filename, "wb");
	if (!fp)
	{
		fprintf(stderr, "failed to open \"%s\" for writing\n", filename);
		return;
	}

	fprintf(stderr, "writing \"%s\"\n", filename);

	// clear offsets
	Q_memset(offsets, 0, sizeof(offsets));

	// magic
	fwrite("HMMS", 4, 1, fp);
	// version
	WriteU32LE(fp, 1 | 0x80000000);
	// section offsets
	for (int y = 0; y < 16; y++)
		for (int x = 0; x < 16; x++)
			WriteU32LE(fp, offsets[y][x]);
	// sections
	for (int sy = 0; sy < 16; sy++)
	{
		for (int sx = 0; sx < 16; sx++)
		{
			fprintf(stderr, "%s: writing section %d %d\n", filename, sx, sy);
			offsets[sy][sx] = ftell(fp);
			// flags
			WriteU32LE(fp, 0);
			// textures
			for (int i = 0; i < 4; i++)
				fwrite(ctx->config.input.ground[i], sizeof(ctx->config.input.ground[i]), 1, fp);
			// texture samples
			for (int y = 0; y < 64; y++)
			{
				for (int x = 0; x < 64; x++)
				{
					int xofs = ((bx + ctx->num_blocks_ew / 2) * 16) + sx;
					int yofs = ((by + ctx->num_blocks_ns / 2) * 16) + sy;
					float lightval = get_light(ctx, xofs, yofs, (float)x / 64, (float)y / 64);

					WriteU8(fp, 0);
					WriteU8(fp, 0);
					WriteU8(fp, 0);
					WriteU8(fp, (uint8_t)lightval);
				}
			}
			// height samples
			for (int y = 0; y < 17; y++)
			{
				for (int x = 0; x < 17; x++)
				{
					int xofs = ((((bx + ctx->num_blocks_ew / 2) * 16) + sx) * 16) + x;
					int yofs = ((((by + ctx->num_blocks_ns / 2) * 16) + sy) * 16) + y;

					float heightval = ctx->heightmap.pixels[yofs * ctx->heightmap.w + xofs];

					heightval *= ctx->config.output.yscale;

					// fprintf(stderr, "%d %d: %f\n", xofs, yofs, heightval);

					WriteU32LE(fp, *(uint32_t *)&heightval);
				}
			}
			// holes
			WriteU16LE(fp, 0);
			// reserved
			WriteU16LE(fp, 0);
			// water height
			WriteU32LE(fp, 0);
			// min height
			WriteU32LE(fp, 0);
			// max height
			WriteU32LE(fp, 0);
			// reserved
			for (int i = 0; i < 4; i++)
				WriteU32LE(fp, 0);
		}
	}
	// section offsets
	fseek(fp, 8, SEEK_SET);
	for (int y = 0; y < 16; y++)
		for (int x = 0; x < 16; x++)
			WriteU32LE(fp, offsets[y][x]);

	fclose(fp);
}

int main(int argc, char **argv)
{
	int num_exported = 0;

	for (int arg = 1; arg < argc; arg++)
	{
		int err;
		context_t *ctx = calloc(1, sizeof(context_t));

		// load config
		if ((err = ini_parse(argv[arg], config_handler, &ctx->config)) != 0)
		{
			if (err == -1)
				fprintf(stderr, "%s: failed to open\n", argv[arg]);
			else if (err == -2)
				fprintf(stderr, "%s: failed to allocate memory\n", argv[arg]);
			else
				fprintf(stderr, "%s: error on line %d\n", argv[arg], err);
			goto next;
		}

		// load images
		ctx->heightmap.pixels = stbi_load_16(ctx->config.input.heightmap, &ctx->heightmap.w, &ctx->heightmap.h, &ctx->heightmap.comp, 1);
		if (!ctx->heightmap.pixels)
		{
			fprintf(stderr, "%s: failed to open heightmap \"%s\": %s\n", argv[arg], ctx->config.input.heightmap, stbi_failure_reason());
			goto next;
		}

		if (*ctx->config.input.weightmap)
			ctx->weightmap.pixels = stbi_load(ctx->config.input.weightmap, &ctx->weightmap.w, &ctx->weightmap.h, &ctx->weightmap.comp, 4);
		if (*ctx->config.input.lightmap)
			ctx->lightmap.pixels = stbi_load(ctx->config.input.lightmap, &ctx->lightmap.w, &ctx->lightmap.h, &ctx->lightmap.comp, 1);

		// east->west and north->south
		ctx->num_blocks_ew = ctx->heightmap.w / 16 / 16;
		ctx->num_blocks_ns = ctx->heightmap.h / 16 / 16;

		fprintf(stderr, "%s: %dx%d blocks (%dx%d sections)\n", argv[arg], ctx->num_blocks_ew, ctx->num_blocks_ns, ctx->num_blocks_ew * 16, ctx->num_blocks_ns * 16);

		for (int by = 0 - ctx->num_blocks_ns / 2; by < ctx->num_blocks_ns / 2; by++)
		{
			for (int bx = 0 - ctx->num_blocks_ew / 2; bx < ctx->num_blocks_ew / 2; bx++)
			{
				write_block(ctx, bx, by);
			}
		}

		num_exported++;

next:
		if (ctx)
		{
			if (ctx->heightmap.pixels) STBI_FREE(ctx->heightmap.pixels);
			if (ctx->weightmap.pixels) STBI_FREE(ctx->weightmap.pixels);
			if (ctx->lightmap.pixels) STBI_FREE(ctx->lightmap.pixels);

			free(ctx);
		}
	}

	fprintf(stderr, "exported %d maps\n", num_exported);

	return 0;
}
