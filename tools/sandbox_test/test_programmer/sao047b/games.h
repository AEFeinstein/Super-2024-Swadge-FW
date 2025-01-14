#ifndef _GAMES_H
#define _GAMES_H

int totalScore;

uint32_t gameTimeUs;

int buttonMask;
int buttonEventDown;
int buttonEventUp;

void DoneGame( int score );

typedef int (*gamemode_t)();

uint8_t gameData[1024];

gamemode_t gameMode;

int GameModeGrand()
{
	struct
	{
		int stage;
		uint32_t endMark;
		uint32_t switchTime;
	} * g = (void*)gameData;

	glyphdraw_invert = -1;
	glyphdraw_nomask = 1;

	uint32_t tsTime = gameTimeUs - g->switchTime;

	switch (g->stage)
	{
	case 0:
		background(11);
		swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, "A GRAND" );
		swadgeDraw( SSD1306_W/2, 16, 1, swadgeGlyph, "%d", (gameTimeUs>>12) );
		if( buttonEventDown || (gameTimeUs>>12) >= 2000 )
		{
			g->switchTime = gameTimeUs;
			g->endMark = ((gameTimeUs>>12) >= 2000) ? 2000 : (gameTimeUs>>12);
			g->stage = 1;
		}
		break;
	case 1:
	{
		// Flash
		background(10);
		glyphdraw_invert = 0;
		glyphdraw_nomask = 1;
		int substage = (tsTime>>21);
		int shownum = g->endMark;
		int showdiff = 1000 - shownum;
		int score = 1000 - ((showdiff<0)?-showdiff:showdiff);
		if( score < 0 ) score = 0;
		if( substage == 1 )
		{
			shownum = showdiff;
		}
		else if( substage == 2 )
		{
			shownum = score;
		}
		else if( substage > 2 )
		{
			totalScore += score;
			return score;
		}
		swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, (const char*[]){"A GRAND","DIFFERENCE","SCORE"}[substage] );
		swadgeDraw( SSD1306_W/2, 16, 1, swadgeGlyph, "%d", shownum );
	}
	}

	return 0;
} 

int GameModeMainMenu()
{
	struct
	{
		int submode;
		uint32_t switchTime;
	} * g = (void*)gameData;

	background( (const int[]){ 0, 9, 9, 9, 7 }[g->submode] );

	uint32_t modeDT = gameTimeUs - g->switchTime;

	switch( g->submode )
	{
	case 0:
		swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, "RINGBOX" );
		swadgeDraw( 16, 16, 1, swadgeGlyphHalf, "GO" );
		swadgeDraw( 56, 16, 1, swadgeGlyphHalf, "SHO" );
		break;
	case 4:
		if( modeDT > 1500000 )
			return 1;
		swadgeDraw( 150 - (modeDT>>12), 8, 0, swadgeGlyph, "LETS GOOOOOOOOOOOO" );
		break;
	}

	if( g->submode != 5 && (buttonEventDown & 2 ) )
	{
		g->submode = (g->submode+1)&3;
	}

	if( buttonEventDown & 1 )
	{
		g->switchTime = gameTimeUs;
		g->submode = 4;
	}
	
	return 0;
}

int GameModeEnding()
{
	background(8);
	swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, "FINAL" );

	int pxup = gameTimeUs >> 16;
	int pxupo = pxup;
	if( pxup > 24 ) pxupo = 24;
	swadgeDraw( SSD1306_W/2, pxupo, 1, swadgeGlyph, "%d", totalScore );

	if( pxup > 64 )
	{
		// TODO: Check max score.
		return 1;
	}


	return 0;
} 


gamemode_t gameModes[] = {
	GameModeGrand,
};

#endif

