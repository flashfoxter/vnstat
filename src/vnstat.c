/*
vnStat - Copyright (c) 2002-07 Teemu Toivola <tst@iki.fi>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program;  if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave., Cambridge, MA 02139, USA.
*/

#include "vnstat.h"
#include "proc.h"
#include "db.h"
#include "misc.h"
#include "cfg.h"

int main(int argc, char *argv[]) {

	int i;
	int currentarg, update=0, query=1, newdb=0, reset=0, sync=0;
	int active=-1, files=0, force=0, cleartop=0, rebuildtotal=0, traffic=0;
	int livetraffic=0;
	char interface[32], dirname[512], nick[32];
	char definterface[32];
	time_t current;
	DIR *dir;
	struct dirent *di;

	debug = 0; /* debug disabled by default */

	/* early check for debug parameter */
	if (argc > 1) {
		for (currentarg=1; currentarg<argc; currentarg++) {
			if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug"))==0) {
				debug = 1;
			}
		}
	}
	
	/* load config if available */
	loadcfg();

	setlocale(LC_ALL, cfg.locale);
	strncpy(interface, "default", 32);
	strncpy(definterface, cfg.iface, 32);
	strncpy(nick, "none", 32);

	current = time(NULL);

	/* init dirname */
#ifdef SINGLE
	strncpy(dirname, getenv("HOME"), 500);
	strcat(dirname,"/.vnstat");
#else
	strncpy(dirname, cfg.dbdir, 512);
#endif

	/* parse parameters, maybe not the best way but... */
	for (currentarg=1; currentarg<argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n",currentarg,argv[currentarg]);
		if ((strcmp(argv[currentarg],"--longhelp"))==0) {
		
			printf(" vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", VNSTATVERSION);
			
			printf("   Update:\n");
			printf("\t -u, --update\t\t update database\n");
			printf("\t -r, --reset\t\t reset interface counters\n");
			printf("\t --sync  \t\t sync interface counters\n");
			printf("\t --enable\t\t enable interface\n");
			printf("\t --disable\t\t disable interface\n");
			printf("\t --nick  \t\t set a nickname for interface\n");
			printf("\t --cleartop\t\t clear the top10\n");
			printf("\t --rebuildtotal\t\t rebuild total transfers from months\n");
			
			printf("   Query:\n");
			printf("\t -q, --query\t\t query database\n");
			printf("\t -h, --hours\t\t show hours\n");
			printf("\t -d, --days\t\t show days\n");
			printf("\t -m, --months\t\t show months\n");
			printf("\t -w, --weeks\t\t show weeks\n");
			printf("\t -t, --top10\t\t show top10\n");
			printf("\t -s, --short\t\t use short output\n");
			printf("\t --dumpdb\t\t show database in parseable format\n");
			
			printf("   Misc:\n");
			printf("\t -i,  --iface\t\t change interface (default: %s)\n", definterface);
			printf("\t -?,  --help\t\t short help\n");
			printf("\t -D,  --debug\t\t show some additional debug information\n");
			printf("\t -v,  --version\t\t show version\n");
			printf("\t -tr, --traffic\t\t calculate traffic\n");
			printf("\t -l,  --live\t\t show transfer rate in real time\n");
			printf("\t --showconfig\t\t dump config file with current settings\n");
			printf("\t --testkernel\t\t check if the kernel is broken\n");
			printf("\t --longhelp\t\t display this help\n\n");

			printf("See also \"man vnstat\".\n");

			exit(0);

		} else if ((strcmp(argv[currentarg],"-?")==0) || (strcmp(argv[currentarg],"--help"))==0) {

			printf(" vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", VNSTATVERSION);

			printf("\t -q,  --query\t\t query database\n");
			printf("\t -h,  --hours\t\t show hours\n");
			printf("\t -d,  --days\t\t show days\n");
			printf("\t -m,  --months\t\t show months\n");
			printf("\t -w,  --weeks\t\t show weeks\n");
			printf("\t -t,  --top10\t\t show top10\n");
			printf("\t -s,  --short\t\t use short output\n");
			printf("\t -u,  --update\t\t update database\n");			
			printf("\t -i,  --iface\t\t change interface (default: %s)\n", definterface);
			printf("\t -?,  --help\t\t short help\n");
			printf("\t -v,  --version\t\t show version\n");
			printf("\t -tr, --traffic\t\t calculate traffic\n");
			printf("\t -l,  --live\t\t show transfer rate in real time\n\n");

			printf("See also \"--longhelp\" for complete options list and \"man vnstat\".\n");

			exit(0);

		} else if ((strcmp(argv[currentarg],"-i")==0) || (strcmp(argv[currentarg],"--iface"))==0) {
			if (currentarg+1<argc) {
				strncpy(interface,argv[currentarg+1],32);
				if (debug)
					printf("Used interface: %s\n",interface);
				currentarg++;
				continue;
			} else {
				printf("Interface for -i missing\n");
				exit(1);
			}
		} else if ((strcmp(argv[currentarg],"--nick"))==0) {
			if (currentarg+1<argc) {
				strncpy(nick,argv[currentarg+1],32);
				if (debug)
					printf("Used nick: %s\n",nick);
				currentarg++;
				continue;
			} else {
				printf("Nick for --nick missing\n");
				exit(1);
			}
		} else if ((strcmp(argv[currentarg],"-u")==0) || (strcmp(argv[currentarg],"--update"))==0) {
			update=1;
			query=0;
			if (debug)
				printf("Updating database...\n");
		} else if ((strcmp(argv[currentarg],"-q")==0) || (strcmp(argv[currentarg],"--query"))==0) {
			query=1;
		} else if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug"))==0) {
			debug=1;
		} else if ((strcmp(argv[currentarg],"-d")==0) || (strcmp(argv[currentarg],"--days"))==0) {
			cfg.qmode=1;
		} else if ((strcmp(argv[currentarg],"-m")==0) || (strcmp(argv[currentarg],"--months"))==0) {
			cfg.qmode=2;
		} else if ((strcmp(argv[currentarg],"-t")==0) || (strcmp(argv[currentarg],"--top10"))==0) {
			cfg.qmode=3;
		} else if ((strcmp(argv[currentarg],"-s")==0) || (strcmp(argv[currentarg],"--short"))==0) {
			cfg.qmode=5;
		} else if ((strcmp(argv[currentarg],"-w")==0) || (strcmp(argv[currentarg],"--weeks"))==0) {
			cfg.qmode=6;			
		} else if ((strcmp(argv[currentarg],"-h")==0) || (strcmp(argv[currentarg],"--hours"))==0) {
			cfg.qmode=7;
		} else if (strcmp(argv[currentarg],"--dumpdb")==0) {
			cfg.qmode=4;
		} else if (strcmp(argv[currentarg],"--enable")==0) {
			active=1;
			query=0;
		} else if ((strcmp(argv[currentarg],"-tr")==0) || (strcmp(argv[currentarg],"--traffic"))==0) {
			if (currentarg+1<argc) {
				if (isdigit(argv[currentarg+1][0])) {
					cfg.sampletime=atoi(argv[currentarg+1]);
					currentarg++;
					traffic=1;
					query=0;
					continue;
				}
			}
			traffic=1;
			query=0;
		} else if ((strcmp(argv[currentarg],"-l")==0) || (strcmp(argv[currentarg],"--live"))==0) {
			livetraffic=1;
			query=0;
		} else if (strcmp(argv[currentarg],"--force")==0) {
			force=1;
		} else if (strcmp(argv[currentarg],"--cleartop")==0) {
			cleartop=1;
		} else if (strcmp(argv[currentarg],"--rebuildtotal")==0) {
			rebuildtotal=1;
		} else if (strcmp(argv[currentarg],"--disable")==0) {
			active=0;
			query=0;
		} else if (strcmp(argv[currentarg],"--testkernel")==0) {
			kerneltest();
			exit(0);
		} else if (strcmp(argv[currentarg],"--showconfig")==0) {
			printcfgfile();
			exit(0);			
		} else if ((strcmp(argv[currentarg],"-v")==0) || (strcmp(argv[currentarg],"--version"))==0) {
#ifndef SINGLE
			if (debug)
				printf("Root ");
#else
			if (debug)
				printf("Single user ");
#endif
			printf("vnStat %s by Teemu Toivola <tst at iki dot fi>\n", VNSTATVERSION);
			exit(0);
		} else if ((strcmp(argv[currentarg],"-r")==0) || (strcmp(argv[currentarg],"--reset"))==0) {
			reset=1;
			query=0;
		} else if (strcmp(argv[currentarg],"--sync")==0) {
			sync=1;
			query=0;			
		} else {
			printf("Unknown arg \"%s\". Use --help for help.\n",argv[currentarg]);
			exit(1);
		}
		
	}

	/* check if the database dir exists and if it contains files */
	if (!traffic && !livetraffic) {
		if ((dir=opendir(dirname))!=NULL) {
			if (debug)
				printf("Dir OK\n");
			while ((di=readdir(dir))) {
				if (di->d_name[0]!='.') {
					strncpy(definterface, di->d_name, 32);
					files++;
				}
			}
			if (debug)
				printf("%d file(s) found\n", files);
			if (files>1) {
				strncpy(definterface, cfg.iface, 32);
			}
			closedir(dir);
		} else {
			printf("Error:\nUnable to open database directory \"%s\".\n", dirname);
			printf("Make sure it exists and is at least read enabled for current user.\n");
			printf("Exiting...\n");
			exit(0);
		}
	}

	/* counter reset */
	if (reset) {
		if (!spacecheck(dirname) && !force) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		if (strcmp(interface, "default")==0)
			strncpy(interface, definterface, 32);
		readdb(interface, dirname);
		data.currx=0;
		data.curtx=0;
		writedb(interface, dirname, 0);
		if (debug)
			printf("Counters reseted for \"%s\"\n", data.interface);
	}

	/* counter sync */
	if (sync) {
		if (!spacecheck(dirname) && !force) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		if (strcmp(interface, "default")==0)
			strncpy(interface, definterface, 32);
		synccounters(interface, dirname);
		if (debug)
			printf("Counters synced for \"%s\"\n", data.interface);
	}

	/* clear top10 */
	if (cleartop) {
		if (!spacecheck(dirname) && !force) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		if (strcmp(interface, "default")==0)
			strncpy(interface, definterface, 32);
		if (force) {
			readdb(interface, dirname);

			for (i=0; i<=9; i++) {
				data.top10[i].rx=data.top10[i].tx=0;
				data.top10[i].rxk=data.top10[i].txk=0;
				data.top10[i].used=0;
			}

			writedb(interface, dirname, 0);
			printf("Top10 cleared for interface \"%s\".\n", data.interface);
			query=0;
		} else {
			printf("Warning:\nThe current option would clear the top10 for \"%s\".\n", interface);
			printf("Use --force to override this message.\n");
			exit(0);
		}
	}

	/* rebuild total */
	if (rebuildtotal) {
		if (!spacecheck(dirname)) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		if (strcmp(interface, "default")==0)
			strncpy(interface, definterface, 32);
		if (force) {
			readdb(interface, dirname);

			data.totalrx=data.totaltx=data.totalrxk=data.totaltxk=0;
			for (i=0; i<=29; i++) {
				if (data.month[i].used) {
					addtraffic(&data.totalrx, &data.totalrxk, data.month[i].rx, data.month[i].rxk);
					addtraffic(&data.totaltx, &data.totaltxk, data.month[i].tx, data.month[i].txk);
				}
			}

			writedb(interface, dirname, 0);
			printf("Total transfer rebuild completed for interface \"%s\".\n", data.interface);
			query=0;
		} else {
			printf("Warning:\nThe current option would rebuild total tranfers for \"%s\".\n", interface);
			printf("Use --force to override this message.\n");
			exit(0);
		}
	}

	/* enable & disable */
	if (active==1) {
		if (!spacecheck(dirname) && !force) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		if (strcmp(interface, "default")==0)
			strncpy(interface, definterface, 32);
		newdb=readdb(interface, dirname);
		if (!data.active && !newdb) {
			data.active=1;
			writedb(interface, dirname, 0);
			if (debug)
				printf("Interface \"%s\" enabled.\n", data.interface);
		} else if (!newdb) {
			printf("Interface \"%s\" is already enabled.\n", data.interface);
		}
	} else if (active==0) {
		if (!spacecheck(dirname) && !force) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		if (strcmp(interface, "default")==0)
			strncpy(interface, definterface, 32);
		newdb=readdb(interface, dirname);
		if (data.active && !newdb) {
			data.active=0;
			writedb(interface, dirname, 0);
			if (debug)
				printf("Interface \"%s\" disabled.\n", data.interface);
		} else if (!newdb) {
			printf("Interface \"%s\" is already disabled.\n", data.interface);
		}	
	}
	
	/* update */
	if (update) {
	
		/* check that there's some free diskspace left */
		if (!spacecheck(dirname) && !force) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		
		/* update every file if -i isn't specified */
		if (strcmp(interface, "default")==0) {
			
			dir=opendir(dirname);

			files=0;
			while ((di=readdir(dir))) {
				if (di->d_name[0]!='.') {
					files++;
					strncpy(interface, di->d_name, 32);
					if (debug)
						printf("\nProcessing file \"%s/%s\"...\n", dirname, interface);
					newdb=readdb(interface, dirname);
					if (data.active) {
						readproc(data.interface);
						parseproc(newdb);
						
						/* check that the time is correct */
						if ((current>=data.lastupdated) || force) {
							writedb(interface, dirname, newdb);
						} else {
							printf("Error:\nThe previous update was after the current date.\n\n");
							printf("Previous update: %s", (char*)asctime(localtime(&data.lastupdated)));
							printf("   Current time: %s\n", (char*)asctime(localtime(&current)));
							printf("Use --force to override this message.\n");
							exit(0);
						}
					} else {
						if (debug)
							printf("Disabled interface \"%s\" not updated.\n", data.interface);
					}				
				}
			}

			closedir(dir);
			if (files==0) {
				// printf("No database found.\n");
				update=0;
			}
			
			/* reset to default */
			strncpy(interface, "default", 32);
		
		/* update only selected file */
		} else {
			newdb=readdb(interface, dirname);
			if (data.active) {
				readproc(data.interface);
				parseproc(newdb);
				if ((current>=data.lastupdated) || force) {
					if (strcmp(nick, "none")!=0)
						strncpy(data.nick, nick, 32);
					writedb(interface, dirname, newdb);
				} else {
					printf("Error:\nThe previous update was after the current date.\n\n");
					printf("Previous update: %s", (char*)asctime(localtime(&data.lastupdated)));
					printf("   Current time: %s\n", (char*)asctime(localtime(&current)));
					printf("Use --force to override this message.\n");
					exit(0);
				}
			} else {
				if (debug)
					printf("Disabled interface \"%s\" not updated.\n", data.interface);
			}
		}
	}
	
	/* show databases */
	if (query) {
	
		/* show all interfaces if -i isn't specified */
		if (strcmp(interface, "default")==0) {
			
			if (files==0) {
				// printf("No database found.\n");
				query=0;
			} else if ((cfg.qmode==0) && (files>1)) {

				dir=opendir(dirname);
				printf("\n                     rx      /     tx      /    total    /  estimated\n");
				while ((di=readdir(dir))) {
					if (di->d_name[0]!='.') {
						strncpy(interface, di->d_name, 32);
						if (debug)
							printf("\nProcessing file \"%s/%s\"...\n", dirname, interface);
						readdb(interface, dirname);
						if (!newdb)
							showdb(5);
						
					}
				}
				closedir(dir);
				
			/* show in qmode if there's only one file or qmode!=0 */
			} else {
				readdb(definterface, dirname);
				if (!newdb) {
					if (cfg.qmode==5)
						printf("\n                     rx      /     tx      /    total    /  estimated\n");
					showdb(cfg.qmode);
				}
			}
		
		/* show only specified file */
		} else {
			readdb(interface, dirname);
			if (!newdb) {
				if (cfg.qmode==5)
					printf("\n                     rx      /     tx      /    total    /  estimated\n");
				showdb(cfg.qmode);
			}
		}
	}
	
	/* calculate traffic */
	if (traffic) {
		if (strcmp(interface, "default")==0)
			strncpy(interface, definterface, 32);
		trafficmeter(interface, cfg.sampletime);
	}

	/* live traffic */
	if (livetraffic) {
		if (strcmp(interface, "default")==0)
			strncpy(interface, definterface, 32);

		livetrafficmeter(interface);
	}
	
	/* if nothing was shown previously */
	if (!query && !update && !reset && !sync && active==-1 && !cleartop && !rebuildtotal && !traffic && !livetraffic) {
		
		/* give more help if there's no database */
		if (files==0) {
			printf("No database found, nothing to do. Use --help for help.\n\n");
			printf("A new database can be created with the following command:\n");
			printf("    %s -u -i eth0\n\n", argv[0]);
			printf("Replace 'eth0' with the interface that should be monitored. A list\n");
			printf("of available interfaces can be seen with the 'ifconfig' command.\n");
		} else {
			printf("Nothing to do. Use --help for help.\n");
		}
	}
	
	return 0;
}
