#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>
using namespace std;

#define NUMSIZE 32 // max char* size to read

void multiProcess(vector<vector<long long>> split, int n);
void multiThread(vector<vector<long long>> split, int n);
void *childThread(void *arg);
void readFile(const char* filename, vector<long long> &nums);
vector<vector<long long>> splitVector(vector<long long> &nums, int n);
void bubbleSort(vector<long long> &nums);
vector<long long> mergeSort(vector<vector<long long>> &sortedNums, int start, int end);
vector<long long> merge(vector<long long> &left, vector<long long> &right);
void printVector(vector<long long> &nums);


int main(int argc, char *argv[])
{
	int c, n;
	bool nflag = false, tflag = false;

	// no arguments after ./mysort
	if (argc<2){
		cerr<<"Wudao Ling (wudao) @UPenn \n";
		return -1;
	}

	// getopt() for command parsing
	while((c=getopt(argc,argv,"tn:"))!=-1){
		switch(c)
		{
		case 'n':
			nflag = true;
			n = atoi(optarg);
			break;
		case 't':
			tflag = true;
			break;

		default:
			cerr <<"Syntax: "<< argv[0] << " [-n process/thread number] [-t] \r\n";
			exit(1);
		}
	}
	// default number of process/thread
	if (!nflag) n = 4;

    // load from files
	vector<long long> nums;
	for (int k = optind; k< argc; k++){
		readFile(argv[k],nums);
	}

	// no concurrency when n = 1
	if (n==1){
		bubbleSort(nums);
		printVector(nums);
		return 0;
	}

	// break into N roughly equal parts
	vector<vector<long long>> split = splitVector(nums, n);

	if (!tflag){
		multiProcess(split,n);
	} else {
		multiThread(split,n);
	}
	return 0;
}

void multiProcess(vector<vector<long long>> split, int n){
    // fork N subprocesses and use pipe comm
    int upload[n][2];
    int download[n][2];
	vector<vector<long long>> sorted;

    for (int i = 0; i<n; i++){
        pipe(upload[i]);
        pipe(download[i]);
		pid_t pid = fork();
		if (pid<0){
			cerr<<"fork failed \n";
			exit(2);
		} else if (pid==0){
			// child process
			close(download[i][0]);
			close(upload[i][1]);

			// read vector from parent
			vector <long long> piece;
			char line[NUMSIZE];
			long long num;
            FILE* input = fdopen(upload[i][0],"r");
            while (fgets(line,NUMSIZE,input)!=NULL){
            	num = atoll(line);
            	piece.push_back(num);
            }
            fclose(input);

			// bubble sort
            bubbleSort(piece);

			// write sorted vector to parent
            FILE* output = fdopen(download[i][1],"w");
            for (int j = 0; j<piece.size();j++){
            	fprintf(output,"%lld\n",piece[j]);
            }
            fclose(output);

			exit(0);

		} else {
			// parent process
			close(download[i][1]);
			close(upload[i][0]);

			// write vector to child(s)
			// only writing to ensure parent wait before child exit
			FILE* output = fdopen(upload[i][1],"w");
			for (int j = 0; j<split[i].size();j++){
				fprintf(output,"%lld\n",split[i][j]);
			}
			fclose(output);
		}
    }

    // parent process continue
    // read sorted vector from child
    for (int i = 0; i<n;i++){
		vector <long long> piece;
		char line[NUMSIZE];
		long long num;
		FILE* input = fdopen(download[i][0],"r");
		while (fgets(line,NUMSIZE,input)!=NULL){
			num = atoll(line);
			piece.push_back(num);
		}
		sorted.push_back(piece);
		fclose(input);
    }

	// wait all child
	for (int i = 0; i<n;i++){
		wait(0);
	}

	//merge sort
	vector<long long> result = mergeSort(sorted,0,sorted.size()-1);
	printVector(result);
}

void multiThread(vector<vector<long long>> split, int n){
	pthread_t threads[n];
	int upload[n][2];
	int download[n][2];
	int Info[n][2];
	vector<vector<long long>> sorted;

	for (int i = 0;i<n;i++){
		// parent process
        pipe(upload[i]);
        pipe(download[i]);

        // necessary pipe info for child thread
        Info[i][0] = upload[i][0];
        Info[i][1] = download[i][1];

		pthread_create(&threads[i],NULL,childThread,&(Info[i]));

		// write vector to child(s)
		FILE* output = fdopen(upload[i][1],"w");
		for (int j = 0; j<split[i].size();j++){
			fprintf(output,"%lld\n",split[i][j]);
		}
		fclose(output);
	}

	// read sorted vector from child
	for (int i = 0;i<n;i++){
		vector <long long> piece;
		char line[NUMSIZE];
		long long num;
		FILE* input = fdopen(download[i][0],"r");
		while (fgets(line,NUMSIZE,input)!=NULL){
			num = atoll(line);
			piece.push_back(num);
		}
		sorted.push_back(piece);
		fclose(input);
	}

	// wait all child
	for (int i = 0;i<n;i++){
		pthread_join(threads[i],NULL);
	}

	// merge sort
	vector<long long> result = mergeSort(sorted,0,sorted.size()-1);
	printVector(result);
}

void *childThread(void *arg){
	// load pipe info from arg
	int* pipeInfo = (int*)arg;
    int childRead = pipeInfo[0];
    int childWrite = pipeInfo[1];

	// read vector from parent
	vector <long long> piece;
	char line[NUMSIZE];
	long long num;
    FILE* input = fdopen(childRead,"r");
    while (fgets(line,NUMSIZE,input)!=NULL){
    	num = atoll(line);
    	piece.push_back(num);
    }
    fclose(input);

	// bubble sort
    bubbleSort(piece);

	// write sorted vector to parent
    FILE* output = fdopen(childWrite,"w");
    for (int j = 0; j<piece.size();j++){
    	fprintf(output,"%lld\n",piece[j]);
    }
    fclose(output);

	pthread_exit(NULL);
}

void readFile(const char* filename, vector<long long> &nums){
	// assumption: txt files with int64 (separate line), <1m numbers
	ifstream ifs(filename); //constructor
	char line[NUMSIZE];
	long long num;
	if (!ifs.is_open()){
		cerr << "unable to open file\n";
		exit(1);
	}

	while (!ifs.eof()){
		ifs>>line;
		if (ifs.eof()) break; // prevent eof() problem at last line
		num = atoll(line);
		nums.push_back(num);
	}
    ifs.close();
}

void bubbleSort(vector<long long> &nums){
	int n = nums.size();
	if (n<=0) return;
	for (int i = 0; i < n-1; i++) // secure last i elements
		for (int j = 0; j < n-1-i; j++)
			if (nums[j] > nums[j+1])
				swap(nums[j],nums[j+1]);
}

vector<vector<long long>> splitVector(vector<long long> &nums, int n){
	// roughly divide into n parts, size+-1
	vector<vector<long long> > result;
	int base = nums.size()/n;
	int remain = nums.size()%n;

	int begin = 0, end = 0;
	for (int i = 0; i<n; i++){
		if (remain>0) {end+=base+1;remain--;}
		else {end+=base;}
		result.push_back(vector<long long>(nums.begin()+begin,nums.begin()+end));
		begin = end;
	}
    return result;
}

vector<long long> mergeSort(vector<vector<long long>> &sortedNums, int start, int end){
	if (start>end) {cerr << "error in mergesort indexing"; exit(3);}
	if (start==end) return sortedNums[start];
	int mid = (start+end)/2;
	vector<long long> left = mergeSort(sortedNums, start, mid);
	vector<long long> right = mergeSort(sortedNums, mid+1, end);
	return merge(left,right);
}

vector<long long> merge(vector<long long> &left, vector<long long> &right){
	vector<long long> result;
	int i = 0, j = 0;
	while(i<left.size()&&j<right.size()){
		if (left[i]<right[j])
			result.push_back(left[i++]);
		else
			result.push_back(right[j++]);
	}
	while(i<left.size())
		result.push_back(left[i++]);
	while(j<right.size())
		result.push_back(right[j++]);
	return result;
}

void printVector(vector<long long> &nums){
	for (auto num:nums)
		cout << num << endl;
}
