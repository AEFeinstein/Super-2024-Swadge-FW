#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "midi_parser.h"

bool	addNode(NoteList *list, Note *data)
{
	if (!data)
		return (false);
	for (; list->note && list->next; list = list->next);
	if (list->note) {
		list->next = malloc(sizeof(*list->next));
		if (!list->next) {
			printf("Error: Cannot alloc %iB\n", (int)sizeof(*list->next));
			return (false);
		}
		list->next->prev = list;
		list->next->next = NULL;
		list = list->next;
	}
	list->note = data;
	return (true);
}

void	deleteNode(NoteList *node)
{
	if (node->prev) {
		node->prev->next = node->next;
		if (node->next)
			node->next->prev = node->prev;
		free(node);
	} else if (node->next) {
		node->note = node->next->note;
		node = node->next;
		node->prev->next = node->next;
		if (node->next)
			node->next->prev = node->prev;
		free(node);
	} else
		node->note = NULL;
}

char	*getNoteString(char note)
{
	static char	buffer[5];
	int		nbr;

	memset(buffer, 0, sizeof(buffer));
	switch (note % 12) {
	case 0:
		buffer[0] = 'C';
		nbr = 1;
		break;
	case 1:
		buffer[0] = 'C';
		buffer[1] = '#';
		nbr = 2;
		break;
	case 2:
		buffer[0] = 'D';
		nbr = 1;
		break;
	case 3:
		buffer[0] = 'D';
		buffer[1] = '#';
		nbr = 2;
		break;
	case 4:
		buffer[0] = 'E';
		nbr = 1;
		break;
	case 5:
		buffer[0] = 'F';
		nbr = 1;
		break;
	case 6:
		buffer[0] = 'F';
		buffer[1] = '#';
		nbr = 2;
		break;
	case 7:
		buffer[0] = 'G';
		nbr = 1;
		break;
	case 8:
		buffer[0] = 'G';
		buffer[1] = '#';
		nbr = 2;
		break;
	case 9:
		buffer[0] = 'A';
		nbr = 1;
		break;
	case 10:
		buffer[0] = 'A';
		buffer[1] = '#';
		nbr = 2;
		break;
	case 11:
		buffer[0] = 'B';
		nbr = 1;
		break;
	}
	sprintf(buffer + nbr, "%i", note / 12 - 1);
	return (buffer);
}

void	deleteTrack(Track *track)
{
	free(track->copyright);
	free(track->name);
	free(track->instrumentName);
	for (int i = 0; i < track->nbOfEvents && track->events; i++)
		free(track->events[i].infos);
	free(track->notes);
	free(track->events);
}

void	deleteMidiParserStruct(MidiParser *result)
{
	for (int i = 0; i < result->nbOfTracks; i++)
		deleteTrack(&result->tracks[i]);
	free(result->tracks);
}

int	readVarLenInt(int fd, int *pos)
{
	char	buff = 0;
	int	result = 0;
	int	count = 0;

	do {
		if (count++ == 4 || read(fd, &buff, 1) <= 0) {
			*pos = -1;
			return (0);
		}
		result = (result << 7) + (buff & 0x7F);
		(*pos)++;
	} while (buff & 0x80);
	return (result);
}

char	*readString(int fd, int len)
{
	char	*buffer = malloc(len + 1);

	if (!buffer) {
		printf("Error: Couldn't alloc %iB\n", len + 1);
		return (NULL);
	} else if (read(fd, buffer, len) != len) {
		free(buffer);
		return (NULL);
	}
	buffer[len] = 0;
	return (buffer);
}

unsigned char	readSingleByte(int fd, int *i)
{
	char	byte;

	if (read(fd, &byte, 1) == 1) {
		(*i)++;
		return (byte);
	}
	*i = -1;
	return (0);
}

void	showChunk(unsigned char *buffer, int pos, int len, int posInFile)
{
	int	realPos = pos - 1;

	posInFile = pos > 15 ? posInFile - 15 : posInFile - pos;
	pos = pos > 15 ? pos - 15 : 0;
	for (int j = 0; j < 10 - (posInFile % 10); j++)
		printf("     ");
	for (int i = 10 - posInFile % 10; i < 40 && pos + i < len; i += 10) {
		for (int j = printf("%i", posInFile + i); j < 50; j++)
			printf(" ");
	}
	printf("\n");
	for (int i = 0; i < 40 && pos + i < len; i++)
		printf(i + pos == realPos ? "  V  " : ((i + posInFile) % 10 ? "  '  " : "  |  "));
	printf("\n");
	for (int i = 0; i < 40 && pos + i < len; i++)
		for (int j = printf(buffer[i + pos] ? " %#x" : " 0x0", buffer[i + pos]); j < 5; j++)
			printf(" ");
	printf("\n");
}

bool	parseMidiTrack(unsigned char *buffer, int buffLen, Track *track, bool outputDebug, MidiParser *result, int posInFile, bool createNoteArray)
{
	void		*buff;
	unsigned char	byte;
	unsigned char	statusByte;
	unsigned int	deltaTime = 0;
	unsigned long	totalTime = 0;
	unsigned int	len;
	NoteList	list = {NULL, NULL, NULL};
	NoteList	*node;
	Note		*noteBuffer;
	Event		*currentEvent = track->events;
	int		i = 0;
	int		currentNote = 0;
	int		currentEventId = 0;

	for (; i != -1 && i < buffLen; ) {
		++track->nbOfEvents;
		for (deltaTime = buffer[i] & 0x7F; buffer[i++] & 0x80; deltaTime = (deltaTime << 7) + (buffer[i] & 0x7F));
		byte = buffer[i];

		if (byte & 0x80) {
			statusByte = byte;
			i++;
		}
		if (outputDebug)
			printf("After % 8i ticks: ", deltaTime);
		if (statusByte == 0xFF) {
			switch (buffer[i++]) {
			case 0x00:
				if (buffer[i++] != 0x02) {
					printf("Error: Invalid byte found (%#x found but expected 0x02)\n", buffer[i - 1]);
					if (outputDebug)
						showChunk(buffer, i, buffLen, posInFile + i);
					while(list.note){deleteNode(&list);}
					return (false);
				} else if (totalTime > 0) {
					printf("Error: Cannot add sequence number after non-zero delta times\n");
					while(list.note){deleteNode(&list);}
					return (false);
				}
				i += 2;
				break;
			case 0x01:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				i += len;
				break;
			case 0x02:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				i += len;
				track->nbOfEvents--;
				break;
			case 0x03:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				i += len;
				track->nbOfEvents--;
				break;
			case 0x04:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				i += len;
				track->nbOfEvents--;
				break;
			case 0x05:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				i += len;
				break;
			case 0x06:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				i += len;
				break;
			case 0x07:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				i += len;
				break;
			case 0x20:
				if (buffer[i++] != 0x01) {
					printf("Error: Invalid byte found (%#x found but expected 0x01)\n", buffer[i - 1]);
					if (outputDebug)
						showChunk(buffer, i, buffLen, posInFile + i);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				i++;
				break;
			case 0x21:
				if (buffer[i++] != 0x01) {
					printf("Error: Invalid byte found (%#x found but expected 0x01)\n", buffer[i - 1]);
					if (outputDebug)
						showChunk(buffer, i, buffLen, posInFile + i);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				i++;
				break;
			case 0X2F:
				if (buffer[i++] != 0x00) {
					printf("Error: Invalid byte found (%#x found but expected 0x00)\n", buffer[i - 1]);
					if (outputDebug)
						showChunk(buffer, i, buffLen, posInFile + i);
					while(list.note){deleteNode(&list);}
					return (false);
				} else if (i != buffLen) {
					printf("Error: Found end of track %s the last index (Found at index %i but expected %i)\n", i > buffLen ? "after" : "before", i, buffLen);
					if (outputDebug)
						showChunk(buffer, i, buffLen, posInFile + i);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				i = -1;
				break;
			case 0x51:
				if (buffer[i++] != 0x03) {
					printf("Error: Invalid byte found (%#x found but expected 0x03)\n", buffer[i - 1]);
					if (outputDebug)
						showChunk(buffer, i, buffLen, posInFile + i);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				i += 3;
				break;
			case 0x54:
				if (buffer[i++] != 0x05) {
					printf("Error: Invalid byte found (%#x found but expected 0x05)\n", buffer[i - 1]);
					if (outputDebug)
						showChunk(buffer, i, buffLen, posInFile + i);
					while(list.note){deleteNode(&list);}
					return (false);
				} else if (totalTime > 0) {
					printf("Error: Cannot add SMTPE Offset after non-zero delta times\n");
					while(list.note){deleteNode(&list);}
					return (false);
				}
				i += 5;
				break;
			case 0x58:
				if (buffer[i++] != 0x04) {
					printf("Error: Invalid byte found (%#x found but expected 0x04)\n", buffer[i - 1]);
					if (outputDebug)
						showChunk(buffer, i, buffLen, posInFile + i);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				i += 4;
				break;
			case 0x59:
				if (buffer[i++] != 0x02) {
					printf("Error: Invalid byte found (%#x found but expected 0x02)\n", buffer[i - 1]);
					if (outputDebug)
						showChunk(buffer, i, buffLen, posInFile + i);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				i += 2;
				break;
			case 0x7F:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				i += len;
				break;
			default:
				if (outputDebug)
					printf("Error: Invalid meta event type (%#x)\n", buffer[i - 1]);
				while(list.note){deleteNode(&list);}
				return (false);
			}
		} else if (statusByte >= 0x80 && statusByte < 0x90) {
			if (buffer[i++] > 127) {
				printf("Error: Note out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			} else if (buffer[i++] > 127) {
				printf("Error: Velocity out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			}
			
		} else if (statusByte >= 0x90 && statusByte < 0xA0) {
			if (buffer[i++] > 127) {
				printf("Error: Note out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			} else if (buffer[i++] > 127) {
				printf("Error: Velocity out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			}
			
			result->nbOfNotes++;
			track->nbOfNotes++;
		} else if (statusByte >= 0xA0 && statusByte < 0xB0) {
			if (buffer[i++] > 127) {
				printf("Error: Note out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			} else if (buffer[i++] > 127) {
				printf("Error: Velocity out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			}
		} else if (statusByte >= 0xB0 && statusByte < 0xC0) {
			if (buffer[i++] > 127) {
				printf("Error: Controller out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			} else if (buffer[i++] > 127) {
				printf("Error: Value out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			}
		} else if (statusByte >= 0xC0 && statusByte < 0xD0) {
			if (buffer[i++] > 127) {
				printf("Error: Program out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			}
		} else if (statusByte >= 0xD0 && statusByte < 0xE0) {
			if (buffer[i++] > 127) {
				printf("Error: Pressure out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			}
		} else if (statusByte >= 0xE0 && statusByte < 0xF0) {
			if (buffer[i++] > 127) {
				printf("Error: Lsb out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			} else if (buffer[i++] > 127) {
				printf("Error: Msb out of range (%i out of range 0-127)\n", buffer[i - 1]);
				if (outputDebug)
					showChunk(buffer, i, buffLen, posInFile + i);
				while(list.note){deleteNode(&list);}
				return (false);
			}
		} else if (statusByte == 0xF0 || statusByte == 0xF7) {
			for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
			i += len;
		} else {
			printf("Warning: Unsupported event (status byte: %#x, delta time: %u) (At pos %i)\n", statusByte, deltaTime, i + posInFile);
			if (outputDebug)
				showChunk(buffer, i, buffLen, posInFile + i);
			i++;
		}
		if (outputDebug)
			printf("   New position: %i\n", i + posInFile);
		totalTime += deltaTime;
	}
	if (outputDebug)
		printf("Begin of the track: %i\n", posInFile);
	track->events = malloc(sizeof(*track->events) * track->nbOfEvents);
	if (!track->events) {
		printf("Error: Cannot alloc %iB\n", (int)(sizeof(*track->events) * track->nbOfEvents));
		while(list.note){deleteNode(&list);}
		return (false);
	}
	memset(track->events, 0, sizeof(*track->events) * track->nbOfEvents);
	if (createNoteArray) {
		track->notes = malloc(sizeof(*track->notes) * track->nbOfNotes);
		if (!track->notes) {
			printf("Error: Cannot alloc %iB\n", (int)sizeof(*track->notes) * track->nbOfNotes);
			while(list.note){deleteNode(&list);}
			return (false);
		}
		memset(track->notes, 0, sizeof(*track->notes) * track->nbOfNotes);
	}
	totalTime = 0;
	for (i = 0; i < buffLen; ) {
		for (deltaTime = buffer[i] & 0x7F; buffer[i++] & 0x80; deltaTime = (deltaTime << 7) + (buffer[i] & 0x7F));
		for (node = &list; node; node = node->next)
			if (node->note)
				node->note->duration += deltaTime;
		byte = buffer[i];

		if (byte & 0x80) {
			statusByte = byte;
			i++;
		}
		if (outputDebug)
			printf("After % 8i ticks: ", deltaTime);
		if (statusByte == 0xFF) {
			switch (buffer[i++]) {
			case 0x00:
				i++;
				buff = malloc(sizeof(int));
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(int));
					while(list.note){deleteNode(&list);}
					return (false);
				}
				*(int *)buff = (buffer[i++] << 8);
				*(int *)buff += buffer[i++];
				if (outputDebug)
					printf("Found sequence number: %i", *(int *)buff);
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiSequenceNumber;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				break;
			case 0x01:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (outputDebug)
					printf("Text event: '%s'", (char *)buff);
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiTextEvent;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				i += len;
				break;
			case 0x02:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (outputDebug)
					printf("Copyrights to: '%s'", (char *)buff);
				free(track->copyright);
				track->copyright = buff;
				i += len;
				break;
			case 0x03:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (outputDebug)
					printf("Track name: '%s'", (char *)buff);
				free(track->name);
				track->name = buff;
				i += len;
				break;
			case 0x04:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (outputDebug)
					printf("Instrument name: '%s'", (char *)buff);
				free(track->instrumentName);
				track->instrumentName = buff;
				i += len;
				break;
			case 0x05:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (outputDebug)
					printf("Lyric event: '%s'", (char *)buff);
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiNewLyric;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				i += len;
				break;
			case 0x06:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (outputDebug)
					printf("Marker: '%s'", (char *)buff);
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiNewMarker;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				i += len;
				break;
			case 0x07:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (outputDebug)
					printf("New cue point: '%s'", (char *)buff);
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiNewCuePoint;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				i += len;
				break;
			case 0x20:
				i++;
				buff = malloc(sizeof(char));
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(char));
					while(list.note){deleteNode(&list);}
					return (false);
				}
				*(char *)buff = buffer[i++];
				if (outputDebug)
					printf("New channel prefix: %i", *(int *)buff);
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiNewChannelPrefix;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				break;
			case 0x21:
				i++;
				buff = malloc(sizeof(char));
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(char));
					while(list.note){deleteNode(&list);}
					return (false);
				}
				*(char *)buff = buffer[i++];
				if (outputDebug)
					printf("Midi port changed: %i", *(int *)buff);
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiPortChange;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				break;
			case 0X2F:
				if (buffer[i++] != 0x00) {
					printf("Error: Invalid byte found (%#x found but expected 0x00)\n", buffer[i - 1]);
					if (outputDebug)
						showChunk(buffer, i, buffLen, posInFile + i);
					while(list.note){deleteNode(&list);}
					return (false);
				} else if (i != buffLen) {
					printf("Error: Found end of track %s the last index (Found at index %i but expected %i)\n", i > buffLen ? "after" : "before", i, buffLen);
					if (outputDebug)
						showChunk(buffer, i, buffLen, posInFile + i);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				if (outputDebug)
					printf("End of track !\n");
				while(list.note){deleteNode(&list);}
				return (true);
			case 0x51:
				i++;
				buff = malloc(sizeof(int));
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(int));
					while(list.note){deleteNode(&list);}
					return (false);
				}
				*(int *)buff = (buffer[i++] << 16);
				*(int *)buff += (buffer[i++] << 8);
				*(int *)buff += buffer[i++];
				if (outputDebug)
					printf("Tempo changed: %i", *(int *)buff);
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiTempoChanged;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				break;
			case 0x54:
				i++;
				buff = malloc(sizeof(char) * 5);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 5);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				for (int j = 0; j < 5; j++)
					((char *)buff)[j] = buffer[i++];
				if (outputDebug)
					printf(
						"New offset: %ih %im %is %iframes %ihundreths of a frame",
						((char *)buff)[0],
						((char *)buff)[1],
						((char *)buff)[2],
						((char *)buff)[3],
						((char *)buff)[4]
					);
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiSMTPEOffset;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				break;
			case 0x58:
				i++;
				buff = malloc(sizeof(char) * 4);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 4);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				for (int j = 0; j < 4; j++)
					((char *)buff)[j] = buffer[i++];
				if (outputDebug)
					printf(
						"Tempo infos: time signature %i/%i 1/4 note is %i ticks %i",
						((unsigned char *)buff)[0],
						((unsigned char *)buff)[1],
						((unsigned char *)buff)[2],
						((unsigned char *)buff)[3]
					);
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiNewTimeSignature;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				break;
			case 0x59:
				i++;
				buff = malloc(sizeof(char) * 2);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 2);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				for (int j = 0; j < 2; j++)
					((char *)buff)[j] = buffer[i++];
				if (outputDebug)
					printf(
						"Key signature %i %i",
						((char *)buff)[0],
						((char *)buff)[1]
					);
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiNewKeySignature;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				break;
			case 0x7F:
				for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
				buff = malloc(len + 1);
				if (!buff) {
					printf("Error: Cannot alloc %iB\n", (int)len + 1);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				strncpy(buff, (char *)&buffer[i], len);
				((char *)buff)[len] = 0;
				if (outputDebug) {
					printf("Sequencer-Specific Meta-event: '");
					fflush(stdout);
					write(1, buff, len);
					printf("'");
				}
				currentEvent = &track->events[currentEventId++];
				currentEvent->type = MidiSequencerSpecificEvent;
				currentEvent->infos = buff;
				currentEvent->timeToAppear = deltaTime;
				i += len;
				break;
			}
		} else if (statusByte >= 0x80 && statusByte < 0x90) {
			if (createNoteArray) {
				for (node = &list; node && (!node->note || node->note->pitch != buffer[i] || node->note->channel != statusByte - 0x80); node = node->next);
				if (!node) {
					printf("Error: Note %s from channel %i is released but has never been pressed\n", getNoteString(buffer[i]), statusByte - 0x80);
					while(list.note){deleteNode(&list);}
					return (false);
				}
				node->note->fadeOutVelocity = buffer[i + 1];
				deleteNode(node);
			}
			buff = malloc(sizeof(MidiNote));
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(MidiNote));
				while(list.note){deleteNode(&list);}
				return (false);
			}
			((MidiNote *)buff)->channel = statusByte - 0x80;
			((MidiNote *)buff)->pitch = buffer[i++];
			((MidiNote *)buff)->velocity = buffer[i++];
			if (outputDebug)
				printf("%s off in channel %i (velocity: %i)", getNoteString(((MidiNote *)buff)->pitch), ((MidiNote *)buff)->channel, ((MidiNote *)buff)->velocity);
			currentEvent = &track->events[currentEventId++];
			currentEvent->type = MidiNoteReleased;
			currentEvent->infos = buff;
			currentEvent->timeToAppear = deltaTime;
		} else if (statusByte >= 0x90 && statusByte < 0xA0) {
			if (createNoteArray) {
				noteBuffer = &track->notes[currentNote++];
				noteBuffer->channel = statusByte - 0x90;
				noteBuffer->timeBeforeAppear = totalTime + deltaTime;
				noteBuffer->pitch = buffer[i];
				noteBuffer->velocity = buffer[i + 1];
				addNode(&list, noteBuffer);
			}
			buff = malloc(sizeof(MidiNote));
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(MidiNote));
				while(list.note){deleteNode(&list);}
				return (false);
			}
			((MidiNote *)buff)->channel = statusByte - 0x90;
			((MidiNote *)buff)->pitch = buffer[i++];
			((MidiNote *)buff)->velocity = buffer[i++];
			currentEvent = &track->events[currentEventId++];
			currentEvent->type = MidiNotePressed;
			currentEvent->infos = buff;
			currentEvent->timeToAppear = deltaTime;
			if (outputDebug)
				printf("%s on in channel %i (velocity: %i)", getNoteString(((MidiNote *)buff)->pitch), ((MidiNote *)buff)->channel, ((MidiNote *)buff)->velocity);
		} else if (statusByte >= 0xA0 && statusByte < 0xB0) {
			buff = malloc(sizeof(MidiNote));
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(MidiNote));
				while(list.note){deleteNode(&list);}
				return (false);
			}
			((MidiNote *)buff)->channel = statusByte - 0xA0;
			((MidiNote *)buff)->pitch = buffer[i++];
			((MidiNote *)buff)->velocity = buffer[i++];
			if (outputDebug)
				printf("Polyphonic pressure on note %s in channel %i (velocity: %i)", getNoteString(((MidiNote *)buff)->pitch), ((MidiNote *)buff)->channel, ((MidiNote *)buff)->velocity);
			currentEvent = &track->events[currentEventId++];
			currentEvent->type = MidiPolyphonicPressure;
			currentEvent->infos = buff;
			currentEvent->timeToAppear = deltaTime;
		} else if (statusByte >= 0xB0 && statusByte < 0xC0) {
			buff = malloc(sizeof(char) * 3);
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 3);
				while(list.note){deleteNode(&list);}
				return (false);
			}
			((char *)buff)[0] = statusByte - 0xB0;
			((char *)buff)[1] = buffer[i++];
			((char *)buff)[2] = buffer[i++];
			if (outputDebug)
				printf("Controller %i in channel %i is now at value %i", ((unsigned char *)buff)[1], ((unsigned char *)buff)[0], ((unsigned char *)buff)[2]);
			currentEvent = &track->events[currentEventId++];
			currentEvent->type = MidiControllerValueChanged;
			currentEvent->infos = buff;
			currentEvent->timeToAppear = deltaTime;
		} else if (statusByte >= 0xC0 && statusByte < 0xD0) {
			buff = malloc(sizeof(char) * 2);
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 2);
				while(list.note){deleteNode(&list);}
				return (false);
			}
			((char *)buff)[0] = statusByte - 0xC0;
			((char *)buff)[1] = buffer[i++];
			if (outputDebug)
				printf("Changed program of channel %i to %i", ((unsigned char *)buff)[0], ((unsigned char *)buff)[1]);
			currentEvent = &track->events[currentEventId++];
			currentEvent->type = MidiProgramChanged;
			currentEvent->infos = buff;
			currentEvent->timeToAppear = deltaTime;
		} else if (statusByte >= 0xD0 && statusByte < 0xE0) {
			buff = malloc(sizeof(char) * 2);
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 2);
				while(list.note){deleteNode(&list);}
				return (false);
			}
			((char *)buff)[0] = statusByte - 0xD0;
			((char *)buff)[1] = buffer[i++];
			if (outputDebug)
				printf("Changed pressure of all note in channel %i to %i", ((unsigned char *)buff)[0], ((unsigned char *)buff)[1]);
			currentEvent = &track->events[currentEventId++];
			currentEvent->type = MidiPressureOfChannelChanged;
			currentEvent->infos = buff;
			currentEvent->timeToAppear = deltaTime;
		} else if (statusByte >= 0xE0 && statusByte < 0xF0) {
			buff = malloc(sizeof(char) * 3);
			if (!buff) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(char) * 3);
				while(list.note){deleteNode(&list);}
				return (false);
			}
			((char *)buff)[0] = statusByte - 0xE0;
			((char *)buff)[1] = buffer[i++];
			((char *)buff)[2] = buffer[i++];
			if (outputDebug)
				printf("Changed pitch bend of all note in channel %i to %i %i", ((char *)buff)[0], ((unsigned char *)buff)[1], ((unsigned char *)buff)[2]);
			currentEvent = &track->events[currentEventId++];
			currentEvent->type = MidiPitchBendChanged;
			currentEvent->infos = buff;
			currentEvent->timeToAppear = deltaTime;
		} else if (statusByte == 0xF0 || statusByte == 0xF7) {
			for (len = buffer[i] & 0x7F; buffer[i++] & 0x80; len = (len << 7) + (buffer[i] & 0x7F));
			i += len;
		} else {
			printf("Warning: Unsupported event (status byte: %#x, delta time: %u) (At pos %i)\n", statusByte, deltaTime, i + posInFile);
			i++;
		}
		if (outputDebug)
			printf("   New position: %i\n", i + posInFile);
		totalTime += deltaTime;
	}
	printf("Error: The end of track wasn't found (expected EOT before %i but didn't find it after %i)\n", buffLen + posInFile, i + posInFile);
	while(list.note){deleteNode(&list);}
	return (false);
}

MidiParser	*parseMidi(const char *path, bool outputDebug, bool createNoteArray)
{
	char			type[5];
	int			length = 0;
	unsigned char		*buffer = NULL;
	int			bytes = 0;
	int			full = 0;
	int			buffLen = 0;
	FILE			*stream = fopen(path, "rb");
	int			fd = stream ? fileno(stream) : -1;
	static MidiParser	result;
	int			tracksFound = 0;
	bool			foundHeader = false;
	int			j = 0;

	if (fd < 0) {
		fclose(stream);
		return (NULL);
	}
	memset(type, 0, sizeof(type));
	memset(&length, 0, sizeof(length));
	memset(&result, 0, sizeof(result));
	for (; read(fd, type, 4) > 0; ) {
		full += 4;
		if (strcmp(type, "MThd") && strcmp(type, "MTrk")) {
			printf("Error: Invalid type '%s'\n", type);
			deleteMidiParserStruct(&result);
			close(fd);
			fclose(stream);
			return (NULL);
		}
		for (int i = 0; i < 4; i++) {
			length <<= 8;
			length += readSingleByte(fd, &j);
			if (j == -1) {
				printf("Error: Unexpected <EOF>\n");
				deleteMidiParserStruct(&result);
				close(fd);
				fclose(stream);
				return (NULL);
			}
		}
		full += 4;
		if (length > buffLen) {
			buffer = realloc(buffer, length + 1);
			if (!buffer) {
				printf("Error: Cannot alloc %iB\n", length + 1);
				deleteMidiParserStruct(&result);
				close(fd);
				fclose(stream);
				free(buffer);
				return (NULL);
			}
			memset(buffer, 0, length + 1);
			buffLen = length;
		}
		bytes = read(fd, buffer, length);
		if (bytes != length) {
			printf("Error: Unexpected <EOF>\n");
			deleteMidiParserStruct(&result);
			close(fd);
			fclose(stream);
			free(buffer);
			return (NULL);
		}
		if (outputDebug)
			printf("Type: %s\nlength: %i\n", type, length);
		if (strcmp(type, "MThd") == 0) {
			if (length != 6) {
				printf("Error: Invalid header: Length is supposed to be 6 but it was %i\n", length);
				deleteMidiParserStruct(&result);
				close(fd);
				fclose(stream);
				free(buffer);
				return (NULL);
			} else if (foundHeader) {
				printf("Error: Two headers were found\n");
				deleteMidiParserStruct(&result);
				close(fd);
				fclose(stream);
				free(buffer);
				return (NULL);
			}
			foundHeader = true;
			for (int i = 0; i < 2; i++) {
				result.format <<= 8;
				result.format += buffer[i];
			}
			if (result.format > 1) {
				printf("Error: Unsupported format (%i)\n", result.format);
				deleteMidiParserStruct(&result);
				close(fd);
				fclose(stream);
				free(buffer);
				return (NULL);
			}
			for (int i = 0; i < 2; i++) {
				result.nbOfTracks <<= 8;
				result.nbOfTracks += buffer[i + 2];
			}
			result.tracks = malloc(sizeof(*result.tracks) * result.nbOfTracks);
			if (!result.tracks) {
				printf("Error: Cannot alloc %iB\n", (int)sizeof(*result.tracks) * result.nbOfTracks);
				deleteMidiParserStruct(&result);
				close(fd);
				fclose(stream);
				free(buffer);
				return (NULL);
			}
			memset(result.tracks, 0, sizeof(*result.tracks) * result.nbOfTracks);
			if (buffer[4] >> 15) {
				result.fps = buffer[5] % 128;
				result.ticks = buffer[4];
			} else
				result.ticks = ((buffer[4] << 9) + (buffer[5] << 1)) >> 1;
		} else if (!foundHeader) {
			printf("Error: Tracks starts before headers\n");
			deleteMidiParserStruct(&result);
			close(fd);
			fclose(stream);
			free(buffer);
			return (NULL);
		} else if (tracksFound++ < result.nbOfTracks && !parseMidiTrack(buffer, length, &result.tracks[tracksFound - 1], outputDebug, &result, full, createNoteArray)) {
			deleteMidiParserStruct(&result);
			close(fd);
			fclose(stream);
			free(buffer);
			return (NULL);
		}
		full += length;
		if (outputDebug)
			printf(strcmp(type, "MThd") == 0 ? "Found header !\n\n" : "End of track %i\n\n", tracksFound);
		memset(type, 0, sizeof(type));
		memset(&length, 0, sizeof(length));
	}
	if (outputDebug)
		printf("Read %iB of file\n", full);
	if (!foundHeader) {
		printf("Error: No header were found\n");
		deleteMidiParserStruct(&result);
		close(fd);
		fclose(stream);
		free(buffer);
		return (NULL);
	} else if (tracksFound != result.nbOfTracks) {
		printf("Error: Invalid header: expected %i tracks but found %i\n", result.nbOfTracks, tracksFound);
		deleteMidiParserStruct(&result);
		close(fd);
		fclose(stream);
		free(buffer);
		return (NULL);
	}
	if (outputDebug) {
		printf("%s: format %hi, %hi tracks, %i notes, ", path, result.format, result.nbOfTracks, result.nbOfNotes);
		if (result.fps) {
			printf("division: %i FPS and %i ticks/frame\n", result.fps, result.ticks);
		} else
			printf("division: %i ticks / 1/4 note\n", result.ticks);
	}
	close(fd);
	fclose(stream);
	free(buffer);
	return (&result);
}
