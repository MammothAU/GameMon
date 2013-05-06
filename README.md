=============== ABOUT =============== 

GameMon is a utility that allows the automatic monitoring and restarting of game 
servers using QStat <http://www.qstat.org>.

For a more powerful game server management system, please check out
GameCreate <http://www.gamecreate.com>. 

=============== INSTALLATION =============== 

1) Extract the contents of the .zip file into a directory. 
2) Download QStat <http://www.qstat.org> and place the qstat binary into the 
same directory as the gamemon binary.
3) Run GameMon with the parameters set as required (see below). 

Please note that you will probably need to change the working directory to
the directory that includes the gamemon and qstat binaries to ensure it is
invoked correctly. 



=============== USAGE =============== 

Game Monitor v1.32  (C) Mammoth Media Pty Ltd, 2001-2003.

GameMon is a utility that will monitor and restart a games server if it
crashes, even if the server process itself does not terminate.

Command line arguments:
* --cmd, -c <cmdline>           Command to execute to start server
  --directory, -d <directory>   Directory to begin execution in
* --game, -g <gametype> One of HLS,Q3S,Q2S,UNS etc
  --ip, -i <ip>                 Local IP of server (def: 'localhost')
  --port, -p <port>             Port the server is running on
                                (only needed if non-default)

  --restart-hour, -h <hour>     Restarts the server daily at the
  --restart-min, -m <min>       specified time, useful for servers
                                that have memory leaks
  --restart-every <hours>       Restart every X hours
  --restart-by-name             Process is killed based on path-name, not PID
                                (useful if server changes PID during execution)
  --die-on-restart              Die at restart time (instead of restarting
                                the managed server)
  --empty-restart, -e           Restarts the server when empty for 5min
  --die-on-empty                Die when empty (instead of restart)
  --timeout <seconds>           Time to wait before deciding server is down
  --pid-file <file>             File to read process ID from (if PID changes
                                during execution

  *     Indicates a required paramater

eg.  gamemon -d "c:\hlserver" -c "hlds.exe -game cstrike +exec server.cfg" -g hls

=============== LINUX =============== 

The included Linux binary was compiled on Ubuntu with gcc 3.3.5 and glibc 2.3.5
with the libraries dynamically linked. If the included binary does not work it
will probably be necessary to compile from source - simply typing 'make' should
do this for you. 


The Linux version of GameMon also includes the following switches: 

--background, -b              Runs GameMon in background
--kill-child, -k              Kill child process on SIGTERM
--print-pid                   If backgrounded, prints pid befo
--email <addr>                Send email to addr when server i
--sendmail-path <path>         Path to sendmail executable

=============== KNOWN ISSUES =============== 

You may need to download and compile QStat from CVS in order to have support 
for the latest games. Please see the SourceForge project page 
<http://sourceforge.net/projects/qstat/>for QStat for more information. 

=============== CONTACT =============== 

http://www.mammothmedia.com.au
