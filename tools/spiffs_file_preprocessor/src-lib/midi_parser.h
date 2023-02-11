#ifndef __LIB_MIDI_PARSER_HEADER_
#define __LIB_MIDI_PARSER_HEADER_

#include <stdbool.h>

typedef enum {
	MidiSequenceNumber,
	MidiTextEvent,
	MidiNewLyric,
	MidiNewMarker,
	MidiNewCuePoint,
	MidiNewChannelPrefix,
	MidiPortChange,
	MidiTempoChanged,
	MidiSMTPEOffset,
	MidiNewTimeSignature,
	MidiNewKeySignature,
	MidiSequencerSpecificEvent,
	MidiNoteReleased,
	MidiNotePressed,
	MidiPolyphonicPressure,
	MidiControllerValueChanged,
	MidiProgramChanged,
	MidiPressureOfChannelChanged,
	MidiPitchBendChanged,
} EventType;

typedef	struct {
	EventType	type;
	int		timeToAppear;
	void		*infos;
} Event;

typedef	struct {
	unsigned char	numerator;
	unsigned char	denominator;
	unsigned char	clockTicksPerMetTick;
	unsigned char	ticksPerQuarterNote;
} MidiTimeSignature;

typedef struct {
	unsigned char	channel;
	unsigned char	pitch;
	unsigned char	velocity;
} MidiNote;

typedef struct	Note {
	unsigned	char	pitch;
	unsigned	char	channel;
	unsigned	char	velocity;
	unsigned	char	fadeOutVelocity;
	unsigned long	int	timeBeforeAppear;
	unsigned long	int	duration;
} Note;

typedef struct {
	char		*copyright;
	char		*name;
	char		*instrumentName;
	int		nbOfEvents;
	int		nbOfNotes;
	Note		*notes;
	Event		*events;
} Track;

typedef struct	NoteList {
	Note			*note;
	struct	NoteList	*next;
	struct	NoteList	*prev;
} NoteList;

typedef struct {
	unsigned short	format;
	short		nbOfTracks;
	char		fps;
	short		ticks;
	int		nbOfNotes;
	Track		*tracks;
} MidiParser;

bool		parseMidiTrack(unsigned char *buffer, int buffLen, Track *track, bool outputDebug, MidiParser *result, int posInFile, bool createNoteArray);
MidiParser	*parseMidi(const char *path, bool outputDebug, bool createNoteArray);
char		*getNoteString(char note);
void		deleteTrack(Track *track);
void		deleteMidiParserStruct(MidiParser *result);

#endif