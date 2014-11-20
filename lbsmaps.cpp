
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "ProcessTable.h"

using namespace std;

void parent(pid_t childPid, char *outputfile, int interval) {

	struct timeval start_time;
	gettimeofday(&start_time, NULL);
	ofstream output;
	ostream *myout;

	if (outputfile != 0) {
		output.open (outputfile);
		myout = &output;
	} else {
		myout = &cout;
	}


	*myout << "<?xml version=\"1.0\"?>" << endl;
	*myout << "<profile main=\"" << childPid <<  "\"" << ">" << endl;

	while(1) {

		bc::ProcessTable *t = new bc::ProcessTable();
		t->load();

		bc::Process *p = t->findById(childPid);

		//cout << "*** Listing children processes for " << childPid << endl;
		vector<bc::Process *> *all = new vector<bc::Process *>();
		p->getAllChildren(all);

		struct timeval ti;
		gettimeofday(&ti, NULL);
		long long cur_time = (long long) (ti.tv_sec - start_time.tv_sec) * 1000000
				+ (ti.tv_usec - start_time.tv_usec);

		ostringstream os;
		os << "<memprof time=\"" << cur_time << "\">" << endl;
		for (vector<bc::Process *>::iterator it = all->begin(); it!=all->end(); ++it){
			bc::Process *proc = (*it);
			//cout << (*it)-> mPid << " " << (*it)->mCommand << " "
			//		<< (*it)->getSize() << " " << (*it)->getRss() << endl;

			os << "<process pid=\"" << proc->getPid() << "\" size=\"" << proc->getSize()
						<< "\" rss=\"" << proc->getRss() << "\" pss=\"" << proc->getPss()
						<< "\">" << proc->getCommand() << "</process>" << endl;
		}
		os << "</memprof>";

		*myout << os.str() << endl;

		delete all;
		delete t;

		// Checking if the child has finished...
		struct rusage rusage;
		int options = WNOHANG;
		int status;
		pid_t wres = wait4(childPid, &status, options, &rusage);
		if (wres > 0) {
			printf("Child has finished...");
			break;
		}
		usleep(interval * 1000);
	}

	*myout << "</profile>" << endl;

}

void child(char *outputfile, char *command, char *commandArgs[]) {
	fprintf(stderr, "Running Command: %s\n", command);
	int idx = 0;
	while(idx >= 0) {
		if(commandArgs[idx] == 0) {
			idx = -1;
		} else {
			fprintf(stderr, "Arg[%d]: %s\n", idx, commandArgs[idx]);
			idx++;
		}
	}
	execvp(command, commandArgs);
	fprintf(stderr, "ERROR EXECVE");
}

int main(int argc, char *argv[]) {

	// Checking the arguments
	int first_arg = 1;
	char *outputfile = 0;
	int interval = 1000;
	while(first_arg < argc) {
		if (strcmp(argv[first_arg], "-o") == 0) {
			outputfile = argv[first_arg+1];
			first_arg += 2;
			continue;
		}
		if (strcmp(argv[first_arg], "-i") == 0) {
			interval = atoi(argv[first_arg+1]);
			first_arg += 2;
			continue;
		}
		break;
	}
	// any args left ?
	if (first_arg >= argc) {
		cerr << "Usage: " << argv[0] << " [-o outputfile] [-i interval] <command>" << endl;
		return EXIT_FAILURE;
	}

	char *command = argv[first_arg];
	char *commandArgs[argc - first_arg];
	for (int i = 0 ; i <=argc - first_arg; i++) {
		commandArgs[i] = argv[first_arg + i];
	}
	commandArgs[argc - first_arg] = 0;

	pid_t forkres = fork();
	if (forkres < 0) {
		fprintf(stderr, "Fork Error: %s\n", strerror(errno));
	} else if (forkres == 0) {
		child(outputfile, command, commandArgs);
	} else {
		parent(forkres, outputfile, interval);
	}
	return EXIT_SUCCESS;
}
