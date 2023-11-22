// DO NOT INCLUDE THIS IN THE NORMAL BUILD PROCESS
//  THIS IS ONLY USED IN UNUSUAL SITUATIONS.

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#include "cnovrsupport.h"

//Used for vertex welding.
#define EPSILON  1
#define FIXEDSCALE 64 // was 48

#define MAXV (1048576)
#define MAXI (1048576*3)

int lineify = 0;

int16_t vertices[MAXV*3];
int vcount;

int tindexset[MAXI];
int ticount; //tris*3 in a,b,c

int lindexset[MAXI];
int licount; //lines*2 in a,b

int tispartof[MAXI];
int lispartof[MAXI];

int tempv[MAXI];
int temp[MAXI];

int emitbytes;

int16_t VertexDist( const int16_t * a, const int16_t * b )
{
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];
    dx *= dx;
    dy *= dy;
    dz *= dz;
    return (int16_t)sqrt( dx+dy+dz );
}

    //IDX is he index to the array of indices.
void GrowGroup( int idx, int stride, int groupno, int depth )
{
    int id = (idx / stride) * stride;
    int i;

    for( i = 0; i < stride; i++ )
    {
        if( stride == 3 )
        {
            if( tispartof[i+id] ) continue;
            tispartof[i+id] = groupno;
            int thisid = tindexset[i+id];
            //This triangle has not yet been marked.
            //fprintf( stderr, "    ASSIGNED %d TO GROUP %d  (INDEX # %d)\n", i + id, groupno, thisid );
            int j;
            for( j = 0; j < ticount; j++ )
            {
                if( tispartof[j] ) continue;
                //fprintf( stderr, "TIP: %d == %d\n", thisid, tindexset[j] );
                if( tindexset[j] == thisid )
                {
                    GrowGroup( j, stride, groupno, depth+1 );
                }
            }
        }
        else if( stride == 2 )
        {
            if( lispartof[i+id] ) continue;
            lispartof[i+id] = groupno;
            int thisid = lindexset[i+id];
            //This line has not yet been marked.

            int j;
            for( j = 0; j < licount; j++ )
            {
                if( lispartof[j] ) continue;
                if( lindexset[j] == thisid )
                {
                    GrowGroup( j, stride, groupno, depth+1 );
                }
            }
        }
    }
}



void Write16u( uint16_t u )
{
	printf( "0x%04x,%s", u, ((emitbytes+=2)&0x1f)?" ":"\n\t" );
 
//    printf( "0x%02x, 0x%02x, ", (uint8_t)(u & 0xff), (uint8_t)(u >> 8) );
//    emitbytes+=2;
//    putchar( u & 0xff );
//    putchar( u >> 8 );
}

void Write16s( int16_t s )
{
	printf( "0x%04x,%s", (uint16_t)s, ((emitbytes+=2)&0x1f)?" ":"\n\t" );

//    printf( "%02x %02x ", (uint8_t)(s & 0xff), (uint8_t)(s >> 8) );
//    emitbytes+=2;
//    putchar( s & 0xff );
//    putchar( s >> 8 );
}


int main( int argc, char ** argv )
{
    int i;
	printf( "\t" ); //First tab for formatting.

    if( argc != 2 )
    {
        fprintf( stderr, "Usage: objProcessor [obj file]\n" );
        return -5;
    }

    if( strstr( argv[1], "lines" ) ) lineify = 1;

    int filelen = 0;
    char * file = CNOVRFileToString( argv[1], &filelen );
    if( !file || filelen == 0 )
    {
        fprintf( stderr, "Error opening file: %s\n", argv[1] );
        return -6;
    }
    char ** splits = CNOVRSplitStrings( file, "\n", "\r", 1, 0 );
    char * sl;
    int lineno = 0;
    char * currento __attribute__((unused)) = 0;
    int * grouptags = malloc( sizeof(int)*2 );
    int groupno = 0;

    while( ( sl = splits[lineno++] ) )
    {
        switch( sl[0] )
        {
        case 'o':
            if( sl[1] )
                currento = sl + 2;
            groupno++;
            grouptags = realloc( grouptags, (groupno+1)*sizeof(int) );
            grouptags[groupno] = 0;
            char * st = strstr( sl+1, "LABEL" );
            if( st )
            {
                grouptags[groupno] = atoi( st + 5 );
            }
            fprintf( stderr, "GROUP: %s %p %d\n", sl+1, st, grouptags[groupno] );
            break;
            //if( strstr( currento, "lines" ) )
            //    lineify = 1;
            //else
            //    lineify = 0;
            //break;
        case 'l':
            if( 1 )
            {

                int l1, l2;
                int lc = sscanf( sl+1, "%d %d", &l1, &l2 );
                if( lc == 2 )
                {
                    lispartof[licount] = groupno;
                    lindexset[licount++] = l1-1; if( licount >= MAXI ) goto overflow;
                    lispartof[licount] = groupno;
                    lindexset[licount++] = l2-1; if( licount >= MAXI ) goto overflow;
                }
                else
                {
                    fprintf( stderr, "Error on line %d: Invalid # of line coordinates\n", lineno );
                }
            }
            break;
        case 'f':
            //"f 3082/3795/934 3080/3796/934 3081/3797/934 3083/3798/934"
            {
                int ct;
                char ** vmaps = CNOVRSplitStrings( sl+1, " ", "", 1, &ct );
                int tfaces[ct];

                for( i = 0; i < ct; i++ )
                {
                    char * fs = vmaps[i];
                    tfaces[i] = atoi( fs )-1;
                }

                //Overrides verts, turns into lines.
                if( lineify )
                {
                    int segs = ct;
                    for( i = 0; i < segs; i++ )
                    {
                        lispartof[licount] = groupno;
                        lindexset[licount++] = tfaces[i]; if( licount >= MAXI ) goto overflow;
                        lispartof[licount] = groupno;
                        lindexset[licount++] = tfaces[(i+1)%segs]; if( licount >= MAXI ) goto overflow;
                    }
                }
                else
                {
                    //Auto triangluate.
                    int tris = ct - 2;
                    for( i = 0; i < tris; i++ )
                    {
                        tispartof[ticount] = groupno;
                        tindexset[ticount++] = tfaces[0]; if( ticount >= MAXI ) goto overflow;
                        tispartof[ticount] = groupno;
                        tindexset[ticount++] = tfaces[(i+1)%ct]; if( ticount >= MAXI ) goto overflow;
                        tispartof[ticount] = groupno;
                        tindexset[ticount++] = tfaces[(i+2)%ct]; if( ticount >= MAXI ) goto overflow;
                    }
                }
            }
            break;
        case 'v':
            if( sl[1] == ' ' )
            {
                //If vt or vn don't worry.
                float vp[3];
                if( vcount > MAXV ) goto overflow;
                int vc = sscanf( sl+2, "%f %f %f", vp+0, vp+1, vp+2 );
                if( vc != 3 ) 
                {
                    fprintf( stderr, "Error on line %d: Invalid # of vertex coordinates\n", lineno );
                }
                int k;
                for( k = 0; k < 3; k++ ) vertices[vcount*3+k] = vp[k]*FIXEDSCALE;
                vcount++;
                break;
            }
        default:
            break;
            //Don't care. 
        }
        //fprintf( stderr, "%d: %s\n", lineno, sl );
    }
    fprintf( stderr, "Loaded %d tri indices and %d line indices with %d vertices\n", ticount, licount, vcount );

    //If we want to ignore existing groups and math it all ourselves.
    int advanced_process = 0;
    if( advanced_process )
    {
        //This process only supports triangles.
        memset( tispartof, 0, sizeof(tispartof) );
        memset( lispartof, 0, sizeof(lispartof) );

        //Step 1: Merge common vertices.
        if( 1 )
        {
            int mergedct = 0;

            uint8_t * vnixed = alloca( vcount );

            for( i = 0; i < vcount; i++ )
            {
                int j;
                int16_t * vm = &vertices[i*3];
                if( vnixed[i] ) continue;
                for( j = i+1;j < vcount; j++ )
                {
                    int16_t * vt = &vertices[j*3];
                    if( vnixed[j] ) continue;

                    int16_t vdist = VertexDist( vm, vt );
                    if( vdist <= EPSILON )
                    {
                        //Weld these vertices.
                        fprintf( stderr, "Merging %d<%d %d %d> with %d<%d %d %d> dist: %d\n", i, vm[0], vm[1], vm[2], j, vt[0], vt[1], vt[2], vdist );
                        vnixed[j] = 1;
                        int k;
                        for( k = 0; k < ticount; k++ )
                            if( tindexset[k] == j ) tindexset[k] = i;
                        for( k = 0; k < licount; k++ )
                            if( lindexset[k] == j ) lindexset[k] = i;
                        mergedct++;
                    }
                }
            }
            fprintf( stderr, "Merged %d vertices\n", mergedct );
        }

        //for( i = 0; i < ticount; i++ )
        //    printf( "%d ", tindexset[i] );
        //Step 2: Nix unused vertices.
        if( 1 ) {
            int nixedct = 0;
            for( i = 0; i < vcount; i++ )
            {
                int uses = 0;
                int k;
                for( k = 0; k < ticount; k++ )
                    if( tindexset[k] == i ) uses++;
                for( k = 0; k < licount; k++ )
                    if( lindexset[k] == i ) uses++;
                if( uses ) continue;

                fprintf( stderr, "Vertex: %d <%d %d %d> nixing\n", i+nixedct, vertices[i*3+0], vertices[i*3+1], vertices[i*3+2] );

                int j;
                for( j = i; j < vcount; j++ )
                {
                    vertices[j*3+0] = vertices[j*3+3];
                    vertices[j*3+1] = vertices[j*3+4];
                    vertices[j*3+2] = vertices[j*3+5];
                }
                for( k = 0; k < ticount; k++ )
                    if( tindexset[k] >= i ) tindexset[k]--;
                for( k = 0; k < licount; k++ )
                    if( lindexset[k] >= i ) lindexset[k]--;
                nixedct++;
                vcount--;
                i--;
            }
            fprintf( stderr, "Nixed %d vertices\n", nixedct );
        }

        groupno = 0;
        //Groupify pieces.
        if( 1 )
        {
            for( i = 0; i < ticount; i+=3 )
            {
                if( tispartof[i] == 0 )
                {
                    groupno++;
                    GrowGroup( i, 3, groupno, 0 );
                }
                //Else, otherwise claimed.
            }
            for( i = 0; i < licount; i++ )
            {
                if( lispartof[i] == 0 )
                {
                    groupno++;
                    GrowGroup( i, 2, groupno, 0 );
                }
                //Else, otherwise claimed.
            }
        }
        fprintf( stderr, "Groups: %d (%d/%d)\n", groupno, ticount, licount );
    }
    else
    {
        //*not* advanced_process
    }


    //Recommended we leave this on as it validates the grouping code.
    {
        int ogroupoffset = 1;
        //Output demo OBJ
        FILE * f = fopen( "ProcOBJ.obj", "wb" );
        for( i = 1; i < groupno; i++ )
        {
            fprintf( f, "o Group%d {%d}\n", i, grouptags[i] );
            int j;
            memset( tempv, 0, sizeof( tempv ) );
            for( j = 0; j < ticount; j += 3 )
            {
                if( tispartof[j] == i ) 
                {
                    tempv[tindexset[j+0]] = 1;
                    tempv[tindexset[j+1]] = 1;
                    tempv[tindexset[j+2]] = 1;
                }
            }

            for( j = 0; j < licount; j += 2 )
            {
                if( lispartof[j] == i ) 
                {
                    tempv[lindexset[j+0]] = 1;
                    tempv[lindexset[j+1]] = 1;
                }
            }

            for( j = 0; j < vcount; j++ )
            {
                if( tempv[j] )
                {
                    temp[j] = ogroupoffset++;
                    fprintf( f, "v %f %f %f\n", vertices[j*3+0]/((float)FIXEDSCALE), vertices[j*3+1]/((float)FIXEDSCALE), vertices[j*3+2]/((float)FIXEDSCALE) );
                }
            }
            for( j = 0; j < licount; j += 2 )
            {
                if( lispartof[j] != lispartof[j+1] )
                {
                    fprintf( stderr, "ERROR: GROUPING CODE JANKY LINE\n" );
                }
                if( lispartof[j] == i )
                {
                    fprintf( f, "l %d %d\n", temp[lindexset[j+0]], temp[lindexset[j+1]] );
                }
            }
            for( j = 0; j < ticount; j += 3 )
            {
                if( tispartof[j] != tispartof[j+1] || tispartof[j] != tispartof[j+2] )
                {
                    fprintf( stderr, "ERROR: GROUPING CODE JANKYTRIANGLES [%d@%d %d %d]\n", j, tispartof[j+0],tispartof[j+1],tispartof[j+2] );
                }
                if( tispartof[j] == i )
                {
                    fprintf( f, "f %d %d %d\n", temp[tindexset[j+0]], temp[tindexset[j+1]], temp[tindexset[j+2]] );
                }
            }
        }
        fprintf( stderr, "Wrote checkfile\n" );
    }

    int absminx = 1000000;
    int absminy = 1000000;
    int absminz = 1000000;
    int absmaxx = -1000000;
    int absmaxy = -1000000;
    int absmaxz = -1000000;

    Write16u( 'i' | ('m'<<8) );
    Write16u( 'd' | ('l'<<8) );
    Write16u( groupno );


    if( lineify )
    {
        int outverts = 0;
        int outtris = 0;
        int comptotal __attribute__((unused)) = 0;
        int g;
        for( g = 1; g <= groupno; g++ )
        {
            int j;
            int lvindex = 0;
            memset( tempv, 0, sizeof( tempv ) );
            for( j = 0; j < licount; j += 2 )
            {
                if( lispartof[j] == g ) 
                {
                    tempv[tindexset[j+0]] = 1;
                    tempv[tindexset[j+1]] = 1;
                }
            }
            int minx = 32767;
            int miny = 32767;
            int minz = 32767;
            int maxx = -32768;
            int maxy = -32768;
            int maxz = -32768;
            for( j = 0; j < vcount; j++ )
            {
                if( tempv[j] )
                {
                    int x = vertices[j*3+0];
                    int y = vertices[j*3+1];
                    int z = vertices[j*3+2];
                    if( x < minx ) minx = x;
                    if( y < miny ) miny = y;
                    if( z < minz ) minz = z;
                    if( x > maxx ) maxx = x;
                    if( y > maxy ) maxy = y;
                    if( z > maxz ) maxz = z;
                    temp[j] = lvindex++;
                }
            }
            if( minx < absminx ) absminx = minx;
            if( miny < absminy ) absminy = miny;
            if( minz < absminz ) absminz = minz;
            if( maxx > absmaxx ) absmaxx = maxx;
            if( maxy > absmaxy ) absmaxy = maxy;
            if( maxz > absmaxz ) absmaxz = maxz;

            int centerx = (maxx-minx)/2;
            int centery = (maxy-miny)/2;
            int centerz = (maxz-minz)/2;
            int radius = 0;
            for( j = 0; j < vcount; j++ )
            {
                if( tempv[j] )
                {
                    int x = vertices[j*3+0]*FIXEDSCALE;
                    int y = vertices[j*3+1]*FIXEDSCALE;
                    int z = vertices[j*3+2]*FIXEDSCALE;
                    int dx = x - centerx;
                    int dy = y - centery;
                    int dz = z - centerz;
                    int r = sqrt( (dx * dx + dy * dy + dz * dz) / ( FIXEDSCALE * FIXEDSCALE ) );
                    if( r > radius ) radius = r;
                }
            }

            int faces = 0;
            for( j = 0; j < licount; j += 2 )
            {
                if( lispartof[j] == g )
                    faces++;
            }

            int stride = 2;
            Write16u( lvindex*stride ); //# of vertx ints count
            Write16u( faces );
            Write16u( 2 ); //3 = triangles. NOTE: MSB is 0 or version.
            Write16s( centerx );
            Write16s( centery );
            Write16s( centerz );
            Write16u( radius );
            Write16u( grouptags[g] );

            fprintf( stderr, "Group %d: %d/%d  (%d,%d,%d) %d\n", g-1, lvindex*stride, faces, centerx, centery, centerz, radius );

            uint16_t storebuffer[lvindex*3+faces*3];
            int storebufferid = 0;


            for( j = 0; j < ticount; j += 2 )
            {
                if( lispartof[j] == g )
                {
                    Write16u( storebuffer[storebufferid++] = temp[tindexset[j+0]]*stride );
                    Write16u( storebuffer[storebufferid++] = temp[tindexset[j+1]]*stride );
                    outtris ++;
                }
            }

            for( j = 0; j < vcount; j++ )
            {
                if( tempv[j] )
                {
                    Write16s( (int)(storebuffer[storebufferid++] = vertices[j*3+0]) );
                    Write16s( (int)(storebuffer[storebufferid++] = vertices[j*3+1]) );
                    Write16s( (int)(storebuffer[storebufferid++] = vertices[j*3+2]) );
                    outverts++;
                }
            }

            //Consider: Decompress the descriptors in RAM, and then decompress the needed models, as-needed.
            //uint8_t * compoutput = alloca(lvindex*4+faces*4+66);
            //int comp = fastlz_compress_level( 2, storebuffer, storebufferid * 2, compoutput);
            //comptotal += comp;
        }
        fprintf( stderr, "Emitted %d groups, %d indices, %d vertices\n", groupno, outtris, outverts );
    }
    else
    {
        int outverts = 0;
        int outtris = 0;
        int outlines = 0;
        int comptotal __attribute__((unused)) = 0;
        int g;
        for( g = 1; g <= groupno; g++ )
        {
            int j;
            int lvindex = 0;

            memset( tempv, 0, sizeof( tempv ) );
            for( j = 0; j < ticount; j +=3 )
            {
                if( tispartof[j] == g ) 
                {
                    tempv[tindexset[j+0]] = 1;
                    tempv[tindexset[j+1]] = 1;
                    tempv[tindexset[j+2]] = 1;
                }
            }


            for( j = 0; j < licount; j +=2 )
            {
                if( lispartof[j] == g )
                {
                    tempv[lindexset[j+0]] = 2;
                    tempv[lindexset[j+1]] = 2;
                }
            }

            int minx = 32767;
            int miny = 32767;
            int minz = 32767;
            int maxx = -32768;
            int maxy = -32768;
            int maxz = -32768;
            for( j = 0; j < vcount; j++ )
            {
                if( tempv[j] == 1 )
                {
                    int x = vertices[j*3+0];
                    int y = vertices[j*3+1];
                    int z = vertices[j*3+2];
                    if( x < minx ) minx = x;
                    if( y < miny ) miny = y;
                    if( z < minz ) minz = z;
                    if( x > maxx ) maxx = x;
                    if( y > maxy ) maxy = y;
                    if( z > maxz ) maxz = z;
                    //fprintf( stderr, "Triangle: %d %d\n", j, lvindex );
                    temp[j] = lvindex++;
                }
            }

            for( j = 0; j < vcount; j++ )
            {
                if( tempv[j] == 2 )
                {
                    int x = vertices[j*3+0];
                    int y = vertices[j*3+1];
                    int z = vertices[j*3+2];
                    if( x < minx ) minx = x;
                    if( y < miny ) miny = y;
                    if( z < minz ) minz = z;
                    if( x > maxx ) maxx = x;
                    if( y > maxy ) maxy = y;
                    if( z > maxz ) maxz = z;
                    //fprintf( stderr, "Line: %d %d\n", j, lvindex );
                    temp[j] = lvindex++;
                }
            }
            if( minx < absminx ) absminx = minx;
            if( miny < absminy ) absminy = miny;
            if( minz < absminz ) absminz = minz;
            if( maxx > absmaxx ) absmaxx = maxx;
            if( maxy > absmaxy ) absmaxy = maxy;
            if( maxz > absmaxz ) absmaxz = maxz;

            int centerx = (maxx+minx)/2;
            int centery = (maxy+miny)/2;
            int centerz = (maxz+minz)/2;
            int radius = 0;
            for( j = 0; j < vcount; j++ )
            {
                if( tempv[j] )
                {
                    int x = vertices[j*3+0];
                    int y = vertices[j*3+1];
                    int z = vertices[j*3+2];
                    int dx = x - centerx;
                    int dy = y - centery;
                    int dz = z - centerz;
                    int r = sqrt( (dx * dx + dy * dy + dz * dz) );
                    if( r > radius ) radius = r;
                }
            }

            int faces = 0;
            for( j = 0; j < ticount; j += 3 )
            {
                if( tispartof[j] == g )
                    faces++;
            }

            int lines = 0;
            for( j = 0; j < licount; j += 2 )
            {
                if( lispartof[j] == g )
                    lines++;
            }

            if( faces > lines )
            {
//                int stride = 3;
                Write16u( lvindex*3 ); //# of vertx ints count
                Write16u( faces );
                Write16u( 3 ); //3 = triangles. NOTE: MSB is 0 or version.
                Write16s( centerx );
                Write16s( centery );
                Write16s( centerz );
                Write16u( radius );
                Write16u( grouptags[g] );
            //    fprintf( stderr, "T--->%d -> %d -> %d\n", g, radius, grouptags[g] );


                uint16_t storebuffer[lvindex*3+faces*3];
                int storebufferid = 0;


                for( j = 0; j < ticount; j += 3 )
                {
                    if( tispartof[j] == g )
                    {
                        Write16u( storebuffer[storebufferid++] = temp[tindexset[j+0]]*3 );
                        Write16u( storebuffer[storebufferid++] = temp[tindexset[j+1]]*3 );
                        Write16u( storebuffer[storebufferid++] = temp[tindexset[j+2]]*3 );
                        outtris ++;
                    }
                }

                for( j = 0; j < vcount; j++ )
                {
                    if( tempv[j] )
                    {
                        //fprintf( stderr, "%d,%d,%d\n", (int)vertices[j*3+0], (int)vertices[j*3+1], (int)vertices[j*3+2] );
                        Write16s( (int)(storebuffer[storebufferid++] = vertices[j*3+0]) );
                        Write16s( (int)(storebuffer[storebufferid++] = vertices[j*3+1]) );
                        Write16s( (int)(storebuffer[storebufferid++] = vertices[j*3+2]) );
                        outverts++;
                    }
                }
                //fprintf( stderr, "Group T %d: %d/%d  (%d,%d,%d) %d\n", g-1, lvindex*stride, faces, centerx, centery, centerz, radius );
            }
            else
            {
//                int stride = 2;
                Write16u( lvindex*3 ); //# of vertx ints count
                Write16u( lines );
                Write16u( 2 ); //3 = triangles. NOTE: MSB is 0 or version.
                Write16s( centerx );
                Write16s( centery );
                Write16s( centerz );
                Write16u( radius );
                Write16u( grouptags[g] );
            //    fprintf( stderr, "L--->%d -> %d -> %d\n", g, radius, grouptags[g] );

                uint16_t storebuffer[lvindex*3+lines*2];
                int storebufferid = 0;

                int thislines = 0;
                int thisvs = 0;

                for( j = 0; j < licount; j += 2 )
                {
                    if( lispartof[j] == g )
                    {
                        //fprintf( stderr, "%d,%d -> %d,%d\n", lindexset[j+0], lindexset[j+1], temp[lindexset[j+0]]*3, temp[lindexset[j+1]]*3 );
                        Write16u( storebuffer[storebufferid++] = temp[lindexset[j+0]]*3 );
                        Write16u( storebuffer[storebufferid++] = temp[lindexset[j+1]]*3 );
                        outlines ++;
                        thislines++;
                    }
                }

                for( j = 0; j < vcount; j++ )
                {
                    if( tempv[j] )
                    {
                        //fprintf( stderr, "%d,%d,%d\n", (int)vertices[j*3+0], (int)vertices[j*3+1], (int)vertices[j*3+2] );
                        Write16s( (int)(storebuffer[storebufferid++] = vertices[j*3+0]) );
                        Write16s( (int)(storebuffer[storebufferid++] = vertices[j*3+1]) );
                        Write16s( (int)(storebuffer[storebufferid++] = vertices[j*3+2]) );
                        outverts++;
                        thisvs++;
                    }
                }
                //fprintf( stderr, "Group L %d: %d/%d  (%d,%d,%d) %d (%d-%d) (%d-%d)\n", g-1, lvindex*stride, faces, centerx, centery, centerz, radius, lines, thislines, lvindex, thisvs );
            }


            //Consider: Decompress the descriptors in RAM, and then decompress the needed models, as-needed.
            //uint8_t * compoutput = alloca(lvindex*4+faces*4+66);
            //int comp = fastlz_compress_level( 2, storebuffer, storebufferid * 2, compoutput);
            //comptotal += comp;
        }
        fprintf( stderr, "Emitted %d groups, %d indices, %d lines, %d vertices\n", groupno, outtris, outlines, outverts );
    }

    fprintf( stderr, "Overall extents: [%d to %d] [%d to %d] [%d to %d]\n",
        absminx, absmaxx, absminy, absmaxy, absminz, absmaxz );

    {
        //Now, the scary stuff... Occlusion and occlusion sorting,maybe?
    }

    fprintf( stderr, "Emitted %d bytes\n", emitbytes );

    //printf( "%d: %s %s\n", argc, argv[1], argv[2] );
    return 0;
overflow:
    fprintf( stderr, "ERROR: element overflow on line %d\n", lineno );
    return -5;
}

