/*
 * ProcessTable.h
 *
 *  Created on: Oct 30, 2012
 *      Author: Ben Couturier
 */

#ifndef PROCESSTABLE_H_
#define PROCESSTABLE_H_

#include <string>
#include <vector>
#include <unistd.h>

namespace bc {

/**
 * Utility class to hold process information
 */
class Process {
public:

	Process(std::string command, pid_t pid, pid_t parentPid):
		mCommand(command), mPid(pid), mParentPid(parentPid),
		mParent(0), mSize(0), mRss(0), mPss(0), mSmapsRead(false) {
		mChildren = new std::vector<Process *>;
	};

	virtual ~Process() {
		// Beware the Processes themselves belong to the process table
		delete mChildren;
	};

	pid_t getPid() {
		return mPid;
	}

	std::string getCommand() {
		return mCommand;
	}

	pid_t getParentPid() {
		return mParentPid;
	}

	void setParent(Process *p) {
		mParent = p;
	}

	long getSize() {
		if (!mSmapsRead)
			readSmaps();
		return mSize;
	}

	long getRss() {
		if (!mSmapsRead)
			readSmaps();
		return mRss;
	}

	long getPss() {
		if (!mSmapsRead)
			readSmaps();
		return mPss;
	}

	/**
	 * reads the smaps file
	 */
	void readSmaps();
	/**
	 * return the list for children for this process
	 */
	std::vector<Process *> * getChildren() {
		return mChildren;
	}

	/**
	 * return the list for children for this process
	 */
	void getAllChildren(std::vector<Process *> *list);

private:

	std::string mCommand;
	pid_t mPid;
	pid_t mParentPid;
	Process *mParent;
	std::vector<Process *> *mChildren;

	// Memory usage
	long mSize;
	long mRss;
	long mPss;
	bool mSmapsRead;
};

/**
 * Table with all running processes...
 */
class ProcessTable {
public:
	/**
	 * Main contructor
	 */
	ProcessTable();

	/**
	 * destructor for everything
	 */
	virtual ~ProcessTable();

	/**
	 * Load data from /proc and organize the links
	 */
	void load();

	/**
	 * Find a process by PID
	 */
	Process* findById(pid_t id);

	/**
	 * Find a process and its children
	 */
	std::vector<Process*> *findHierarchyById(pid_t id);


private:
	/**
	 * Utility method to register a process
	 */
	void addProcess(const char *comm, pid_t pid, pid_t ppid, uid_t uid,
			const char *args, int size);

	/**
	 * reads /proc and creates the object in the list
	 */
	void loadFromProc();

	/**
	 * reads /proc and creates the object in the list
	 */
	void loadFromProc2();

	/**
	 * vector with all processes...
	 */
	std::vector<Process *> *mProcesses;
};

} /* namespace bc */
#endif /* PROCESSTABLE_H_ */
