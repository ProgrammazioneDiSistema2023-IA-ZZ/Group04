# Group04

## Progetto 1.1 

### Sezione 1 - Analisi Comparativa

Il nostro team ha selezionato il sistema operativo didattico MentOS per effettuare un confronto con OS161 per quanto riguarda la gestione della memoria e dei processi.  
Le slides inerenti a questa parte sono:
* MentOS - Introduzione
* MentOS - Process Management
* MentOS - Gestione Della Memoria

Le slides sono disponibili sia in formato __pptx__ nella cartella principale, sia in formato **pdf** nella cartella **slides_pdf**.  

### Sezione 2 - Implementazione

Per la seconda parte del progetto, abbiamo scelto di implementare il **buddy system** nel codice di MentOS, per la gestione della memoria.  
Abbiamo inoltre implementato gli **algoritmi di scheduling** per cui il sistema forniva già un supporto ed integrato, in aggiunta a questi, l'algoritmo LLF.  
Per ulteriori informazioni su questa parte, si rimanda alle slides (anch'esse disponibili sia in pdf sia in formato pptx) e al codice stesso:
* MentOS - Implementazione Buddy System
* MentOS - Implementazione Scheduling Algorithms
* scheduler_algorithm.c
* buddysystem.c

 ### Ulteriore Materiale
Nella repository è presente un file **zip** contenente la versione modificata di MentOS con i cambiamenti apportati.  
La repository contiene inoltre il file immagine di un sistema Ubuntu con all'interno la versione di MentOS sopra indicata.  
L'account e la password di Ubuntu sono entrambe **politoPDS**. 
I file sono contenuti all'interno della cartella **home/PDS_Project**  
Per quanto riguarda la compilazione del sistema, ci limitiamo a riportare il link alla documentazione ufficiale: https://mentos-team.github.io/doc/doxygen/index.html. Si noti che tutti i programmi necessari alla compilazione sono già installati nella macchina.  
Per accedere a MentOS, una volta compilato, si possono usare i seguenti username:password
* root : root
* user : user 

(Per eseguire i test non vi è differenza tra un account e l'altro).  

I comandi dei test implementati/utilizzati per la verifica del funzionamento del codice realizzato sono:
* __memtest X__ dove X è un numero intero: effettua X allocazioni/deallocazioni di dimensione crescente, consente di testare il buddysystem
* __/bin/tests/t_fork X__ dove X è un numero intero: effettua X fork del processo, consentendo di testare sia buddy system sia (parzialmente) gli algoritmi di scheduling
* __/bin/tests/t_periodic1__ crea una task periodica che a sua volta ne invoca altre due, periodic2 e periodic3. Queste due effettureanno 10 stampe, e periodic3 deve concludersi prima di periodic2 in quanto avente un periodo minore. Periodic1 proseguirà a stampare (numeri da 0 a 9, in maniera ciclica). Serve a testare gli algoritmi di scheduling real time con funzioni periodiche 



