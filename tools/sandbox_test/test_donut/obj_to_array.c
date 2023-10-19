#include <stdio.h>
#include <math.h>
#include <string.h>

#define MAXVTS 8192

int main( int argc, char ** argv )
{
	if( argc != 2 )
	{
		fprintf( stderr, "Usage: [obj_to_array] name\n" );
		return -5;
	}

	char objname[1024];
	snprintf( objname, sizeof( objname ), "%s.obj", argv[1] );
	char hname[1024];
	snprintf( hname, sizeof( hname ), "%s.h", argv[1] );
	
	FILE * f = fopen( objname, "r" );
	char buffer[1024];
	char * line;
	int mode = 1;
	FILE * bh = fopen( hname, "w" );

	float minB[3] = { 1e20, 1e20, 1e20 };
	float maxB[3] = {-1e20,-1e20,-1e20 };

	float maxextent = -1e20;

	int iAliasedVert[MAXVTS];
	float fvVerts[MAXVTS][6];
	int fvc = 0;

	int ivS[MAXVTS][3];
	int ivc = 0;

	while( line = fgets( buffer, sizeof(buffer)-1, f ) )
	{
		if( line[0] == 'v' )
		{
			float fv[6];
			sscanf( line + 2, "%f %f %f %f %f %f", fv+0, fv+1, fv+2, fv+3, fv+4, fv+5 );

			if( fv[0] < minB[0] ) minB[0] = fv[0];
			if( fv[0] > maxB[0] ) maxB[0] = fv[0];

			if( fv[1] < minB[1] ) minB[1] = fv[1];
			if( fv[1] > maxB[1] ) maxB[1] = fv[1];

			if( fv[2] < minB[2] ) minB[2] = fv[2];
			if( fv[2] > maxB[2] ) maxB[2] = fv[2];

			memcpy( fvVerts[fvc++], fv, sizeof( fv ) );
		}
		if( line[0] == 'f' )
		{
			int vv[3];
			sscanf( line + 2, "%d %d %d", vv+0, vv+1, vv+2 );
			// 1 indexed.
			vv[0]--;
			vv[1]--;
			vv[2]--;
			memcpy( ivS[ivc++], vv, sizeof( vv ) );
		}
	}

	float compverts[MAXVTS][3];
	int cvct = 0;
	int i, j;
	for( i = 0; i < fvc; i++ )
	{
		float * fvc = fvVerts[i];
		for( j = 0; j < cvct; j++ )
		{
			float dx = fvc[0] - compverts[j][0];
			float dy = fvc[1] - compverts[j][1];
			float dz = fvc[2] - compverts[j][2];
			float diff = sqrtf( dx*dx + dy*dy + dz*dz );
			if( diff < 0.0001 )
			{
				break;
			}
		}
		if( j == cvct )
		{
			// New one!
			memcpy( compverts[cvct++], fvc, sizeof( float ) * 3 );
		}
		iAliasedVert[i] = j;
	}

	printf( "Unique Vertices: %d from %d\n", cvct, fvc );

	if( -minB[0] > maxextent ) maxextent = -minB[0];
	if( -minB[1] > maxextent ) maxextent = -minB[1];
	if( -minB[2] > maxextent ) maxextent = -minB[2];
	if(  maxB[0] > maxextent ) maxextent =  maxB[0];
	if(  maxB[1] > maxextent ) maxextent =  maxB[1];
	if(  maxB[2] > maxextent ) maxextent =  maxB[2];

	printf( "BB: [%f %f %f] [%f %f %f] ME %f\n", minB[0], minB[1], minB[2], maxB[0], maxB[1], maxB[2], maxextent );
	printf( "Tris: %d\n", ivc );
	float scale = 255.9 / maxextent;

	int ivR[MAXVTS][4];

	for( i = 0; i < ivc; i++ )
	{
		int i0 = ivS[i][0];
		int i1 = ivS[i][1];
		int i2 = ivS[i][2];

		float * color = &fvVerts[i0][3];
		int fc = ((int)(color[0] * 6.9)) + ((int)(color[1] * 6.9)) * 6 + ((int)(color[2] * 6.9)) * 36;
		int * face = ivR[i];
		face[0] = iAliasedVert[i0];
		face[1] = iAliasedVert[i1];
		face[2] = iAliasedVert[i2];
		face[3] = fc;
	}

	fprintf( bh, "#include <stdint.h>\nint8_t %s_verts[] = {\n", argv[1] );
	for( i = 0; i < cvct; i++ )
	{
		fprintf( bh, "\t%d, %d, %d,\n", (int)(compverts[i][0]*scale), (int)(compverts[i][1]*scale), (int)(compverts[i][2]*scale) );		
	}
	fprintf( bh, "};\nuint8_t %s_lines[] = {\n", argv[1] );
	for( i = 0; i < ivc; i++ )
	{
		fprintf( bh, "\t%d, %d, %d, %d,\n", ivR[i][0], ivR[i][1], ivR[i][2], ivR[i][3] );
	}
	fprintf( bh, "};\n" );

	return 0;
}

