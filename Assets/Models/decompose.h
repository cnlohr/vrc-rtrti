#ifndef _DECOMPOSE_H
#define _DECOMPOSE_H

#define MAXTAPS 4

static void HandleMip( int miplo, float intensity, float fx, float fy, float * summap )
{
	// Size, in pixels of a sample
	int pxsize = 1<<miplo;
	float localintensity = intensity;// / (pxsize*pxsize);
	
	// Need to scale intensity by mip inverse.
	int px_in_mip = (1<<((miplo)*2));
	localintensity /= px_in_mip;
	
	if( fx < pxsize/2 ) fx = pxsize/2;
	if( fy < pxsize/2 ) fy = pxsize/2;
	if( fx > MSIZEW-pxsize/2 ) fx = MSIZEW-pxsize/2;
	if( fy > MSIZEH-pxsize/2 ) fy = MSIZEH-pxsize/2;
	
	int lxlo = fx / pxsize - 0.5;
	int lxhi = fx / pxsize + 0.5;
	int lylo = fy / pxsize - 0.5;
	int lyhi = fy / pxsize + 0.5;
	float dfhix = fx / pxsize - lxlo - 0.5;
	float dfhiy = fy / pxsize - lylo - 0.5;
	float dflox = 1.0-dfhix;
	float dfloy = 1.0-dfhiy;
	
	lxlo *= pxsize;
	lylo *= pxsize;
	lxhi *= pxsize;
	lyhi *= pxsize;
	
	//printf(" %d %d %d %d %f %d\n", lxlo, lxhi, lylo, lyhi, intensity, miplo );
	
	int x, y;
	for( y = 0; y < pxsize; y++ )
	for( x = 0; x < pxsize; x++ )
	{
		int lxo = lxlo + x;
		int lyo = lylo + y;
		summap[lxo+lyo*MSIZEW] += localintensity * dflox * dfloy;
		lxo = lxhi + x;
		lyo = lylo + y;
		summap[lxo+lyo*MSIZEW] += localintensity * dfhix * dfloy;
		lxo = lxlo + x;
		lyo = lyhi + y;
		summap[lxo+lyo*MSIZEW] += localintensity * dflox * dfhiy;
		lxo = lxhi + x;
		lyo = lyhi + y;
		summap[lxo+lyo*MSIZEW] += localintensity * dfhix * dfhiy;
	}
}

static float CalculateTestFromTaps( float * testtaps, const float * tomatch, float * summap, float * errmapSIGN, float * errmap )
{
	// Returns sum error, not RMS.
	
	int x, y;
	for( y = 0; y < MSIZEH; y++ )
	{
		for( x = 0; x < MSIZEW; x++ )
		{
			summap[y*MSIZEW+x] = 0;
		}
	}
	
	//Now, we have to "sample" the taps
	int i;
	for( i = 0; i < MAXTAPS; i++ )
	{	
		float * tt = testtaps + 4*i;
		// Correct faults.
		if( tt[3] < 0 ) tt[3] = 0;
		if( tt[2] < 0 ) tt[2] = 0;
		if( tt[2] > MIPLEVELS ) tt[2] = MIPLEVELS;
		if( tt[0] < 0 ) tt[0] = 0;
		if( tt[1] < 0 ) tt[1] = 0;
		if( tt[0] > 1 ) tt[0] = 1;
		if( tt[1] > 1 ) tt[1] = 1;
		float mip = tt[2];
		int miplo = (int)mip;
		int miphi = miplo+1;
		float mip_porportion_hi = mip - miplo;
		float mip_porportion_lo = 1.0 - mip_porportion_hi;
		
		float cx = tt[0] * (MSIZEW-1);
		float cy = tt[1] * (MSIZEH-1);
		// First low-mip (highest res)
		HandleMip( miplo, mip_porportion_lo * tt[3], cx, cy, summap );
		HandleMip( miphi, mip_porportion_hi * tt[3], cx, cy, summap );
		
		// Need to pick both levels.
		//XXX PICK UP HERE
	}

	float sumerror = 0;
	for( y = 0; y < MSIZEH; y++ )
	{
		for( x = 0; x < MSIZEW; x++ )
		{
			float diff = tomatch[y*MSIZEW+x] - summap[y*MSIZEW+x];

			errmapSIGN[y*MSIZEW+x] = diff;

			if( diff < 0 ) diff = -diff;
			sumerror += diff;
			errmap[y*MSIZEW+x] = diff;
		}
	}

	return sumerror;
}


static void GenFromErrMap( float * TargTaps, int mode, float * errmapSIGN )
{
	int x, y;
	float avgx = 0;
	float avgy = 0;
	float sum = 0;
	float sum_square = 0;
	float fas = 0;
	float higheste = 0;
	float hex, hey;
	float count = 0;
	for( y = 0; y < MSIZEH; y++ )
	{
		for( x = 0; x < MSIZEW; x++ )
		//x = MSIZEW/2;
		{
			float m = errmapSIGN[y*MSIZEW+x];
			float absm = m;
			if( absm < 0 ) absm = -absm;
			avgx += x*absm;
			avgy += y*absm;
			sum += m;
			sum_square += m*m;
			fas += absm;
			count++;
			if( absm > higheste )
			{
				higheste = absm;
				hex = x; hey = y;
			}
		}
	}
	printf( "AX AY %f %f\n", avgx, avgy );
	avgx /= fas;
	avgy /= fas;
	avgx /= (MSIZEW-1);
	avgy /= (MSIZEH-1);
	hex /= (MSIZEW-1); 
	hey /= (MSIZEH-1);
	
	float target_mip = 0;
	if( mode == 1 )
	{
		// Use highest point.
		avgx = hex;
		avgy = hey;
		sum *= .666;
	}
	else
	{
		sum *= .666;
	}

	{
		float stddevx = 0;
		float stddevy = 0;
		for( y = 0; y < MSIZEH; y++ )
		{
			for( x = 0; x < MSIZEW; x++ )
			//x = MSIZEW/2;
			{
				float m = errmapSIGN[y*MSIZEW+x];
				float absm = m;
				if( absm < 0 ) absm = -absm;
				float lx = x / (float)(MSIZEW-1);
				float ly = y / (float)(MSIZEH-1);
				stddevx += ( (lx-avgx)*(lx-avgx)*m*m );
				stddevy += ( (ly-avgy)*(ly-avgy)*m*m );
			}
		}
		stddevx /= sum_square;
		stddevy /= sum_square;
		//stddevx = sqrtf( stddevx );
		//stddevy = sqrtf( stddevy );
		float avgdev = ( stddevx + stddevy ) / 2;
		target_mip = MIPLEVELS +1.7818+ ( log( avgdev ) / 0.69314718 )/2; //ln(2)
		
		if( mode == 1 )
		{
			target_mip-=0.5;
		}
		printf( "%f %f %f/%f [%f] %f\n", avgx, avgy, stddevx, stddevy, target_mip, sum );
	}
	
	
	TargTaps[0] = avgx;
	TargTaps[1] = avgy;
	TargTaps[2] = target_mip;
	TargTaps[3] = sum; // Don't actually do full sum for initial guess.
}



static float rrnd()
{
	return ((rand()%10000)-4999.5)/5000.;
}

static float absf( float f )
{
	return (f<0)?-f:f;
}


static void RunCeleste( int group, float * tomatch, float * summap, float * errmapSIGN, float * errmap, uint32_t * calout, float * SetTAPS, float * SetError )
{
	int j;
	float errorporp = 0.1;
	float TestTAPS[4*MAXTAPS];
	int y, x;
	for( j = 0; j < 100; j++ )
	{
		memcpy( TestTAPS, SetTAPS, sizeof(TestTAPS) );
		
		float err = CalculateTestFromTaps( TestTAPS, tomatch, summap, errmapSIGN, errmap );
		for( y = 0; y < MSIZEH; y++ )
		{
			for( x = 0; x < MSIZEW; x++ )
			{
				float tm = summap[y*MSIZEW+x];
				int r = tm*255;
				if( r < 0 ) r = 0; if( r > 255 ) r = 255;
				calout[y*MSIZEW+x] = 0xff | (r<<24)| (r<<16)|(r<<8);
			}
		}
		
		
		CNFGBlitImage( calout, 256, 0, MSIZEW, MSIZEH );

		float intenjump1 = errorporp;
		TestTAPS[0+group*4] += intenjump1*rrnd()*.5;
		TestTAPS[1+group*4] += intenjump1*rrnd()*.5;
		TestTAPS[2+group*4] += intenjump1*rrnd()*(MIPLEVELS/2.0);
		TestTAPS[3+group*4] += intenjump1*rrnd()*.5;

		err = CalculateTestFromTaps( TestTAPS, tomatch, summap, errmapSIGN, errmap );
		if( err < *SetError )
		{
			memcpy( SetTAPS, TestTAPS, sizeof( TestTAPS ) );
			*SetError = err;
		}
		
		errorporp *= .99;
	}
}



void PerformDecompose( float * SetTAPS, float * tomatch, float * SetError )
{
	int x, y;
	uint32_t calout[MSIZEW*MSIZEH]; // Visualization
	float summap[MSIZEW*MSIZEH];
	float errmap[MSIZEW*MSIZEH];
	float errmapSIGN[MSIZEW*MSIZEH];

	SetTAPS[0] = MSIZEW/2.0;
	SetTAPS[1] = MSIZEH/2.0;
	SetTAPS[2] = 0.0;
	SetTAPS[3] = 0;
	SetTAPS[4] = MSIZEW/2.0;
	SetTAPS[5] = MSIZEH/2.0;
	SetTAPS[6] = 0.0;
	SetTAPS[7] = 0;
	SetTAPS[8] = MSIZEW/2.0;
	SetTAPS[9] = MSIZEH/2.0;
	SetTAPS[10] = 0.0;
	SetTAPS[11] = 0;
	SetTAPS[12] = MSIZEW/2.0;
	SetTAPS[13] = MSIZEH/2.0;
	SetTAPS[14] = 0.0;
	SetTAPS[15] = 0;


	// First

	CalculateTestFromTaps( SetTAPS, tomatch, summap, errmapSIGN, errmap );
	GenFromErrMap( SetTAPS, 0, errmapSIGN );
	CalculateTestFromTaps( SetTAPS, tomatch, summap, errmapSIGN, errmap );
	GenFromErrMap( SetTAPS + 4, 1, errmapSIGN );
	CalculateTestFromTaps( SetTAPS, tomatch, summap, errmapSIGN, errmap );
	GenFromErrMap( SetTAPS + 8, 1, errmapSIGN );
	CalculateTestFromTaps( SetTAPS, tomatch, summap, errmapSIGN, errmap );
	GenFromErrMap( SetTAPS + 12, 1, errmapSIGN );
	CalculateTestFromTaps( SetTAPS, tomatch, summap, errmapSIGN, errmap );

	RunCeleste( 0, tomatch, summap, errmapSIGN, errmap, calout, SetTAPS, SetError );
	RunCeleste( 1, tomatch, summap, errmapSIGN, errmap, calout, SetTAPS, SetError );
	RunCeleste( 2, tomatch, summap, errmapSIGN, errmap, calout, SetTAPS, SetError );
	RunCeleste( 3, tomatch, summap, errmapSIGN, errmap, calout, SetTAPS, SetError );

	float errorsum = 0;
	float totalsum = 0;
	for( y = 0; y < MSIZEH; y++ )
	{
		for( x = 0; x < MSIZEW; x++ )
		{
			float m = errmapSIGN[y*MSIZEW+x];
			float e = summap[y*MSIZEW+x];
			totalsum += e;
			errorsum += m;
		}
	}
	float gmod = 1 + (errorsum/totalsum);
	printf( "ES: %f / %f / %f\n", errorsum, totalsum, gmod );
	SetTAPS[0+3] *= gmod;
	SetTAPS[4+3] *= gmod;
	SetTAPS[8+3] *= gmod;
	SetTAPS[12+3] *= gmod;
	CalculateTestFromTaps( SetTAPS, tomatch, summap, errmapSIGN, errmap );
	errorsum = 0;
	totalsum = 0;
	for( y = 0; y < MSIZEH; y++ )
	{
		for( x = 0; x < MSIZEW; x++ )
		{
			float m = errmapSIGN[y*MSIZEW+x];
			float e = summap[y*MSIZEW+x];
			totalsum += e;
			errorsum += m;
		}
	}
	printf( "ES: %f / %f\n", errorsum, totalsum );

}

#endif