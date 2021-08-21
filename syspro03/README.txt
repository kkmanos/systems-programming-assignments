
ΕΡΓΑΣΙΑ 3 - ΠΡΟΓΡΑΜΜΑΤΙΣΜΟΣ ΣΥΣΤΗΜΑΤΟΣ
ΚΟΥΚΟΥΛΑΡΗΣ ΕΜΜΑΝΟΥΗΛ - sdi1700262

    Μεταγλώττιση και εκτέλεση
    ~~~~~~~~~~~~~~~~~~~~~~~~~

    Για μεταγλώττιση εκτελείτε στον κατάλογο src/ την εντολή
    
    $ make
    
    και θα παραχθούν τα εκτελέσιμα αρχεία travelMonitorClient και 
    monitorServer.

    Για εκτέλεση, ως ορίσματα χρησιμοποιούνται αυτά που ζητούνται απο
    την εκφώνηση.

    ./travelMonitorClient –m numMonitors -b socketBufferSize -c cyclicBufferSize -s sizeOfBloom -i input_dir -t numThreads


    Σημαντικά αρχεία
    ~~~~~~~~~~~~~~~~

    Τα αρχεία src/travelMonitorClient.c και src/monitor/monitorServer.c
    περιέχουν τις main() συναρτήσεις για το travelMonitorClient και
    monitorServer εκτελέσιμο αντίστοιχα.

    Στον κατάλογο src/monitor/ περιέχονται όλα τα αρχεία που υλοποιούν
    λειτουργίες του monitorServer.

    Στον κατάλογο src/travel_monitor/ περιέχονται όλα τα αρχεία που
    υλοποιούν λειτουργίες του travelMonitorClient.

    Στον κατάλογο src/cyclic_buffer/ περιέχεται η υλοποίηση
    για τον κυκλικό buffer που ζητείται για το shared memory στο
    multi-threading.

    Στον κατάλογο src/sockets έχουν υλοποιηθεί λειτουργίες για τα 
    sockets, όπως connection_create() , connection_close() , κλπ


    TCP connection με monitorServer
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Δομή connection_t (στον κατάλογο src/sockets/)

typedef struct connection_t { // is used by a client 
    int client_sock;
    int server_port;
    char *server_hostname;
    struct sockaddr_in server;
} connection_t;

    Για την σύνδεση του travelMonitorClient με τον monitorServer 
    δημιουργείται ένας πίνακας απο δείκτες σε αντικείμενα της παραπάνω
    δομής.


    O monitorServer απο την άλλη έχει περιοριστεί να δέχεται μόνο ένα
    connection για λόγους τερματισμού της άσκησης.


    Multi-threading και Cyclic-buffer
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Για την υλοποίηση του cyclic buffer, υλοποίησα μια ουρά προτεραιότητας.
    Για τον συγχρονισμό του των threads μεταξύ τους και με το monitorServer
    χρησιμοποίησα τη βιβλιοθήκη των σημαφόρων. Πιο συγκεκριμένα, δημιούργησα
    3 σημαφόρους:
    
    sem_t mutex, empty, full;
    όπου o mutex είναι αρχικοποιημένος σε 1,
    o empty σε CYCLIC_BUFFER_SZ και o full σε 0.

    Το πρώτυπο χρησιμοποιήθηκε για τον συγχρονισμό,
    είναι αυτό του προβλήματος producers-consumer.

    όπου ο βασικός κώδικας είναι:

producer:
    do {
        wait(empty);
        wait(mutex);
        ... προσθήκη της next_produced στο cyclic buffer ...
        signal(mutex);
        signal(full);
    } while (1);

consumer:
    do {
        wait(full);
        wait(mutex);
        ... αφαίρεση ενός αντικειμένου απο το cyclic buffer ...
        signal(mutex);
        signal(empty);
    } while (1);


    Δεν έγινε καμία χρήση κώδικα απο το διαδίκτυο.
    Το πρώτυπο αυτό είναι διατυπωμένο στο βιβλίο.



    Ο κώδικας του κάθε thread βρίσκεται στη συνάρτηση
    thread_i() που είναι υλοποιημένη στο αρχείο
    monitorServer.c και παίρνει ως είσοδο έναν δείκτη
    σε δομή monitor_t η οποία περιέχει δείκτες σε σημαντικές
    δομές που είναι η μνήμη του monitorServer.
    
    
    
    Σημειώσεις
    ~~~~~~~~~~
    
		1. Στην περίπτωση που επιλεγεί η δημιουργία ενός monitorServer μόνο, 
	τότε προτείνεται να δεσμευθούν ~10 threads ή παραπάνω. Ο λόγος για
	τον οποίο γίνεται αυτή η πρόταση, είναι επειδή παρατηρείται μια
	περίεργη συμπεριφορά, όπου δεν καταφέρνει το monitorServer να τελειώσει
	την διαδικασία αρχικοποίησης με αποτέλεσμα να μήν είναι ποτέ διαθέσιμο
	για σύνδεση. Παρόλο που προσπάθησα να διορθώσω αυτό το πρόβλημα, δεν
	τα κατάφερα διότι δεν βρήκα σημείο που να το προκαλεί, αφού και ο κώδικας
	είναι πανομοιότυπος με αυτόν της 2ης εργασίας όπου στο επίπεδο της
	αρχικοποίησης όλα λειτουργούσαν χωρίς κάποια περίεργη συμπεριφορά.
	
	Το πρόγραμμα δοκιμάστηκε στα μηχανήματα της σχολής.
	



