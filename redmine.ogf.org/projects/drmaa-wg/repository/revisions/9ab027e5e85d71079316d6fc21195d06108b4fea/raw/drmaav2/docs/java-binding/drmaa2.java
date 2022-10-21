// Design decisions:
// - Minimum is Java 1.5 (to have enumerations)
// - Structures on IDL module scope become part of session manager
// - IDL structs become Java classes with only attributes
// - IDL interfaces become abstract Java interfaces

package org.ogf.drmaa2;

public enum JobState {
	UNDETERMINED, QUEUED, QUEUED_HELD, RUNNING, SUSPENDED, REQUEUED, 
	REQUEUED_HELD, DONE, FAILED};
public enum OperatingSystem {
	AIX, BSD, LINUX, HPUX, IRIX, MACOS, SUNOS, TRUE64, UNIXWARE, WIN,
	WINNT, OTHER_OS};
public enum CpuArchitecture {
	ALPHA, ARM, CELL, PARISC, X86, X64, IA64, MIPS, PPC, PPC64, 
	SPARC, SPARC64, OTHER_CPU};
public enum ResourceLimitType {
	CORE_FILE_SIZE, CPU_TIME, DATA_SEG_SIZE, FILE_SIZE, OPEN_FILES,  
	STACK_SIZE, VIRTUAL_MEMORY, WALLCLOCK_TIME };
public enum JobTemplatePlaceholder {
	HOME_DIRECTORY,WORKING_DIRECTORY,PARAMETRIC_INDEX };
public enum DrmaaEvent {
	NEW_STATE, MIGRATED, ATTRIBUTE_CHANGE
};
public enum DrmaaCapability {
	ADVANCE_RESERVATION, RESERVE_SLOTS, CALLBACK, BULK_JOBS_MAXPARALLEL, 
	 JT_EMAIL, JT_STAGING, JT_DEADLINE, JT_MAXSLOTS, JT_ACCOUNTINGID, 
	RT_STARTNOW, RT_DURATION, RT_MACHINEOS, RT_MACHINEARCH
};

public class JobInfo {
	String jobId;
	long exitStatus;
	String terminatingSignal;
	String annotation;
	JobState jobState;
	any jobSubState;
	OrderedSlotInfoList allocatedMachines;
	String submissionMachine;
	String jobOwner;
	long slots;
	String queueName;
	TimeAmount wallclockTime;
	long cpuTime;
	AbsoluteTime submissionTime;
	AbsoluteTime dispatchTime;
	AbsoluteTime finishTime;
};

public abstract interface SessionManager{
}

