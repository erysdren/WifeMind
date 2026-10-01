#include "image.h"

#ifdef IMAGEFMT_PBM

static int PBM_ParseNum(qbyte **buf, qbyte *end)
{
	qbyte token[256];
	size_t l;
	while (*buf < end)
	{
		if (**buf <= ' ')
			(*buf)++;
		else if (**buf == '#')
		{
			while (*buf < end && **buf != '\n' && **buf != '\r')
				(*buf)++;
		}
		else
			break;
	}

	for (l = 0; *buf < end && **buf > ' ';)
		if (l < countof(token))
			token[l++] = *(*buf)++;
	token[l] = 0;
	return strtol(token, NULL, 0);
}

qbyte *ReadPBMFile(qbyte *buf, size_t len, const char *fname, int *width, int *height, uploadfmt_t *format)
{	//this isn't expected to be fast.
	qbyte *end = buf+len;
	int maxval = *width = *height = 0;
	qbyte *r, *bo;
	unsigned short *so;
	size_t l, x, y;
	float m, *fo, *fi;
	int c = buf[1];
	buf+=2;
	switch(c)
	{
	case '7': //arbitrary
		//WIDTH HEIGHT DEPTH MAXVAL TUPLTYPE ENDHDR
		return NULL;
	case '6':	//raw ppm
	case '3':	//plain ppm
		*width = PBM_ParseNum(&buf, end);
		*height = PBM_ParseNum(&buf, end);
		maxval = PBM_ParseNum(&buf, end);

		if (maxval > 255)
		{
			r = BZ_Malloc(*width**height*8);
			for(y = 0; y < *height; y++)
			for(x = 0, so=(unsigned short*)r+(*height-y-1)*4**width; x < *height; x++)
			{
				*so++ = (65535u*PBM_ParseNum(&buf, end))/maxval;
				*so++ = (65535u*PBM_ParseNum(&buf, end))/maxval;
				*so++ = (65535u*PBM_ParseNum(&buf, end))/maxval;
				*so++ = 65535u;
			}
			*format = PTI_RGBA16;
		}
		else
		{
			r = BZ_Malloc(*width**height*4);
			for(y = 0; y < *height; y++)
			for(x = 0, bo=(qbyte*)r+(*height-y-1)*4**width; x < *height; x++)
			{
				*bo++ = (255u*PBM_ParseNum(&buf, end))/maxval;
				*bo++ = (255u*PBM_ParseNum(&buf, end))/maxval;
				*bo++ = (255u*PBM_ParseNum(&buf, end))/maxval;
				*bo++ = 255u;
			}
			*format = PTI_RGBA8;
		}
		return r;
	case '5':	//raw pgm
	case '2':	//plain pgm
	case '4':	//raw pbm
	case '1':	//plain pbm
		*width = PBM_ParseNum(&buf, end);
		*height = PBM_ParseNum(&buf, end);

		if (c == '4' || c == '1')
			maxval = 1;
		else
			maxval = PBM_ParseNum(&buf, end);

		l = (size_t)*width*(size_t)*height;
		if (maxval > 255)
		{
			r = BZ_Malloc(*width**height*sizeof(*so));
			for(y = 0; y < *height; y++)
			for(x = 0, so=(unsigned short*)r+(*height-y-1)**width; x < *height; x++)
				*so++ = (65535u*PBM_ParseNum(&buf, end))/maxval;
			*format = PTI_R16;
		}
		else
		{
			r = BZ_Malloc(*width**height*sizeof(*bo));
			for(y = 0; y < *height; y++)
			for(x = 0, bo=(qbyte*)r+(*height-y-1)**width; x < *height; x++)
				*bo++ = (255u*PBM_ParseNum(&buf, end))/maxval;
			*format = PTI_R8;
		}
		return r;

	case 'F':	//rgb pfm
	case 'f':	//grey pfm
		*width = PBM_ParseNum(&buf, end);
		*height = PBM_ParseNum(&buf, end);
		m = PBM_ParseNum(&buf, end);

		if (*buf == '\n')
			buf++;
		fi = (float*)buf;

		l = (size_t)*width*(size_t)*height;
		if ((qbyte*)(fi+l*((c=='F')?3:1)) != end)
			return NULL;
		r = BZ_Malloc(l*sizeof(float) * ((c=='F')?4:1));
		if (c == 'F')
		{
			if (m < 0)
			{
				r = BZ_Malloc(*width**height*4*sizeof(float));
				for(y = 0; y < *height; y++)
				for(x = 0, fo=(float*)r+(*height-y-1)*4**width; x < *height; x++)
				{
					*fo++ = LittleFloat(*fi++);
					*fo++ = LittleFloat(*fi++);
					*fo++ = LittleFloat(*fi++);
					*fo++ = 1;
				}
			}
			else
			{
				r = BZ_Malloc(*width**height*4*sizeof(float));
				for(y = 0; y < *height; y++)
				for(x = 0, fo=(float*)r+(*height-y-1)*4**width; x < *height; x++)
				{
					*fo++ = BigFloat(*fi++);
					*fo++ = BigFloat(*fi++);
					*fo++ = BigFloat(*fi++);
					*fo++ = 1;
				}
			}
			*format = PTI_RGBA32F;
		}
		else
		{
			r = BZ_Malloc(l*sizeof(float));
			if (m < 0)
			{
				r = BZ_Malloc(*width**height*sizeof(float));
				for(y = 0; y < *height; y++)
				for(x = 0, fo=(float*)r+(*height-y-1)**width; x < *height; x++)
					*fo++ = LittleFloat(*fi++);
			}
			else
			{
				r = BZ_Malloc(*width**height*sizeof(float));
				for(y = 0; y < *height; y++)
				for(x = 0, fo=(float*)r+(*height-y-1)**width; x < *height; x++)
					*fo++ = BigFloat(*fi++);
			}
			*format = PTI_R32F;
		}
		return r; //erk?
	}
	return NULL;
}

#endif // IMAGEFMT_PBM
