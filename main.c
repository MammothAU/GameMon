/*
GameMon - game server process monitoring utility
Copyright (C) 2001-2003 Mammoth Media Pty Ltd

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

For further information, contact gamemon@mammothmedia.com.au
*/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <psapi.h>
#else
#define _UNIX
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#define UNUSED(p) (void)p

const unsigned empty_timeout = 300;
unsigned game_timeout = 90;
const int game_test  = 10;

struct prog_args_s
{
	char* ip;
	short port;
	char* cmdline;
	char* dir;
	char* gametype;
	char* pid_file;
	int background;
	int restart_hour;
	int restart_min;
	int restart_die;
	float restart_every;
	int do_restart;
	int kill_child;
	int print_pid;
	char* email_recipient;
	char* sendmail_path;
	int empty_restart;
	int empty_die;
	int restart_byname;
};

#ifdef _WIN32
typedef HANDLE PID;
#else
typedef pid_t PID;

PID* processId;	// used when catching SIGTERM process
#endif
	

struct prog_args_s prog_args = {0};//default init

void PrintTimestamp(void)
{
	struct tm *newtime;
	time_t aclock;
	char output[128];

	time( &aclock );                 /* Get time in seconds */

	newtime = localtime( &aclock );  /* Convert time to struct */
                                    /* tm form */

   /* Print local time as a string */
	strftime(output,128,"%a %d %b %I:%M:%S%p",newtime);
   printf( "%s ", output );
}



void ReadProgArgs(int argc, char* argv[], struct prog_args_s* prog_args)
{
	int i;
	char buf[512];

	strncpy(buf,argv[0],512);
	buf[511] = 0;
	for (i=strlen(buf)-1;i>=0;i--)
		if (buf[i] == '\\')
			break;

	if (i > 0)
	{
		buf[i] = 0;
		chdir(buf);
	}


	for (i=1;i<argc;i++)
	{
		if (strcmp(argv[i],"-p")==0 || strcmp(argv[i],"--port") == 0)
		{
			sscanf(argv[i+1],"%hd",&prog_args->port);
			i++;
		}
		else if (strcmp(argv[i],"-e")==0 || strcmp(argv[i],"--empty-restart") == 0)
			prog_args->empty_restart = 1;
		else if (strcmp(argv[i],"-b")==0 || strcmp(argv[i],"--background") == 0)
			prog_args->background = 1;
		else if (strcmp(argv[i],"-k")==0 || strcmp(argv[i],"--kill-child") == 0)
			prog_args->kill_child = 1;
		else if (strcmp(argv[i],"--restart-by-name") == 0)
			prog_args->restart_byname = 1;
		else if (strcmp(argv[i],"-k")==0 || strcmp(argv[i],"--die-on-restart") == 0)
			prog_args->restart_die = 1;
		else if (strcmp(argv[i],"-k")==0 || strcmp(argv[i],"--die-on-empty") == 0)
		{
			prog_args->empty_die = 1;
			prog_args->empty_restart = 1;
		}
		else if (strcmp(argv[i],"--print-pid") == 0)
			prog_args->print_pid = 1;
		else if (strcmp(argv[i],"--email") == 0)
		{
			prog_args->email_recipient = argv[i+1];
			i++;
		}
		else if (strcmp(argv[i],"--sendmail-path") == 0)
		{
			prog_args->sendmail_path = argv[i+1];
			i++;
		}
		else if (strcmp(argv[i],"-c")==0 || strcmp(argv[i],"--cmd") == 0)
		{
			prog_args->cmdline = argv[i+1];
			i++;
		}
		else if (strcmp(argv[i],"--pid-file") == 0)
		{
			prog_args->pid_file = argv[i+1];
			i++;
		}
		else if (strcmp(argv[i],"-i")==0 || strcmp(argv[i],"--ip") == 0)
		{
			prog_args->ip = argv[i+1];
			i++;
		}
		else if (strcmp(argv[i],"-d")==0 || strcmp(argv[i],"--directory") == 0)
		{
			prog_args->dir = argv[i+1];
			i++;
		}
		else if (strcmp(argv[i],"-g")==0 || strcmp(argv[i],"--game") == 0)
		{
			prog_args->gametype = argv[i+1];
			i++;
		}
		else if (strcmp(argv[i],"--restart-every") == 0)
		{
			prog_args->restart_every = 0;
			sscanf (argv[i+1],"%f",&prog_args->restart_every);
			i++;
		}
		else if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--restart-hour") == 0)
		{
			sscanf(argv[i+1],"%d",&prog_args->restart_hour);
			i++;
		}
		else if (strcmp(argv[i],"-m")==0 || strcmp(argv[i],"--restart-minute") == 0)
		{
			sscanf(argv[i+1],"%d",&prog_args->restart_min);
			i++;
		}
		else if (strcmp(argv[i],"--timeout") == 0)
		{
			sscanf(argv[i+1],"%d",&game_timeout);
			i++;
		}
	
		else {
			PrintTimestamp();
			printf("-- Warning:  ignoring argument '%s'\n",argv[i]);
		}
	}
}

void Exit(int val)
{
	char tmp[128];

	UNUSED(val);

	printf("Press enter to exit...\n");
	fgets(tmp,128,stdin);
	exit(0);
}

void KillServerByName(char* path)
{
#ifdef _WIN32

#define NUMPROCS	1024
#define NUMMODULES  1024
#define FILELEN		1024

	DWORD process_ids[NUMPROCS];
	DWORD bytes;

	HMODULE modules[NUMMODULES];
	DWORD modbytes;

	unsigned int i, j;

	HANDLE proc;

	char filename[FILELEN];
	DWORD len;

	if (!EnumProcesses(process_ids,NUMPROCS*sizeof(DWORD),&bytes))
	{
		PrintTimestamp();
		printf("-- ERROR: Could not enumerate processes\n");
		return;
	}

	bytes /= sizeof(DWORD);

	//printf("Got %d processes\n",bytes);

	for (i=0;i<bytes;i++)
	{
		//printf("\n%d\n",process_ids[i]);

		proc = OpenProcess(PROCESS_ALL_ACCESS, 0, process_ids[i]);

		if (!proc)
		{
			//printf("ERROR: Could not open process handle for process %d\n",process_ids[i]);
			continue;
		}

		if (!EnumProcessModules(proc,modules,NUMMODULES*sizeof(HMODULE),&modbytes))
		{
			//printf("ERROR: Could not enumerate modules for process %d\n",process_ids[i]);
			CloseHandle(proc);
			continue;
		}

		modbytes /= sizeof(HMODULE);
		
		
		
		for (j=0;j<modbytes && j<1;j++)
		{
			len = GetModuleFileNameEx(proc,modules[j],filename,FILELEN);
			filename[len] = 0;
			//printf("-- %s\n",filename);
			//printf("-- %d\n",modules[j]);

			if (stricmp(filename,path) == 0)
			{
				TerminateProcess(proc,0);
				PrintTimestamp();
				printf("-- %s terminated.\n",filename);
			}

		}

		CloseHandle(proc);
	}
#else
#pragma error KillServerByName not implemented
#endif
}

void KillServer(struct prog_args_s* prog_args, PID pid)
{
	char filename[1024];
	char tmp[1024];
	FILE* fp;
	unsigned int i;

	if (prog_args->restart_byname)
	{
		strcpy(filename,prog_args->dir);
		strcat(filename,"\\");
		sscanf(prog_args->cmdline,"%[^ ]",tmp);
		strcat(filename,tmp);

		KillServerByName(filename);
		return;
	}

	// If a pid file is set; over-ride the provided PID
	if (prog_args->pid_file)
	{
		fp = fopen(prog_args->pid_file, "rt");
		if (fp)
		{
			//printf("-- Notice: Had PID %d\n",pid);
			PrintTimestamp();
			fscanf(fp,"%d",&i);
			fclose(fp);
			printf("-- Notice: Read PID %d from PID file\n",pid);

#ifdef _WIN32
			CloseHandle(pid);
			pid = OpenProcess(PROCESS_ALL_ACCESS,FALSE,i);
#endif
		}
	}

#ifdef _WIN32
	if (pid)
	{
		TerminateProcess(pid,0);
		CloseHandle(pid);
	}
#else
	if (pid)
		kill(pid,9);
#endif

}




#ifdef _WIN32
PID Win32_StartServer(struct prog_args_s* prog_args)
{
	STARTUPINFO si;
	PROCESS_INFORMATION ps;
	LPVOID lpMsgBuf;
	char file[256];
	char cmd[512];
	size_t i;

	strncpy(file,prog_args->cmdline,255);	file[255] = 0;

	for (i=0;i<strlen(file);i++)
		if (file[i] == ' ') file[i] = 0;


	if (prog_args->dir && strstr(file,"\\") == NULL)
		sprintf(cmd,"%s\\%s",prog_args->dir,file);
	else strcpy(cmd,file);

	

	
	GetStartupInfo(&si);

	if (! CreateProcess(cmd,prog_args->cmdline,NULL,NULL,NORMAL_PRIORITY_CLASS,CREATE_NEW_CONSOLE,NULL,prog_args->dir,&si,&ps)) 
	{
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
		PrintTimestamp();
		printf("-- Error: Unable to start process:\n%s - %s\n--\t%s\n",cmd,prog_args->cmdline,lpMsgBuf);
		Exit(1);
	}
	return ps.hProcess;
}
#else
PID UNIX_StartServer(struct prog_args_s* prog_args)
{
	PID pid;
	char file[512];
	size_t i;
	char* argv[4];

	pid = fork();
	if (pid == -1)
	{
		PrintTimestamp();
		printf("-- Error: Unable to fork\n");
		return 0;
	}
	else if (pid != 0)
		return pid;
	

//	printf("Changing to %s\n",prog_args->dir);
	if (chdir(prog_args->dir) < 0)
	{
		PrintTimestamp();
		perror("chdir failed()");
	}

	strncpy(file,prog_args->cmdline,512);
	file[511] = 0;
	for (i=0;i<strlen(file);i++)
	{
		if (file[i] == ' ') 
		{
			file[i] = 0;
			break;
		}
	}
//	printf("Execing %s (%s)\n",file,prog_args->cmdline);
	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = prog_args->cmdline;
	argv[3] = NULL;

//	system("pwd");
	fclose(stdout); fclose(stderr);
	execv("/bin/sh",argv);
	perror("execl failed(): ");
	exit(0);
}	

void UNIX_TermSignal(int signal)
{
	KillServer(&prog_args, *processId);
	exit(0);
}
#endif

PID StartServer(struct prog_args_s* prog_args)
{
#ifdef _WIN32
	return Win32_StartServer(prog_args);
#else
	return UNIX_StartServer(prog_args);
#endif
}

void ProcSleep(int seconds)
{
#ifdef _WIN32
	Sleep(seconds * 1000);
#else
	sleep(seconds);
#endif

}

int TestServer(struct prog_args_s* prog_args, int* players)
{
	int r = rand() % 10000;
	char buf[512], file[128], port[128];
	FILE* fp;
	char *token;
#ifdef _WIN32
	char* cmd = "qstat";
#else
	char* cmd = "./qstat";
#endif
	strcpy(port,"");

	if (prog_args->port)
		sprintf(port,":%d",prog_args->port);

	sprintf(buf,"%s -%s %s%s -raw $ > %d.tmp",cmd,prog_args->gametype,prog_args->ip,port, r);

	system(buf);

	sprintf(buf,"%d.tmp",r);
	if (!(fp = fopen(buf,"rt")))
	{
		PrintTimestamp();
		printf("-- Warning: unable to open temporary file\n");
		return 0;
	}

	fgets(buf,512,fp);
	fclose(fp);

	sprintf(file,"%d.tmp",r);
	unlink(file);

	if (buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = 0;

	token = strtok(buf,"$");
	token = strtok(NULL,"$");
	token = strtok(NULL,"$");

	*players = 0;

	if (token == NULL || strcmp(token,"TIMEOUT") == 0 || strcmp(token,"DOWN") == 0)
		return 0;

	token = strtok(NULL,"$");
	token = strtok(NULL,"$");
	token = strtok(NULL,"$");

	sscanf(token,"%d",players);

	return 1;
}

void EmailNotification(char* to, struct prog_args_s* args)
{
#ifndef _WIN32
	char hostname[128];
	
	time_t t;
	struct tm* tm;
	char timestr[128];
		
	FILE* pf;
	char* cmdline;
	char port[10];

	gethostname(hostname,128);
	time(&t);
	tm = localtime(&t);
	strftime(timestr,128,"%I:%M:%S%p %d/%m/%Y",tm);

	cmdline = malloc(strlen(args->sendmail_path) + strlen(to) + 2);
	if (!cmdline)
	{
		PrintTimestamp();
		printf("-- Error:  Unable to malloc cmdline\n");
		return;
	}
	sprintf(cmdline,"%s %s",args->sendmail_path, to);

	pf = popen(cmdline,"w");
	free(cmdline);
	if (!pf)
	{
		PrintTimestamp();
		printf("-- Error:  Unable to spawn sendmail\n");
		return;
	}

	if (args->port)
		sprintf(port,"%hu",args->port);
	else strcpy(port,"default");

	fprintf(pf,"From: GameMon Daemon <>\n"
		"To: %s <%s>\n"
		"Subject:  (%s) GameMon-monitored server restarted\n\n"
		"Host: %s\n"
		"Time: %s\n"
		"Command-line: %s\n"
		"IP: %s\n"
		"Port: %s\n"
		"Gametype: %s\n",
		to,to,hostname,hostname,timestr,args->cmdline,args->ip,port,args->gametype);
	
	pclose(pf);
#endif
}



int main(int argc, char* argv[])
{
	PID pid = (PID)NULL;
	unsigned timeout, e_timeout;
	int players;
	int exit_now = 0;
	time_t t, restart_at;
	struct tm* tm;

	srand((unsigned) time(NULL));

	prog_args.restart_hour = -1;
	prog_args.ip = "localhost";
	prog_args.sendmail_path = "/usr/sbin/sendmail";

	ReadProgArgs(argc,argv,&prog_args);

	if (!prog_args.background)
	{
		printf("\nGameMon v1.32  (C) Mammoth Media Pty Ltd, 2001-2003.\n\n");
		printf("GameMon comes with ABSOLUTELY NO WARRANTY.\n");
		printf("This is free software, and you are welcome to redistribute it\n");
		printf("under certain conditions; see LICENSE file for details.\n\n");
	}

#ifndef _WIN32
	if (prog_args.background)
	{
		if ((exit_now = fork()) != 0)
		{
			if (prog_args.print_pid)
				printf("%u\n",exit_now);
			_exit(0);
		}
		setsid();
		fclose(stdin); fclose(stdout); fclose(stderr);
	}

	processId = &pid;
	if (prog_args.kill_child)
		signal(SIGTERM,UNIX_TermSignal);
#endif

	/*if (prog_args.port == 0)
	{
		printf("--Error:  Port not specified\n");
		exit = 1;
	}*/
	if (prog_args.cmdline == NULL)
	{
		PrintTimestamp();
		printf("-- Error:  Command line not specified\n");
		exit_now = 1;
	}
	if (prog_args.gametype == NULL)
	{
		PrintTimestamp();
		printf("-- Error:  Game type not specified\n");
		exit_now = 1;
	}
	if (exit_now)
	{
		printf("\nGameMon is a utility that will monitor and restart a games server if it\n"
			"crashes, even if the server process itself does not terminate.\n\n");

		printf("Command line arguments:\n" 
			"* --cmd, -c <cmdline>\t\tCommand to execute to start server\n"
			"  --directory, -d <directory>\tDirectory to begin execution in\n"
			"* --game, -g <gametype>\tOne of HLS,Q3S,Q2S,UNS etc\n"
			"  --ip, -i <ip>\t\t\tLocal IP of server (def: 'localhost')\n"
			"  --port, -p <port>\t\tPort the server is running on\n\t\t\t\t(only needed if non-default)\n"
			"\n"		
			"  --restart-hour, -h <hour>\tRestarts the server daily at the\n"
			"  --restart-min, -m <min>\tspecified time, useful for servers\n"
				"\t\t\t\tthat have memory leaks\n"
			"  --restart-every <hours>\tRestart every X hours\n"
			"  --restart-by-name\t\tProcess is killed based on path-name, not PID\n"
				"\t\t\t\t(useful if server changes PID during execution)\n"
			"  --die-on-restart\t\tDie at restart time (instead of restarting\n"
				"\t\t\t\tthe managed server)\n"
			"  --empty-restart, -e\t\tRestarts the server when empty for 5min\n"
			"  --die-on-empty\t\tDie when empty (instead of restart)\n"
			"  --timeout <seconds>\t\tTime to wait before deciding server is down\n"
			"  --pid-file <file>\t\tFile to read process ID from (if PID changes\n"
			"\t\t\t\tduring execution\n"
				
#ifdef _UNIX
			"\n"
			"  --background, -b \t\tRuns GameMon in background\n\n"
			"  --kill-child, -k \t\tKill child process on SIGTERM\n"
			"  --print-pid\t\t\tIf backgrounded, prints pid before exit\n"
			"  --email <addr>\t\tSend email to addr when server is restarted\n"
			"  -sendmail-path <path>\t\tPath to sendmail executable\n"
#endif
			"\n"
			"  *\tIndicates a required paramater\n\n"
			"eg.  gamemon -d \"c:\\hlserver\" -c \"hlds.exe -game cstrike +exec server.cfg\" -g hls\n\n");
		Exit(exit_now);
	}
	
	timeout = (unsigned)-1;	// force timeout
	e_timeout = 0;

	restart_at = 0;
	

	for (;;)	// forever
	{
		// Test server if its up
		if (pid)
		{
			if (!TestServer(&prog_args,&players))
			{
				timeout += game_test;
				PrintTimestamp();
				printf("-- Warning:  Server not responding (%d secs)\n",timeout);
				
			}
			else timeout = 0;

			// Test players if required
			if (prog_args.empty_restart && players == 0)
			{
				e_timeout += game_test;
				PrintTimestamp();
				printf("-- Notice:  Server is empty (%d secs)\n",e_timeout);

				// Kill the server immediately and exit
				if (prog_args.empty_die && e_timeout >= empty_timeout)
				{
					KillServer(&prog_args,pid);
					PrintTimestamp();
					printf("-- Notice:  Terminating GameMon due to user request\n");
					exit(0);
				}
			}

#ifndef _WIN32
			if (waitpid(-1,NULL,WNOHANG) == pid)
			{
				PrintTimestamp();
				printf("-- Warning:  Child process was terminated, restarting\n");
				timeout = game_timeout;
			}
#endif
		}

		// Test if restart is required
		time(&t);
		tm = localtime(&t);

		// NOTE: Since do_restart is 0 on start up, if the 
		// time at start up is the same as the restart time
		// a restart will _not_ be triggered (do_restart must
		// equal 1 to trigger a restart)
		if (tm->tm_hour == prog_args.restart_hour && tm->tm_min == prog_args.restart_min)
		{
			if (prog_args.do_restart)
			{
				PrintTimestamp();
				printf("-- Notice:  Performing daily restart of server (%d:%02d)\n",
					prog_args.restart_hour,prog_args.restart_min);
				timeout = game_timeout;

				// Kill the server immediately and exit
				if (prog_args.restart_die)
				{
					KillServer(&prog_args,pid);
					PrintTimestamp();
					printf("-- Notice:  Terminating GameMon due to user request\n");
					exit(0);
				}
			}
			prog_args.do_restart = 0;
		}
		else
			prog_args.do_restart = 1;

		if (restart_at && restart_at <= time(NULL))
		{
			PrintTimestamp();
			printf("-- Notice:  Performing periodic restart of server (%2.1f hours elapsed)\n",
					prog_args.restart_every);
				timeout = game_timeout;

			// Kill the server immediately and exit
			if (prog_args.restart_die)
			{
				KillServer(&prog_args,pid);
				PrintTimestamp();
				printf("-- Notice:  Terminating GameMon due to user request\n");
				exit(0);
			}
		}


		// If timed out, kill and restart		
		if (timeout >= game_timeout || e_timeout >= empty_timeout)
		{
			if (pid)
			{
				PrintTimestamp();
				printf("-- Notice:  Terminating server\n");
				KillServer(&prog_args,pid);

				// If there is a defined email recipient and we aren't 
				// restarting due to a triggered restart (do_restart goes
				// low  during a restart)
				if (prog_args.email_recipient && prog_args.do_restart)
					EmailNotification(prog_args.email_recipient,&prog_args);
			}
			
			PrintTimestamp();
			printf("-- Notice:  Spawning server\n");
			pid = StartServer(&prog_args);
			e_timeout = timeout = 0;

			if (prog_args.restart_every) restart_at = time(NULL) + (long)(prog_args.restart_every * 60 * 60);
		}
		

		ProcSleep(game_test);
	}
}
