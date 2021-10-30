#include <iostream>
#include <string.h>
#include <vector>
#include <stdio.h>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define BASH_EXEC "/bin/sh"
#define BSIZE 256
#define ARGSIZE 32
using namespace std;
//hi
class JobList {
public:
	JobList() {
		list = new vector<Job>();
	}
	void addJob(int jo, int p, string cmd, bool b) {
		Job j;
		j.jobID = jo;
		j.pID = p;
		j.command = cmd;
		j.bg = b;
		list->push_back(j);
	}
	void updateJob(int p) {
		for (auto it = list->begin(); it != list->end();) {
			if (kill((*it).pID, 0)) {
				list->erase(it);
			}
			++it;
		}
	}
	void removeJob() {
		list->pop_back();
	}
	void printLastJob() {
		cout<<"["<<list->back().jobID<<"] "<<list->back().pID<<" ";
		cout<<" "<<list->back().command<<endl;
	}
	void printJobs() {
		for (int i = 0; i < list->size(); i++) {
			cout<<"["<<list->at(i).jobID<<"] "<<list->at(i).pID<<" ";
			cout<<" ";
			if (list->at(i).bg) cout <<"&";
			cout<<list->at(i).command<<endl;
		}
	}
	int jobNum() {
		return list->size();
	}
private:
	struct Job {
		int jobID;
		int pID;
		string command;
		bool bg;
	};
	vector<Job> * list;
};

class Env {
public:
	Env(char * envp[]) {
	}
	char* getPath() {
		//cout<< path <<endl; CORRECT
		return getenv("PATH");
	}
	char* getHome() {
		return getenv("HOME");
	}
	char* getPWD() {
		return getenv("PWD");
	}
	void setPath(char* p) {
		setenv("PATH", p, 1);
	}
	void setHome(char* h) {
		setenv("HOME", h, 1);
	}
	void setPWD(char* d) {
		setenv("PWD", d, 1);
	}
};
//////////////GLOBALS
Env* env0;
JobList jobs;
//func to grab first word delimited by ' ' or '|'
char* firstWord(string cmd) {
	char* ret = new char[BSIZE];
	//cout<<"sizeof: "<<sizeof(ret)<<endl; //always 8
	bzero(ret, 32);
	int spacePos = cmd.find(' ');
	int pipePos = cmd.find('|');
	if (pipePos!=-1&&spacePos > pipePos) {
		ret = strdup(cmd.substr(0, pipePos).c_str());
	} else {
		ret = strdup(cmd.substr(0, spacePos).c_str());
	}
	return ret;
}

int pipeCount(string cmd) {
	int count = 0;
	for (int i = 0; i < cmd.size(); i++) {
		if (cmd[i] == '|') { count++; }
	}
	return count;
}

int wordCount(string cmd) {
	int count = 1;
	bool is = false;
	for (int i = 0; i < cmd.size(); i++) {
		if (cmd[i] == ' ' || cmd[i] == '|') {if (!is&&i!=cmd.size()-1) {count++; is = true;}}
		else {is = false;};
	}
	return count;
}

string beforeChar(string cmd, char a) {
	cmd = cmd.substr(0, cmd.find(a));
	while (cmd[cmd.size()-1] == ' ') {
		cmd = cmd.substr(0, cmd.size()-1);
	}
	return cmd;
}

string afterChar(string cmd, char a) {
	string ret = cmd.erase(0, cmd.find(a)+1);
	while (ret[0] == ' ') {
		ret = ret.erase(0, 1);
	}
	return ret;
}

void set(char* a) {
	string f = strsep(&a, "=");
	if (f == "HOME") {
		env0->setHome(firstWord(a));
	} else if (f == "PATH") {
		env0->setPath(firstWord(a));
	} else {
		cout<<"Arg not recognized. Try 'set HOME=/filepath' or 'set PATH=/filepath'.\n";
	}
}

void printenv(char* a) {
	string tmp2;
	//cout<<a<<endl;
	if (a) {
		tmp2 = beforeChar(a, ' ');
	} else {tmp2 = "";}
	if (tmp2.size()==0) {
		cout<<tmp2<<endl;
		cout<<"Env Variable not recognized...\n";
	} else if (tmp2 == "HOME" || tmp2 == "PATH" || tmp2 == "PWD") {
		cout<<tmp2<<"="<<getenv(tmp2.c_str())<<endl;
	} else {
		cout<<tmp2<<endl;
		cout<<"Env Variable not recognized...\n";
	}
}

void changeDir(string a) {
	if (a.size() == 0) {a = env0->getHome();}
	if (-1 == chdir(a.c_str())) {
		cout<<"Directory not found..."<<endl;
		return;
	}
	char a2[a.size()];
	strcpy(a2, a.c_str());
	env0->setPWD(a2);
}

int checkRedirect(char* cmd) {
	//cout<<"here\n";
	if (strchr(cmd, '<')) {
		return 2; //read from
	}
	if (strchr(cmd, '>')) {
		return 1; //write to
	}
	return 0;
}

void redirect(bool how, char* file) {
	//if true, write
	if (how) {
		//cout<<"new file "<<file<<endl;
		freopen(file, "w", stdout);
		//cout<<"test out";
	} else {
		//cout<<"new file "<<file<<endl;
		freopen(file, "r", stdin);
		//cout<<")))\n";
	}
	//cout<<file<<endl;
}

char** getList(char* cmd, char** list) {
	int i;
	for (i = 0; i < BSIZE; i++) {
		list[i] = strsep(&cmd, " ");
		//cout<<i<<":"<<list[i]<<endl;
		if (list[i] == NULL) {
			return list;
		} else if (strlen(list[i]) == 0) {i--;}
	}
}

char* redirecthandler(char* cmd) {
	int r = checkRedirect(cmd);
	if (r == 1) {
		char *pass = (char*) malloc(strlen(cmd));
		strcpy(pass, cmd);
		cmd = strsep(&pass, ">");
		cmd = strsep(&cmd, ">");
		pass+=1;

		redirect(true, pass);
	} else if (r == 2) {
		char *pass = (char*) malloc(strlen(cmd));
		strcpy(pass, cmd);
		cmd = strsep(&pass, "<");
		cmd = strsep(&cmd, "<");
		pass+=1;

		redirect(false, pass);
	}
	return cmd;
}

void sig_rm_job(int signo) {
	if (signo == SIGINT) {
		jobs.removeJob();
	}
}

int execute(char* cmd, char* envp[], bool bg) {
	int status;
	if (bg) {
		cmd = strsep(&cmd, "&");
	}
	string arg0 = firstWord(cmd);
	cmd = redirecthandler(cmd);
	struct sigaction sig;
	sigfillset(&sig.sa_mask);
	sig.sa_flags = 0;
	sig.sa_handler = sig_rm_job;
	sigaction(SIGINT,&sig,NULL)==0;
	if (arg0 == "quit" || arg0 == "exit") {
		exit(0); //quit safely
	}
	fflush(stdout);
	pid_t ppid = getpid();
	pid_t ret = fork();
	if (ret < 0) {
		cout<<"Fork failed...\n";
	} else if (ret == 0) {
		if (bg) {ret = setsid();}
		jobs.addJob(jobs.jobNum(), getpid(), arg0, bg);
		//cout<<"child\n"<<cmd<<endl;
		//cout<<"newcmd:"<<cmd<<endl;
		string arg0 = firstWord(cmd);
		if (arg0 == "set") {
			char* pass = strsep(&cmd, " ");
			set(cmd);
		} else if (arg0 == "jobs") {
			//jobs.printJobs();
			//todo
		} else if (arg0 == "printenv") {
			char* pass = strsep(&cmd, " ");
			printenv(cmd);
		} else if (arg0 == "cd") {
			/*string pass = cmd;
			pass = pass.erase(0,3);
			pass.shrink_to_fit();
			changeDir(pass);*/
		} else {
			extern char** environ;
			char ** chararray =  (char**) malloc (BSIZE);
			*(chararray+(wordCount(cmd)*8)) = (char*)0;
			//cout<<"here";
			//cout<<"char0: "<<chararray[wordCount(cmd)]<<endl; //here
			int count = wordCount(cmd);
			getList(cmd, chararray);
			//cout<<"here3"<<endl;
			//for (int i = 0; chararray[i]!=NULL; i++) {cout<<i<<":"<<chararray[i]<<"\n";}
			if (count <= 1) {
				if(-1 == execlp(arg0.c_str(), arg0.c_str(), NULL)){
					cout<<arg0<<endl;
					cout<<"Executable not found...\n";
					kill(ppid, SIGINT);
				}
			} else if (execvp(chararray[0], chararray)<0) {
				cout<<"Executable not found...\n";
				kill(ppid, SIGINT);
			}
			fclose(stdout);
		}
		exit(1);
	} else {
		jobs.addJob(jobs.jobNum(), ret, arg0, bg);
		if (bg) {jobs.printLastJob();}
		waitpid(ret, &status, 0);
		//jobs.updateJob(ret);
		//cout<<"status: "<<status<<endl;
		if (arg0 == "set") {
			char* pass = strsep(&cmd, " ");
			set(cmd);
		} else if (arg0 == "cd") {
			string pass = cmd;
			pass = pass.erase(0,3);
			pass.shrink_to_fit();
			changeDir(pass);
		} else if (arg0 == "jobs") {
			jobs.printJobs();
		}
	}
}

int executeP(char* before, char* cmd, char* envp[]) {
	cmd = redirecthandler(cmd);
	before = redirecthandler(before);
	while (cmd[0] == ' ') {char* pass = strsep(&cmd, " ");}
	//cout<<firstWord(cmd)<<endl;
	bool bg1 = strchr(before, '&');
	bool bg2 = strchr(cmd, '&');
	int fd[2];
	int status;
	pid_t pid, m;
	pipe(fd);
	m = fork();

	if (m < 0) {
		cout<<"Fork err\n";
	} else if (m == 0) {
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);
		execute(before, envp, bg1);
		exit(0);
	}
	pid = fork();
	if (pid < 0) {
		cout<<"Fork err\n";
	} else if (pid == 0) {
		close(fd[1]);
		dup2(fd[0], STDIN_FILENO);
		close(fd[0]);
		execute(cmd, envp, bg2);
		exit(0);
	}
	jobs.addJob(jobs.jobNum(), m, firstWord(before), bg1);
	if (bg1) {jobs.printLastJob();}
	jobs.addJob(jobs.jobNum(), pid, firstWord(cmd), bg2);
	if (bg2) {jobs.printLastJob();}
	close(fd[0]);
	close(fd[1]);
	if ((waitpid(m, &status, 0)) == -1) {
    fprintf(stderr, "Process 1 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }
	if ((waitpid(pid, &status, 0)) == -1) {
    fprintf(stderr, "Process 1 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }

}

void openIO(int argc, char *argv[]) {
	if (argc > 2) {
		if (argv[1] == ">") { //write in
			freopen(argv[2], "w", stdout);
			return;
		} else if (argv[1] == "<") { //read it
			freopen(argv[2], "r", stdin);
			return;
		} else {
			cout<<"Invalid argv[1]...\n";
			return;
		}
	} else if (argc == 2){
		cout<<"Not enough arguments...\n";
		return;
	} else {
		freopen ("/dev/tty", "a", stdout);
	}
}

int main(int argc, char *argv[], char *envp[]) {
	string cmds;

	env0 = new Env(envp);
	while(getline(cin, cmds)) {
		int i;
		openIO(argc, argv);
		while (cmds[0] == ' ') {
			cmds = cmds.erase(0,1);
		}
		while (cmds == "\n") {getline(cin, cmds);}
		char * cmd = (char*) malloc(cmds.size());
		strcpy(cmd, cmds.c_str());
		bool bg = strchr(cmd, '&');
		int check;
		fflush(stdout);
		fflush(stdin);
		//cout<<cmds<<endl;
		if (pipeCount(cmd) > 0) {
			char* before = strsep(&cmd, "|");
			check = executeP(before, cmd, envp);
		} else {
			check = execute(cmd, envp, bg);
		}

		wait(&i);
		if (check == 0) {
			return 0;
		}
	}

}
