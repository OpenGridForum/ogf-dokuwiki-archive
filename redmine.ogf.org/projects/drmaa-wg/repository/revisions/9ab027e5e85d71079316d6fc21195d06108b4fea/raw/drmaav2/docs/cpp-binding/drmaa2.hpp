
#include <set>
#include <map>
#include <vector>
#include <string>

using namespace std;

namespace drmaa2 
{
  enum job_state 
  {
    UNDETERMINED            =  0,
    QUEUED                  =  1,
    QUEUED_HELD             =  2,
    RUNNING                 =  3,
    SUSPENDED               =  4,
    REQUEUED                =  5,
    REQUEUED_HELD           =  6,
    DONE                    =  7,
    FAILED                  =  8
  };


  enum operating_system 
  {
    AIX                     =  0,
    BSD                     =  1,
    LINUX                   =  2,
    HPUX                    =  3,
    IRIX                    =  4,
    MACOS                   =  5,
    SUNOS                   =  6,
    TRUE64                  =  7,
    UNIXWARE                =  8,
    WI                      =  9,
    WINNT                   = 10,
    OTHER_OS                = 11
  };
  

  enum cpu_architecture 
  {
    ALPHA                   =  0,
    ARM                     =  1,
    CELL                    =  2,
    PARISC                  =  3,
    X86                     =  4,
    X64                     =  5,
    IA64                    =  6,
    MIPS                    =  7,
    PPC                     =  8,
    PPC64                   =  9,
    SPARC                   = 10,
    SPARC64                 = 11,
    OTHER_CPU               = 12
  };
  

  enum resource_limit_type 
  {
    CORE_FILE_SIZE          =  0,
    CPU_TIME                =  1,
    DATA_SEG_SIZE           =  2,
    FILE_SIZE               =  3,
    OPEN_FILES              =  4,
    STACK_SIZE              =  5,
    VIRTUAL_MEMORY          =  6,
    WALLCLOCK_TIME          =  7
  };
  

  enum job_template_placeholder 
  {
    HOME_DIRECTORY          =  0,
    WORKING_DIRECTORY       =  1,
    PARAMETRIC_INDEX        =  2
  };


  enum event_type 
  {
    NEW_STATE               =  0,
    MIGRATED                =  1,
    ATTRIBUTE_CHANGE        =  2
  };


  enum capability 
  {
    ADVANCE_RESERVATION     =  0,
    RESERVE_SLOTS           =  1,
    CALLBACK                =  2,
    BULK_JOBS_MAXPARALLEL   =  3,
    JT_EMAIL                =  4,
    JT_STAGING              =  5,
    JT_DEADLINE             =  6,
    JT_MAXSLOTS             =  7,
    JT_ACCOUNTINGID         =  8,
    RT_STARTNOW             =  9,
    RT_DURATION             = 10,
    RT_MACHINEOS            = 11,
    RT_MACHINEARCH          = 12
  };
  

  class  job;          
  class  job_array;    
  struct queue_info;
  struct machine_info;
  struct slot_info;
  class  reservation;  
  class  job_session;  


  #define  DRMAA2_ZERO_TIME      ((time_t)  0.0)
  #define  DRMAA2_INFINITE_TIME  ((time_t) -1.0)
  #define  DRMAA2_NOW            ((time_t) -2.0)


  class dict 
  {
    private:
      // we want to be inherited
      virtual ~dict (void);

    public:
      bool         has_attr                  (string name);
      string       get_attr                  (string name);
      void         set_attr                  (string name, string value);
      void         get_attr_description      (string name);

      set <string> list_mandatory_attributes (string name);
      set <string> list_optional_attributes  (string name);
  };


  struct job_info : public dict
  {
    string            jobId;
    long              exitStatus;
    string            terminatingSignal;
    string            annotation;
    job_state         jobState;
    string            jobSubState;
    vector <string>   allocatedMachines;
    string            submissionMachine;
    string            jobOwner;
    long              slots;
    string            queueName;
    time_t            wallclockTime;
    long              cpuTime;
    time_t            submissionTime;
    time_t            dispatchTime;
    time_t            finishTime;
  };


  struct slot_info // : public dict
  {
    string            machineName; 
    string            slots;
  };


  struct reservation_info: public dict
  {
    string            reservationId;
    string            reservationName;
    time_t            reservedStartTime;
    time_t            reservedEndTime;
    set <string>      usersACL;
    long              reservedSlots;
    set <slot_info>   reservedMachines;
  };


  struct job_template : public dict
  {
    string                remoteCommand;          
    vector <string>       args;          
    bool                  submitAsHold;          
    bool                  rerunnable;            
    map <string, string>  jobEnvironment;        
    string                workingDirectory;        
    string                jobCategory;            
    set <string>          email;            
    bool                  emailOnStarted;          
    bool                  emailOnTerminated;        
    string                jobName;              
    string                inputPath;            
    string                outputPath;            
    string                errorPath;            
    bool                  joinFiles;            
    string                reservationId;          
    string                queueName;            
    long                  minSlots;              
    long                  maxSlots;              
    long                  priority;              
    vector <string>       candidateMachines;  
    long                  minPhysMemory;            
    operating_system      machineOS;        
    cpu_architecture      machineArch;      
    time_t                startTime;          
    time_t                deadlineTime;        
    map <string, string>  stageInFiles;        
    map <string, string>  stageOutFiles;        
    map <string, string>  resourceLimits;      
    string                accountingId;          
  };


  struct reservation_template : public dict
  {
    string            reservationName;          
    time_t            startTime;          
    time_t            endTime;          
    time_t            duration;          
    long              minSlots;              
    long              maxSlots;              
    set <string>      usersACL;            
    vector <string>   candidateMachines;  
    long              minPhysMemory;            
    operating_system  machineOS;        
    cpu_architecture  machineArch;      
  };


  // we had to move job definition up, as it is needed for declaring the
  // notification struct.  Ugh.
  //
  class job 
  {
    public: 
      string       get_job_id         (void) const; // really const?
      job_session  get_session        (void) const; // really const?
      job_template get_template       (void) const; // really const?
                                      
      void         suspend            (void);  
      void         resume             (void); 
      void         hold               (void); 
      void         release            (void); 
      void         terminate          (void);  
                                      
      job_state    get_state          (string sub_state);
      job_info     get_info           (void);
      job          wait_started       (time_t timeout);
      job          wait_terminated    (time_t timeout);
  };


  struct notification : public dict
  {
    event_type        event;
    job               j;
    job_state         jobState;
  };


  struct queue_info : public dict
  {
    string            name;
  };


  struct version // : public dict // dict not needed
  {
    string            major; 
    string            minor;
  };


  struct machine_info : public dict
  {
    string            name;  
    bool              available;    
    long              sockets;      
    long              coresPerSocket;
    long              threadsPerCore;  
    double            load;  
    long              physMemory;
    long              virtMemory;    
    operating_system  machineOS;  
    version           machineOSVersion;
    cpu_architecture  machineArch;
  };    



  class exception                       : public std::exception    {  };
  class denied_by_drms_exception        : public drmaa2::exception {  };
  class drm_communication_exception     : public drmaa2::exception {  };
  class try_later_exception             : public drmaa2::exception {  };
  class session_management_exception    : public drmaa2::exception {  };
  class timeout_exception               : public drmaa2::exception {  };
  class internal_exception              : public drmaa2::exception {  };
  class invalid_argument_exception      : public drmaa2::exception {  };
  class invalid_session_exception       : public drmaa2::exception {  };
  class invalid_state_exception         : public drmaa2::exception {  };
  class out_of_resource_exception       : public drmaa2::exception {  };
  class unsupported_attribute_exception : public drmaa2::exception {  };
  class unsupported_operation_exception : public drmaa2::exception {  };


  class callback 
  {
    public:
      virtual void notify (notification notification);
  };

  class reservation_session 
  {
    public:
      string            get_contact         (void) const;
      string            get_session_name    (void) const;

      reservation       getReservation      (string               reservationId);
      reservation       requestReservation  (reservation_template reservationTemplate);
      set <reservation> getReservations     (void);
  };

  class reservation 
  {
    public:
      string               get_reservation_id       (void) const;
      reservation_session  get_session              (void) const;
      reservation_template get_reservation_template (void) const;

      reservation_info     getInfo                  (void);
      void                 terminate                (void);
  };


  class job_array 
  {
    public:
      string       get_job_array_id   (void) const;
      vector <job> get_jobs           (void) const;
      job_session  get_session        (void) const;
      job_template get_job_template   (void) const;

      void         suspend            (void);
      void         resume             (void);
      void         hold               (void);
      void         release            (void);
      void         terminate          (void);
  };


  class job_session 
  {
    public:
      string       get_contact        (void) const;
      string       get_session_name   (void) const;
      set <string> get_job_categories (void) const;

      vector <job> getJobs            (job_info       filter);
      job_array    getJobArray        (string         jobArrayId);
      job          runJob             (job_template   jobTemplate);

      job_array    runBulkJobs        (job_template   jobTemplate,
                                       long           beginIndex,
                                       long           endIndex,
                                       long           step,
                                       long           maxParallel);
      job          waitAnyStarted     (vector <job> jobs, time_t timeout);
      job          waitAnyTerminated  (vector <job> jobs, time_t timeout);
  };


  class monitoring_session 
  {
    public:
      set <reservation>  get_all_reservations (void);
      vector <job>       get_all_jobs         (job_info     filter);
      set <queue_info>   get_all_queues       (set <string> names);
      set <machine_info> get_all_machines     (set <string> names);
  };


  class session_manager 
  {
    public:
      string              get_drms_name             (void) const;
      version             get_drms_version          (void) const;
      version             get_drmaa_version         (void) const;

      bool                supports                  (capability          capability);
      job_session         createJobSession          (string              sessionName, 
                                                     string              contactString);
      reservation_session createReservationSession  (string              sessionName,  
                                                     string              contactString);
      monitoring_session  createMonitoringSession   (string              contactString);
      job_session         openJobSession            (string              sessionName);
      reservation_session openReservationSession    (string              sessionName);
      void                closeJobSession           (job_session         s);
      void                closeReservationSession   (reservation_session s);
      void                closeMonitoringSession    (monitoring_session  s);
      void                destroyJobSession         (string              sessionName);
      void                destroyReservationSession (string              sessionName);
      set <string>        getJobSessions            (void);
      set <string>        getReservationSessions    (void);
      void                registerEventNotification (callback            callback);
  };
};

