/*
 * ProcessTable.cpp
 *
 *  Created on: Oct 30, 2012
 *      Author: Ben Couturier
 */

#include "ProcessTable.h"

#include <vector>
#include <string>
#include <iterator>
#include <fstream>
#include <iostream>
#include <sstream>
#include <boost/regex.hpp>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <pwd.h>
#include <dirent.h>
#include <curses.h>
#include <term.h>
#include <termios.h>
#include <langinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>

#define PROC_BASE    "/proc"
#define COMM_LEN 16
static int print_args = 1;
static int output_width = 132;
using namespace std;

namespace bc {

void Process::getAllChildren(std::vector<Process *> *list) {
	list->push_back(this);
	for(vector<Process *>::iterator it = mChildren->begin();  it != mChildren->end(); ++it) {
		Process *child = (*it);
		if (child != 0) {
			child->getAllChildren(list);
		}
	}
}

void Process::readSmaps() {

	ostringstream os;
	os << PROC_BASE << "/" << mPid << "/smaps";
	string filename = os.str();
	ifstream file(filename.c_str());
	string line;
	boost::cmatch matches;

	boost::regex SIZE_REGEX( "Size:\\s*(\\d+)\\s*kB" ) ;
	boost::regex RSS_REGEX( "Rss:\\s*(\\d+)\\s*kB" ) ;
	boost::regex PSS_REGEX( "Pss:\\s*(\\d+)\\s*kB" ) ;

	long totalSize = 0;
	long totalRss = 0;
	long totalPss = 0;

	if(file)
	{
		while(getline(file,line))
		{
			if (boost::regex_match( line.c_str(), matches, SIZE_REGEX )) {
				totalSize += atoi(matches[1].str().c_str());
			}
			if (boost::regex_match( line.c_str(), matches, RSS_REGEX )) {
				totalRss += atoi(matches[1].str().c_str());
			}
			if (boost::regex_match( line.c_str(), matches, PSS_REGEX )) {
				totalPss += atoi(matches[1].str().c_str());
			}
		}
		file.close();
	}

	mSize = totalSize;
	mRss = totalRss;
	mPss = totalPss;
	mSmapsRead = true;
}


ProcessTable::ProcessTable() {
	mProcesses = new vector<Process *>();
}

Process* ProcessTable::findById(pid_t pid) {
	for(vector<Process *>::iterator it = mProcesses->begin();  it != mProcesses->end(); ++it) {
		if ((*it)->getPid() == pid) {
			return (*it);
		}
	}
	return 0;
}

void ProcessTable::addProcess(const char *comm, pid_t pid, pid_t ppid, uid_t uid,
		const char *args, int size)
{
	string c = string(comm);
	Process *p = new Process(c, pid, ppid);
	mProcesses->push_back(p);
	//printf("Proc:%s, %d parent:%d\n", comm, pid, ppid);
}

void ProcessTable::load() {
	// First load the data from /proc
	loadFromProc();

	// Now we need to establish the relationships...
	// Iterating on all processes
	for(vector<Process *>::iterator it = mProcesses->begin();  it != mProcesses->end(); ++it) {
		Process *p = *it;
		if (p->getParentPid() != 0) {
			Process *parent = findById(p->getParentPid());
			if (parent == 0) {
				fprintf(stderr, "process: %d, Parent %d could not be found", p->getPid(), p->getParentPid());
			} else {
				p->setParent(parent);
				parent->getChildren()->push_back(p);
			}
		}
	}
}


void ProcessTable::loadFromProc2() {
	DIR *dir;
	struct dirent *de;

	if (!(dir = opendir (PROC_BASE)))
	{
		perror (PROC_BASE);
		exit (1);
	}


}

void ProcessTable::loadFromProc() {
	DIR *dir;
	struct dirent *de;
	FILE *file;
	struct stat st;
	char *path, comm[COMM_LEN + 1];
	char *buffer;
	char readbuf[BUFSIZ+1];
	char *tmpptr;
	pid_t pid, ppid;
	int fd, size;
	int empty;


	if (!(buffer = (char *)malloc ((size_t) (output_width + 1))))
	{
		perror ("malloc");
		exit (1);
	}


	if (!(dir = opendir (PROC_BASE)))
	{
		perror (PROC_BASE);
		exit (1);
	}
	empty = 1;
	while ((de = readdir (dir)) != NULL)
		if ((pid = (pid_t) atoi (de->d_name)) != 0)
		{
			if (!(path = (char *)malloc (strlen (PROC_BASE) + strlen (de->d_name) + 10)))
				exit (2);
			sprintf (path, "%s/%d/stat", PROC_BASE, pid);
			if ((file = fopen (path, "r")) != NULL)
			{
				empty = 0;
				sprintf (path, "%s/%d", PROC_BASE, pid);
				if (stat (path, &st) < 0)
				{
					perror (path);
					exit (1);
				}
				fread(readbuf, BUFSIZ, 1, file) ;
				if (ferror(file) == 0)
				{
					memset(comm, '\0', COMM_LEN+1);
					tmpptr = strrchr(readbuf, ')'); /* find last ) */
					/* We now have readbuf with pid and cmd, and tmpptr+2
					 * with the rest */
					/*printf("readbuf: %s\n", readbuf);*/
					if (sscanf(readbuf, "%*d (%15[^)]", comm) == 1)
					{
						/*printf("tmpptr: %s\n", tmpptr+2);*/
						if (sscanf(tmpptr+2, "%*c %d", &ppid) == 1)
						{
							/*
				    if (fscanf
				    (file, "%d (%s) %c %d", &dummy, comm, (char *) &dummy,
				    &ppid) == 4)
							 */
							{
								DIR *taskdir;
								struct dirent *dt;
								char *taskpath;
								char *threadname;
								int thread;

								if (!(taskpath = (char *)malloc(strlen(path) + 10))) {
									exit (2);
								}
								sprintf (taskpath, "%s/task", path);

								if ((taskdir=opendir(taskpath))!=0) {
									/* if we have this dir, we're on 2.6 */
									if (!(threadname = (char *)malloc(strlen(comm) + 3))) {
										exit (2);
									}
									sprintf(threadname,"{%s}",comm);
									while ((dt = readdir(taskdir)) != NULL) {
										if ((thread=atoi(dt->d_name)) !=0) {
											if (thread != pid) {
												if (print_args)
													addProcess(threadname, thread, pid, st.st_uid, threadname, strlen(threadname)+1);
												else
													addProcess(threadname, thread, pid, st.st_uid, NULL, 0);
											}
										}
									}
									free(threadname);
									(void) closedir(taskdir);
								}
								free(taskpath);
							}

							if (!print_args)
								addProcess (comm, pid, ppid, st.st_uid, NULL, 0);
							else
							{
								sprintf (path, "%s/%d/cmdline", PROC_BASE, pid);
								if ((fd = open (path, O_RDONLY)) < 0)
								{
									perror (path);
									exit (1);
								}
								if ((size = read (fd, buffer, (size_t) output_width)) < 0)
								{
									perror (path);
									exit (1);
								}
								(void) close (fd);
								if (size)
									buffer[size++] = 0;
								addProcess (comm, pid, ppid, st.st_uid, buffer, size);
							}
						}
					}
				}
				(void) fclose (file);
			}
			free (path);
		}
	(void) closedir (dir);
	if (print_args)
		free (buffer);
	if (empty)
	{
		fprintf (stderr, "%s is empty (not mounted ?)\n", PROC_BASE) ;
		exit (1);
	}
}

ProcessTable::~ProcessTable() {
	for(vector<Process *>::iterator it = mProcesses->begin();  it != mProcesses->end(); ++it) {
		if (*it != 0) {
			delete (*it);
		}
	}
	delete mProcesses;
}

} /* namespace bc */
