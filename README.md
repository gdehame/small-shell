# Mon Shell accepte les commandes internes suivantes
- ps: affiche les processus, j'ai choisis de n'afficher que les processus en cours (R) ou suspendu (T) et non les commandes internes
- env: affiche les variables d'environnement
- export: pour définir une variable d'environnement, mon shell n'accepte pas les variables avec espace, il prendra comme nom de variable le premier mot écrit
- exit: ferme le shell
- kill: envoie un signal à un processus, par défaut envoie le signal de terminaison si aucun signal n'est précisé, pour préciser un signal: `kill -s signal pid` et signal peut être SIGKILL, SIGCONT, SIGSTOP
- bg: relance le dernier processus suspendu
- nohup: sert à lancer un processus sans le lier au shell

# Mon Shell accepte également l'exécution de commandes et programmes externes en spécifiant un chemin entier ou après l'ajout d'une variable PATH.
On peut également lancer un processus en background en spécifiant `&` à la fin.
Mon Shell gère également les signaux d'interruption et d'arrêt lors de l'exécution d'un processus en avant plan afin de lui retransmettre plutôt que de fermer le shell ou de le mettre en pause
Par ailleurs j'utilise signal et non sigaction donc mon shell devrait fonctionner sur vos distributions mais je ne peux pas en être sûr.
