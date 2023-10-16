#include <stdio.h>

int main()
{
	FILE * f = fopen( "bunny_out.obj", "r" );
	char buffer[1024];
	float scale = 280000;
	char * line;
	int mode = 1;
	FILE * bh = fopen( "bunny.h", "w" );
	fprintf( bh, "#include <stdint.h>\nint16_t bunny_verts[] = {\n" );
	while( line = fgets( buffer, sizeof(buffer)-1, f ) )
	{
		if( line[0] == 'v' )
		{
			float x, y, z;
			sscanf( line + 2, "%f %f %f", &x, &y, &z );
			fprintf( bh, "\t%d, %d, %d,\n", (int)(x*scale), (int)(y*scale), (int)(z*scale) );
		}
		if( line[0] == 'l' )
		{
			if( mode == 1 )
			{
				mode = 2;
				fprintf( bh, "};\nuint8_t bunny_lines[] = {\n" );
			}

			int x, y;
			sscanf( line + 2, "%d %d", &x, &y );
			fprintf( bh, "\t%d, %d,\n", x-1, y-1 );
		}
	}
	fprintf( bh, "};\n" );

}

