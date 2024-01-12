#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "shell.h"
#include <errno.h>

int occupe = 0;
int psCourant = 0;
int end = 0;

listP * procL;
listP * procSuspendus;

listV * varL;

int main()
{
  //On gère à notre facon les signaux d'interruption, d'arret, des enfants
  signal(SIGINT, quickKillPs);
  signal(SIGTSTP, quickStopPs);
  signal(SIGCHLD, child_handler);
  //On commence par créé le process du shell
  int shellId = getpid();
  ps * shellPs = (ps *) malloc (sizeof(ps));
  shellPs->id = shellId;
  shellPs->ppid = shellId;
  shellPs->cmd = "./shell\0";
  shellPs->status = "R\0";

  char * input;
  //On alloue la mémoire pour nos listes
  procL = (listP *) malloc (sizeof(listP));
  procL->process = shellPs;
  varL = (listV *) malloc (sizeof(listV));
  procSuspendus = (listP *) malloc (sizeof(listP));

  while (!end)
  {
    printf("> "); //On met le symbole d'attente d'input
    input = readL(); //On lit une ligne dont la taille est inconnue
    if (input != NULL)
    {
      end = execute(input); //On analyse la ligne
    }
  }
  return 0;
}

void addPs(ps * process) //Ajoute un processus à la liste
{
  listP * p = procL;
  while (p->next != NULL)
  {
    p = p->next;
  }
  listP * new = (listP *) malloc (sizeof(listP));
  new->process = process;
  new->previous = p;
  p->next = new;
}

void addVar(var * newV) //Ajoute une variable à l'environnement
{
  listV * p = varL;
  if (p->v == NULL)
  {
    p->v = newV;
  }
  else
  {
    while (p->next != NULL)
    {
      if (strcmp(p->v->name, newV->name) == 0)
      {
        p->v = newV;
        return;
      }
      p = p->next;
    }
    if (strcmp(p->v->name, newV->name) == 0)
    {
      p->v = newV;
      return;
    }
    listV * new = (listV *) malloc (sizeof(listV));
    new->v = newV;
    p->next = new;
    new->previous = p;
  }
}

void printPs() //Affiche les processus
{
  listP * p = procL;
  while(p != NULL && p->process != NULL)
  {
    printf("id: %d | ppid: %d | cmd: %s | status: %s\n", p->process->id, p->process->ppid, p->process->cmd, p->process->status);
    p = p->next;
  }
}

void printEnv() //Affiche l'environnement
{
  listV * p = varL;
  while(p != NULL && p->v != NULL && p->v->name != NULL && p->v->val != NULL)
  {
    printf("%s = %s\n", p->v->name, p->v->val);
    p = p->next;
  }
}

int havePATH() //Regarde si il y a une variable PATH définie
{
  listV * p = varL;
  while (p != NULL)
  {
    if (p->v != NULL && p->v->name != NULL && !strcmp("PATH", p->v->name))
    {
      return 1;
    }
    p = p->next;
  }
  return 0;
}

char * parsePATH(char * file) //Parse la variable PATH pour exécuter le fichier file si il est dans un des répertoire de PATH
{
  struct stat s;
  stat(file, &s);
  if ((s.st_mode & S_IFMT) == S_IFREG && !access(file, X_OK))
  {
    return file;
  }
  else if (file[0] != '/')
  {
    listV * p = varL;
    while (p != NULL)
    {
      if (p->v != NULL && p->v->name != NULL && !strcmp("PATH", p->v->name))
      {
        break;
      }
      p = p->next;
    }
    char * path = (p->v->val);
    char * directory = strtok(path, ":");
    int len = strlen(file);
    do
    {
      int n = strlen(directory);
      char * fileDest = (char *) malloc (n + len + 2);
      int i = 0;
      fileDest[n+len+1] = '\0';
      while (i < n + len + 1)
      {
        if (i < n)
        {
          fileDest[i] = directory[i];
        }
        else if (i == n)
        {
          fileDest[i] = '/';
        }
        else
        {
          fileDest[i] = file[i-n-1];
        }
        i++;
      }
      stat(fileDest, &s);
      if ((s.st_mode & S_IFMT) == S_IFREG && !access(fileDest, X_OK))
      {
        return fileDest;
      }
      free(fileDest);
      directory = strtok(NULL, ":");
    }
    while (directory != NULL);
    printf("L'executable n'est pas accessible via le PATH\n");
  }
  else
  {
    printf("Le fichier n'existe pas ou n'est pas exécutable\n");
  }
  return NULL;
}

char ** env_array() //Transforme la liste des variables d'environnement en tableau
{
  listV * p = varL;
  int card = 1;
  while (p != NULL)
  {
    card++;
    p = p->next;
  }
  char ** envAr = (char**) malloc (sizeof(char*)*card);
  p = varL;
  char * eg = "=";
  card--;
  envAr[card] = NULL;
  for (int i = 0; i < card; i++)
  {
    int k = strlen(p->v->name);
    int n = strlen(p->v->val) + k + 1;
    char * variable = (char *) malloc (n);
    memcpy(variable, p->v->name, k);
    memcpy(variable + k, eg, 1);
    memcpy(variable + k + 1, p->v->val, strlen(p->v->val));
    envAr[i] = variable;
  }
  return envAr;
}

char * readL() //Récupère une ligne
{
  char * line = (char *) malloc (1024);
  int ind = 0;
  int size = 1024;
  char c = getchar();
  while (c != EOF && c != '\n') //On récupère caractère par caractère la ligne et réalloue notre ligne si jamais l'input est trop long
  {
    line[ind] = c;
    ind++;
    if (ind == size)
    {
      size *= 2;
      line = realloc(line, size);
    }
    c = getchar();
  }
  line[ind] = '\0';
  if (ind == 0) //Si c'est le cas c'est que l'utilisateur a juste appuyé sur entrée sans écrire
  {
    return NULL;
  }
  return line;
}

void supprimerPs(int pid) //Supprime le processus dont l'id des pid de la list procL
{
  listP * p = procL;
  while (p != NULL && p->process != NULL)
  {
    if (p->process->id == pid)
    {
      p->previous->next = p->next;
      if (p->next != NULL)
      {
        p->next->previous = p->previous;
      }
      free(p->process);
      free(p);
      break;
    }
    p = p->next;
  }
}

void killPs(int pid, char * action) // Envoie un signal au processus d'id pid en fonction de l'action demandée
{
  if (strcmp(action, "SIGSTOP") == 0)
  {
    ps * proc = findPID(pid);
    if (proc == NULL)
    {
      printf("Echec d'arrêt d'un processus qui n'existe pas (%d)\n", pid);
    }
    else
    {
      proc->status = "T\0";
      listP * nouvSus = (listP *) malloc (sizeof(listP));
      nouvSus->process = proc;
      if (procSuspendus == NULL)
      {
        procSuspendus = nouvSus;
      }
      else
      {
        nouvSus->next = procSuspendus;
        procSuspendus->previous = nouvSus;
        procSuspendus = nouvSus;
      }
      kill(pid, SIGSTOP);
    }
  }
  else if (strcmp(action, "SIGTERM") == 0)
  {
    kill(pid, SIGTERM);
    supprimerPs(pid);
  }
  else if (strcmp(action, "SIGCONT") == 0)
  {
    ps * proc = findPID(pid);
    proc->status = "R\0";
    kill(pid, SIGCONT);
  }
  else
  {
    printf("Signal inconnu\n");
  }
}

ps * findPID(int candidat) //Recherche le processus d'id candidat, si il existe, le renvoie, sinon renvoie NULL
{
  listP * p = procL;
  while (p != NULL)
  {
    if (p->process != NULL && p->process->id == candidat)
    {
      return p->process;
    }
    p = p->next;
  }
  return NULL;
}

int execute(char * line)
{
  char * fullline = (char*) malloc (strlen(line));
  strcpy(fullline, line); //On conserve la ligne si jamais on a besoin de la préciser pour la création d'un nouveau processus
  char * cmd = strtok(line, " "); //On récupère la commande
  if (strcmp(cmd, "exit") == 0) //Si c'est exit, on ferme le shell
  {
    closeShell();
    return 1;
  }
  else if (strcmp(cmd, "ps") == 0) //Si c'est ps on affiche les processus
  {
    printPs(procL);
  }
  else if (strcmp(cmd, "env") == 0) //Si c'est env on affiche l'environnement
  {
    printEnv();
  }
  else if (strcmp(cmd, "export") == 0) //Si c'est export on récupère le nom de la variable puis la valeur et on retire les potentiels espace afin d'accepter les déclarations de la forme VAR = VAL
  {
    var * new = (var *) malloc (sizeof(var));
    char * nom = strtok(NULL, "=");
    char * valeur = strtok(NULL, "\n");
    if (valeur == NULL)
    {
      printf("Definition invalide (Nom=valeur)\n");
      free(new);
    }
    else
    {
      int ind = 0;
      while (valeur[ind] == ' ' && ind < (int) strlen(valeur))
      {
        ind++;
      }
      char * v = (char *) malloc (strlen(valeur) - ind);
      memcpy(v, valeur+ind, strlen(valeur)-ind);
      new->val = v;
      new->name = strtok(nom, " ");
      addVar(new);
    }
  }
  else if (strcmp(cmd, "kill") == 0) //On envoie un signal à un processus enfant
  {
    char * arg1 = strtok(NULL, " ");
    if (strcmp(arg1, "-s") == 0) //On regarde si la commande est de la forme kill -s signal pid
    {
      char * arg2 = strtok(NULL, " ");
      char * arg3 = strtok(NULL, " ");
      if (arg2 == NULL || arg3 == NULL)
      {
        printf("Pas assez d'arguments\n");
      }
      else
      {
        int argID = strtol(arg3, NULL, 10); //On convertit le pid sous forme de char * en entier
        if (argID == 0 || findPID(argID) == NULL) //Si argID = 0 c'est que l'argument 3 n'est pas un entier, alors les arguments n'ont pas été mis dans le bon ordre ou on n'a pas écrit un entier pour le pid
        {
          printf("Le PID est invalide ou les arguments ne sont pas dans le bon ordre (kill -s signal pid)\n");
        }
        else
        {
          killPs(argID, arg2);
        }
      }
    }
    else if (strtok(NULL, " ") != NULL)
    {
      printf("Mauvais arguments\n");
    }
    else // Par défaut, on SIGTERM
    {
      int argID = strtol(arg1, NULL, 10);
      if (argID == 0 || findPID(argID) == NULL)
      {
        printf("Le PID est invalide ou les arguments ne sont pas dans le bon ordre (`kill -s signal pid` ou `kill pid`)\n");
      }
      else
      {
        killPs(argID, "SIGTERM");
      }
    }
  }
  else if (strcmp(cmd, "bg") == 0) //Relance le dernier processus suspendu
  {
    if (procSuspendus == NULL)
    {
      printf("Aucun process suspendu\n");
    }
    else
    {
      //On dépile la pile des processus suspendus
      int dernierSuspendu = procSuspendus->process->id;
      procSuspendus->process->status = "R\0";
      listP * ps = procSuspendus->next;
      procSuspendus = ps;
      if (ps != NULL && ps->previous != NULL) free(ps->previous);
      killPs(dernierSuspendu, "SIGCONT");
    }
  }
  else if (strcmp(cmd, "nohup") == 0) //On récupère la commande qui suit le nohup puis l'execute sans ajouter le processus créé à la liste des processus afin qu'il soit indépendant du shell
  {
    char * args[256];
    char * p = strtok(NULL, " ");
    int i = 0;
    while (p != NULL)
    {
      args[i] = p;
      i++;
      p = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (!havePATH() && args[0][0] != '/')
    {
      printf("Veuillez définir une variable PATH pour utiliser des commandes externes (export PATH=...)\n");
    }
    else
    {
      args[0] = parsePATH(args[0]);
      if (args[0] == NULL)
      {
        return 0;
      }
      int pid = fork();
      if (pid < 0)
      {
        printf("Erreur dans le fork\n");
        closeShell();
        return 1;
      }
      else if (pid == 0)
      {
        signal(SIGINT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        setpgid(0,0);
        if (varL->v == NULL) //execve ne fonctionne pas si l'environnement est vide donc on doit vérifier s'il existe une variable d'environnement ou non
        {
          execv(args[0], args);
        }
        execve(args[0], args, env_array());
      }
    }
  }
  else if (!havePATH() && cmd[0] != '/') //Sinon, si il n'y a pas de variable PATH et qu'on ne donne pas un chemin entier, on affiche une erreur
  {
    printf("Veuillez définir une variable PATH pour utiliser des commandes externes (export PATH=...)\n");
  }
  else //Sinon, on récupère la commande et les arguments
  {
    char * args[256];
    char * p = strtok(NULL, " ");
    int i = 1;
    args[0] = cmd;
    while (p != NULL)
    {
      args[i] = p;
      i++;
      p = strtok(NULL, " ");
    }
    int bg = 0;
    if (strcmp(args[i-1], "&") == 0) //On regarde si on cherche à l'executer en background ou non
    {
      args[i-1] = NULL;
      bg = 1;
    }
    args[i] = NULL;

    args[0] = parsePATH(args[0]);
    if (args[0] == NULL)
    {
      return 0;
    }
    else
    {
      int pid = fork();
      if (pid < 0)
      {
        printf("Erreur dans le fork\n");
        closeShell();
        return 1;
      }
      else if (pid == 0) //Si on est le fils on lance le processus
      {
        signal(SIGINT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        if (bg) //Si on execute en background on change l'id du groupe de processus pour ne pas recevoir les CTRCL+C/CTRL+Z
        {
          setpgid(0,0);
        }
        if (varL->v == NULL) //execve ne fonctionne pas si l'environnement est vide donc on doit vérifier s'il existe une variable d'environnement ou non
        {
          execv(args[0], args);
        }
        execve(args[0], args, env_array());
      }
      else //Si on est le père, on enregistre le processus fils créé dans la liste des processus et si on n'a pas demandé à le lancer en background on attend qu'il soit fini avant de continuer
      {
        ps * proc = (ps *) malloc(sizeof(ps));
        proc->id = pid;
        proc->ppid = getpid();
        proc->cmd = fullline;
        proc->status = "R\0";
        addPs(proc);
        if (!bg)
        {
          occupe = 1;
          psCourant = pid;
      		while (occupe)
          {
            if(end)
            {
              return 1;
            }
          }
        }
      }
    }
  }
  return 0;
}

void quickKillPs() // Tue le processus courant lors d'un CTRL+C
{
  signal(SIGINT, quickKillPs);
  if(occupe)
  {
    killPs(psCourant, "SIGTERM");
    occupe = 0;
    printf("\n");
  }
  else
  {
    printf("\n> ");
    fflush(stdout);
  }
}

void quickStopPs() // Arrete le processus courant lors d'un CTRL+Z
{
  signal(SIGTSTP, quickStopPs);
  if(occupe)
  {
    killPs(psCourant, "SIGSTOP");
    occupe = 0;
    printf("\n");
  }
  else
  {
    printf("\n> ");
    fflush(stdout);
  }
}

void checkChild(int pid, int status) // Analyse le signal recu d'un fils pour savoir s'il a été arrêté, s'il a été tué ou si il a été continué
{
  if (pid < 0)
  {
    if (pid == -1)
    {
      printf("Erreur dans le waitpid (%d)\n", errno);
    }
    else
    {
      occupe = 0;
      printf("Erreur dans les processus enfants (%d)\n", pid);
      closeShell();
      end = 1;
    }
  }
  else if (pid > 0)
  {
    if (WIFEXITED(status))
    {
      if(pid == psCourant)
      {
        occupe = 0;
      }
      killPs(pid, "SIGTERM");
    }
    else if (WIFSTOPPED(status))
    {
      if(pid == psCourant)
      {
        occupe = 0;
      }
      killPs(pid, "SIGSTOP");
    }
    else if (WIFCONTINUED(status))
    {
      killPs(pid, "SIGCONT");
    }
  }
}

void child_handler() // Analyse tous les signaux recus par les fils
{
  signal(SIGCHLD, child_handler);
  int pid;
  int status;
  while((pid = waitpid(-1, &status, WNOHANG)) > 0)
  {
    checkChild(pid, status);
  }
}

void closeShell() //Eteint le shell
{
  listP * p = procL->next;
  while (p != NULL && p->process != NULL)
  {
    kill(p->process->id, SIGTERM);
    free(p->process);
    p = p->next;
    if ( p != NULL && p->previous != NULL)
    {
      free(p->previous);
    }
  }
  free(p);
  listV * pv = varL;
  while (pv != NULL && pv->v != NULL)
  {
    free(pv->v);
    pv = pv->next;
    if (pv != NULL && pv->previous != NULL)
    {
      free(pv->previous);
    }
  }
  free(pv);
}
