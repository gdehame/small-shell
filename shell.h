typedef struct ps
{
  int id;
  int ppid; // id du parent
  char * cmd; //commande qui a créé le process
  char * status;
} ps;

typedef struct var
{
  char * name;
  char * val;
} var;

typedef struct l
{
  struct l * next;
  ps * process;
  struct l * previous;
} listP;

typedef struct lv
{
  struct lv * next;
  var * v;
  struct lv * previous;
} listV;

void addPs(ps * process);

void addVar(var * newV);

void printPs();

void printEnv();

int havePATH();

char * parsePATH(char * file);

char ** env_array();

char * readL();

void supprimerPs(int pid);

void killPs(int pid, char * action);

ps * findPID(int candidat);

int execute(char * line);

void quickKillPs();

void quickStopPs();

void child_handler();

void checkChild(int pid, int status);

void closeShell();
