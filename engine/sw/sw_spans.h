/*
this file is expected to be #included as the body of a real function
to define create a new pixel shader, define PLOT_PIXEL(outval) at the top of your function and you're good to go

//modifiers:
SPAN_ST - interpolates S+T across the span. access with 'sc' and 'tc'
		affine... no perspective correction.


*/

{
	swvert_t *vt;
	int y;
	int secondhalf;

	//l=value on left
	//ld=change per y (on left)
	//d=change per x
	int xl,xld, xr,xrd;
#ifdef SPAN_ST
	int sl,sld, sd;
	int tl,tld, td;
#endif
#ifdef SPAN_ZI
	int zil, zild, zid;
#endif
#ifdef SPAN_Z
	int zl,zld, zd;
#endif
	unsigned int *restrict outbuf;
	unsigned int *restrict ti;
	int i;
	const swvert_t *vlt,*vlb,*vrt,*vrb;
	int spanlen;
	int numspans;
	unsigned int *vplout;
	int dx, dy;
	int recalcside;
	int interlace;

	float fdx1,fdy1,fdx2,fdy2,fz,d1,d2;

	if (!img)
		return;

	/*we basically render a diamond
	that is, the single triangle is split into two triangles, outwards towards the midpoint and inwards to the final position.
	*/

	/*reorder the verticies for height*/
	if (v1->scoord[1] > v2->scoord[1])
	{
		vt = v1;
		v1 = v2;
		v2 = vt;
	}
	if (v1->scoord[1] > v3->scoord[1])
	{
		vt = v1;
		v1 = v3;
		v3 = vt;
	}
	if (v2->scoord[1] > v3->scoord[1])
	{
		vt = v3;
		v3 = v2;
		v2 = vt;
	}

	{
		const swvert_t *v[3];

		v[0] = v1;
		v[1] = v2;
		v[2] = v3;

		//reject triangles with any point offscreen, for now
		for (i = 0; i < 3; i++)
		{
			if (v[i]->scoord[0] < 0 || v[i]->scoord[0] > th->vpwidth)
				return;
			if (v[i]->scoord[1] < 0 || v[i]->scoord[1] > th->vpheight)
				return;
			if (v[i]->zicoord < 0)
				return;
		}

		for (i = 0; i < 2; i++)
		{
			if (v[i]->scoord[1] > v[i+1]->scoord[1])
				return;
		}
	}

	fdx1 = v2->scoord[0] - v1->scoord[0];
	fdy1 = v2->scoord[1] - v1->scoord[1];

	fdx2 = v3->scoord[0] - v1->scoord[0];
	fdy2 = v3->scoord[1] - v1->scoord[1];

	fz = fdx1*fdy2 - fdx2*fdy1;

	if (fz == 0)
	{
		//weird angle...
		return;
	}

	fz = 1.0 / fz;
	fdx1 *= fz;
	fdy1 *= fz;
	fdx2 *= fz;
	fdy2 *= fz;

#ifdef SPAN_ST	//affine
	d1 = (v2->tccoord[0] - v1->tccoord[0])*(img->pwidth<<16);
	d2 = (v3->tccoord[0] - v1->tccoord[0])*(img->pwidth<<16);
	sld = fdx1*d2 - fdx2*d1;
	sd = fdy2*d1 - fdy1*d2;

	d1 = (v2->tccoord[1] - v1->tccoord[1])*(img->pheight<<16);
	d2 = (v3->tccoord[1] - v1->tccoord[1])*(img->pheight<<16);
	tld = fdx1*d2 - fdx2*d1;
	td = fdy2*d1 - fdy1*d2;
#endif
#ifdef SPAN_ZI
	d1 = (1<<16);
	d2 = (1<<16);
	zild = 0;//fdx1*d2 - fdx2*d1;
	zid = 0;//fdy2*d1 - fdy1*d2;
#endif
#ifdef SPAN_Z
	d1 = (v2->zicoord - v1->zicoord)*(1<<16);
	d2 = (v3->zicoord - v1->zicoord)*(1<<16);
	zld = fdx1*d2 - fdx2*d1;
	zd = fdy2*d1 - fdy1*d2;
#endif

	ti = img->data;

	y = v1->scoord[1];

	for (secondhalf = 0; secondhalf <= 1; secondhalf++)
	{
		if (secondhalf)
		{
		//	return;
			if (numspans < 0)
			{
				interlace = -numspans;
				y+=interlace;
				numspans-=interlace;

				xl += xld*interlace;
				xr += xrd*interlace;
				vplout += th->vpcstride*interlace;

#ifdef SPAN_ST
				sl += sld*interlace;
				tl += tld*interlace;
#endif
#ifdef SPAN_ZI
				zil += zild*interlace;
#endif
#ifdef SPAN_Z
				zl += zld*interlace;
#endif
			}

			/*v2->v3*/
			if (fz <= 0)
			{
				vlt = v2;
				//vrt == v1;
				vlb = v3;
				//vrb == v3;

				recalcside = 1;

#ifdef SPAN_ST
				sld -= (((long long)sd*xld)>>16);
				tld -= (((long long)td*xld)>>16);
#endif
#ifdef SPAN_ZI
				zild -= (((long long)zid*xld)>>16);
#endif
#ifdef SPAN_Z
				zld -= (((long long)zd*xld)>>16);
#endif
			}
			else
			{
				//vlt == v1;
				vrt = v2;
				///vlb == v3;
				vrb = v3;

				recalcside = 2;
			}

			//flip the triangle to keep it facing the screen (we swapped the verts almost randomly)
			numspans = v3->scoord[1] - y;
		}
		else
		{
			vlt = v1;
			vrt = v1;
			/*v1->v2*/
			if (fz < 0)
			{
				vlb = v2;
				vrb = v3;
			}
			else
			{
				vlb = v3;
				vrb = v2;
			}
			recalcside = 3;

			//flip the triangle to keep it facing the screen (we swapped the verts almost randomly)
			numspans = v2->scoord[1] - y;
		}

		if (recalcside & 1)
		{
			dx = (vlb->scoord[0] - vlt->scoord[0]);
			dy = (vlb->scoord[1] - vlt->scoord[1]);
			if (dy > 0)
				xld = (dx<<16) / dy;
			else
				xld = 0;
			xl = (int)vlt->scoord[0]<<16;

#ifdef SPAN_ST
			sl = vlt->tccoord[0] * (img->pwidth<<16);
			sld = sld + (((long long)sd*xld+32767)>>16);
			tl = vlt->tccoord[1] * (img->pheight<<16);
			tld = tld + (((long long)td*xld+32767)>>16);
#endif
#ifdef SPAN_ZI
			zil = (1<<16);///vlt->zicoord;
			zild = zild + (((long long)zid*xld)>>16);
#endif
#ifdef SPAN_Z
			zl = vlt->zicoord * (1<<16);
			zld = zld + (((long long)zd*xld)>>16);
#endif
		}

		if (recalcside & 2)
		{
			dx = (vrb->scoord[0] - vrt->scoord[0]);
			dy = (vrb->scoord[1] - vrt->scoord[1]);
			if (dy)
				xrd = (dx<<16) / dy;
			else
				xrd = 0;
			xr = (int)vrt->scoord[0]<<16;
		}

		if (y + numspans > th->vpheight)
			numspans = th->vpheight - y;

		if (numspans <= 0)
			continue;


		vplout = th->vpcbuf + y * th->vpcstride;	//this is a pointer to the left of the viewport buffer.

		interlace = ((y + th->interlaceline) % th->interlacemod);
		if (interlace)
		{
			if (interlace > numspans)
			{
				interlace = numspans;
				y+=interlace;
			}
			else
			{
				y+=interlace;
				numspans-=interlace;
			}
			xl += xld*interlace;
			xr += xrd*interlace;
			vplout += th->vpcstride*interlace;

#ifdef SPAN_ST
			sl += sld*interlace;
			tl += tld*interlace;
#endif
#ifdef SPAN_ZI
			zil += zild*interlace;
#endif
#ifdef SPAN_Z
			zl += zld*interlace;
#endif
		}

		for (; numspans > 0;
			numspans -= th->interlacemod
			,xl += xld*th->interlacemod
			,xr += xrd*th->interlacemod
			,vplout += th->vpcstride*th->interlacemod
			,y += th->interlacemod

#ifdef SPAN_ST
			,sl += sld*th->interlacemod
			,tl += tld*th->interlacemod
#endif
#ifdef SPAN_ZI
			,zil += zild*th->interlacemod
#endif
#ifdef SPAN_Z
			,zl += zld*th->interlacemod
#endif
			)
		{
#ifdef SPAN_ST
			unsigned int s = sl;
			unsigned int t = tl;
#endif
#ifdef SPAN_ZI
			unsigned int zi = zil;
#else
			const unsigned int zi = (1<<16);
#endif
#ifdef SPAN_Z
			unsigned int z = zl;
			unsigned int *restrict zb = th->vpdbuf + y * th->vpwidth + (xl>>16);
#endif

			spanlen = (xr - xl)>>16;
			outbuf = vplout + (xl>>16);

			while(spanlen-->0)
			{
				PLOT_PIXEL(*outbuf);
				outbuf++;

#ifdef SPAN_ST
				s += sd;
				t += td;
#endif
#ifdef SPAN_ZI
				zi += zid;
#endif
#ifdef SPAN_Z
				z += zd;
				zb++;
#endif
			}
		}
	}
}

#undef SPAN_Z
#undef SPAN_ZI
#undef SPAN_ST
#undef PLOT_PIXEL
