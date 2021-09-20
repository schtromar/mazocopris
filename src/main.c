/*
 * copris.c
 * Parses arguments, trfile and printerset selections and runs the server
 * 
 * COPRIS - a converting printer server
 * Copyright (C) 2020-2021 Nejc Bertoncelj <nejc at bertoncelj.eu.org>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define COPRIS_VER "0.9"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>

#include "debug.h"
#include "config.h"
#include "Copris.h"

#include "server.h"
#include "translate.h"
#include "printerset.h"

#define list_prsets() for(int p = 0; printerset[p][0][0] != '\0'; p++)\
	printf("%s  ", printerset[p][0]);

#ifndef DEBUG
#	define COPRIS_RELEASE ""
#else
#	define COPRIS_RELEASE ("-" REL)
#endif

/*
 * Verbosity levels:
 * 0  silent/only fatal
 * 1  error
 * 2  info
 * 3  debug
 */
int verbosity = 1;

void copris_help(char *copris_location) {
	printf("Usage: %s [arguments] [printer or output file]\n\n"
	       "  -p, --port NUMBER      Listening port\n"
	       "  -t, --trfile TRFILE    Character translation file\n"
	       "  -r, --printer PRSET    Printer feature set\n"
	       "  -d, --daemon           Run as a daemon\n"
	       "  -l, --limit NUMBER     Limit number of received bytes\n"
	       "      --cutoff-limit     Cut text off at limit instead of\n"
	       "                         discarding the whole chunk\n"
	       "\n"
	       "  -v, --verbose          Be verbose (-vv more)\n"
	       "  -q, --quiet            Display nothing except fatal errors (to stderr)\n"
	       "  -h, --help             Show this help\n"
	       "  -V, --version          Show program version and included printer\n"
	       "                         feature sets\n"
	       "\n"
	       "To read from stdin, omit the port argument. To echo data to stdout\n"
	       "(console/terminal), omit the output file.\n"
	       "\n"
	       "Warning notes will be printed if COPRIS assumes it is not invoked\n"
	       "correctly, but never when the quiet argument is present.\n",
	       copris_location);

	exit(EXIT_SUCCESS);
}

void copris_version() {
	printf("COPRIS version %s%s\n"
	       "(C) 2020-21 Nejc Bertoncelj <nejc at bertoncelj.eu.org>\n\n"
	       "Compiled options:\n"
	       "  Buffer size:          %4d bytes\n"
	       "  Max. filename length: %4d characters\n",
	       COPRIS_VER, COPRIS_RELEASE, BUFSIZE, FNAME_LEN);
	printf("Included printer feature sets:\n  ");
	list_prsets();
	printf("\n");

	exit(EXIT_SUCCESS);
}

int parse_arguments(int argc, char **argv, struct Attribs *attrib) {
	int c;          // Current Getopt argument
	char *parserr;  // String to integer conversion error

	/* man 3 getopt_long */
	static struct option long_options[] = {
		{"port",          1, NULL, 'p'},
		{"daemon",        0, NULL, 'd'},
		{"trfile",        1, NULL, 't'},
		{"printer",       1, NULL, 'r'},
		{"limit",         1, NULL, 'l'},
		{"cutoff-limit",  0, NULL, 'D'},
		{"verbose",       2, NULL, 'v'},
		{"quiet",         0, NULL, 'q'},
		{"help",          0, NULL, 'h'},
		{"version",       0, NULL, 'V'},
		{NULL,            0, NULL,   0} /* end-of-list */
	};

	// int argc, char* argv[], char* optstring, struct* long_options, int* longindex
	while((c = getopt_long(argc, argv, ":p:dt:r:l:vqhV", long_options, NULL)) != -1) {
		switch(c) {
		case 'p':
			attrib->portno = strtol(optarg, &parserr, 10);
			if(*parserr) {
				fprintf(stderr, "Unrecognised characters in port number (%s). "
				                "Exiting...\n", parserr);
				return 1;
			}

			if(attrib->portno > 65535 || attrib->portno < 1) {
				fprintf(stderr, "Port number out of range. "
				                "Exiting...\n");
				return 1;
			}
			break;
		case 'd':
			attrib->daemon = 1;
			break;
		case 't':
			if(strlen(optarg) <= FNAME_LEN) {
				attrib->trfile = optarg;
				attrib->copris_flags |= HAS_TRFILE;
			} else {
				fprintf(stderr, "Trfile filename too long (%s). "
				                "Exiting...\n", optarg);
				return 1;
			}
			break;
		case 'r':
			if(strlen(optarg) <= PRSET_LEN) {
				attrib->prsetname = optarg;
				attrib->copris_flags |= HAS_PRSET;
			} else {
				// Excessive length already makes it wrong
				fprintf(stderr, "Selected printer feature set does not exist (%s). "
				                "Exiting...\n", optarg);
				return 1;
			}
			break;
		case 'l':
			attrib->limitnum = strtol(optarg, &parserr, 10);
			if(*parserr) {
				fprintf(stderr, "Unrecognised characters in limit number (%s). "
				                "Exiting...\n", parserr);
				return 1;
			}

			if(attrib->limitnum > 4096 || attrib->limitnum < 0) {
				fprintf(stderr, "Limit number out of range. "
				                "Exiting...\n");
				return 1;
			}
			break;
		case 'D':
			attrib->limit_cutoff = 1;
			break;
		case 'v':
			if(verbosity != 0 && verbosity < 3)
				verbosity++;
			break;
		case 'q':
			verbosity = 0;
			break;
		case 'h':
			copris_help(argv[0]);
			break;
		case 'V':
			copris_version();
			break;
		case ':':
			if(optopt == 'p')
				fprintf(stderr, "Port number is missing. "
				                "Exiting...\n");
			else if(optopt == 't')
				fprintf(stderr, "Translation file is missing. "
				                "Exiting...\n");
			else if(optopt == 'r')
				fprintf(stderr, "Printer set is missing. "
				                "Exiting...\n");
			else if(optopt == 'l')
				fprintf(stderr, "Limit number is missing. "
				                "Exiting...\n");
			else
				fprintf(stderr, "Option '-%c' is missing an argument. "
				                "Exiting...\n", optopt);
			return 1;
		case '?':
			if(optopt == 0)
				fprintf(stderr, "Option '%s' not recognised. "
				                "Exiting...\n", argv[optind - 1]);
			else
				fprintf(stderr, "Option '-%c' not recognised. "
				                "Exiting...\n", optopt);
			return 1;
		default:
			fprintf(stderr, "Getopt returned an unknown character code 0x%x. "
			                "Exiting... \n", c);
			return 2;
		}
	} /* end of getopt */

	// Parse the last argument, destination (or lack thereof).
	// Note that only the first argument is accepted.
	if(argv[optind]) {
		if(strlen(argv[optind]) <= FNAME_LEN) {
			attrib->destination = argv[optind];
		} else {
			fprintf(stderr, "Destination filename too long (%s). "
			                "Exiting...\n", argv[optind]);
			return 1;
		}

		if(access(attrib->destination, W_OK) == -1) {
			log_perr(-1, "access", "Unable to write to output file/printer. Does "
			                       "it exist, with appropriate permissions?");
			return 1;
		}
	}

	return 0;
}

int main(int argc, char **argv) {
	// The main attributes struct which holds most of the run-time options
	struct Attribs attrib;

	attrib.portno       = -1;
	attrib.prset        = -1;
	attrib.daemon       = 0;
	attrib.limitnum     = 0;
	attrib.limit_cutoff = 0;
	attrib.copris_flags = 0x00;

	int is_stdin  = 0;     // Determine if text is coming via the standard input
	int parentfd  = 0;     // Parent file descriptor to hold a socket
	int error     = 0;

	// Parsing arguments
	error = parse_arguments(argc, argv, &attrib);
	if(error)
		return EXIT_FAILURE;

	// Parsing the selected printer feature sets
	if(attrib.copris_flags & HAS_PRSET) {
		for(int p = 0; printerset[p][0][0] != '\0'; p++) {
			if(strcmp(attrib.prsetname, printerset[p][0]) == 0) {
				//prset.exists = p + 1;
				attrib.prset = p;
				break;
			}
		}

		// Missing printer sets are not a fatal error in quiet mode
		if(attrib.prset == -1) {
			fprintf(stderr, "Selected printer feature set does not exist "
			                "(%s). ", attrib.prsetname);
			if(verbosity) {
				fprintf(stderr, "Exiting...\n");
				return EXIT_FAILURE;
			} else {
				fprintf(stderr, "Disabling it.\n");
				attrib.copris_flags &= ~HAS_PRSET;
			}
		}
	}

	// Parsing and loading translation definitions
	if(attrib.copris_flags & HAS_TRFILE) {
		error = copris_loadtrfile(attrib.trfile);
		if(error) {
			// Missing translation files are as well not a fatal error when --quiet
			if(verbosity) {
				fprintf(stderr, "Exiting...\n");
				return EXIT_FAILURE;
			} else {
				fprintf(stderr, "Disabling translation.\n");
				attrib.copris_flags &= ~HAS_TRFILE;
			}
		}
	}

	if(argc < 2 && log_err()) {
		if(log_info())
			log_date();
		else
			printf("Note: ");

		printf("COPRIS won't do much without any arguments. "
               "Try using the '--help' option.\n");
	}

	if(attrib.portno < 1)
		is_stdin = 1;

	if(log_info()) {
		log_date();
		printf("Verbosity level set to %d.\n", verbosity);
	}

	if(attrib.daemon && is_stdin) {
		attrib.daemon = 0;
		if(log_err()){
			if(log_info())
				log_date();
			else
				printf("Note: ");
			
			printf("Daemon mode not available while reading from stdin.\n");
		}
	}

	if(attrib.daemon && log_debug()) {
		log_date();
		printf("Daemon mode enabled.\n");
	}
	
	if((attrib.copris_flags & HAS_PRSET) && log_info()) {
		log_date();
		printf("Selected printer feature set %s.\n",
		       printerset[attrib.prset][0]);
	}
	
	if(attrib.limitnum > 0 && log_debug()) {
		log_date();
		printf("Limiting incoming data to %d bytes.\n", attrib.limitnum);
	}
	
	if(log_debug() && !is_stdin) {
		log_date();
		printf("Server is listening to port %d.\n", attrib.portno);
	}
	
	if(log_info()) {
		log_date();
		printf("Data stream will be sent to ");
		if(attrib.copris_flags & HAS_DESTINATION)
			printf("%s.\n", attrib.destination);
		else
			printf("stdout.\n");
	}
	
	// Open socket and listen
	if(!is_stdin) {
		error = copris_socket_listen(&parentfd, attrib.portno);
		if(error)
			return EXIT_FAILURE;
	}

	do {
		if(is_stdin) {
			error = copris_read_stdin(&attrib);
		} else {
			// Read from socket
			error = copris_read_socket(&parentfd, &attrib);
		}
	} while(attrib.daemon && !error);

	if(error)
		return EXIT_FAILURE;

	if(log_debug() && !is_stdin) {
		log_date();
		printf("Not running as a daemon, exiting.\n");
	}

	return 0;
}
