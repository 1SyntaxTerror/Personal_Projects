IMPORTANT: Folderul Proiect_FINAL conține toate funcționalitățile din Proiect2 + Erori mai detaliate la autentificare.

Funcționalități principale

Serverul

    	Ascultă pe un port TCP definit (ex: 9091).

    	Acceptă conexiuni multiple folosind poll() (Linux) sau echivalent.

    	Pentru fiecare client conectat:

        	Primește nickname-ul (și parola în varianta cu autentificare).

        	În varianta cu autentificare, verifică nickname-ul și parola într-o listă fixă de utilizatori validați.

        	Verifică dacă nickname-ul este deja folosit de alt client conectat (doar în varianta cu autentificare).

        		Dacă autentificarea e validă, clientul este acceptat și se adaugă în lista de clienți.

        		Dacă autentificarea e invalidă, conexiunea este închisă.

   		Primește mesaje de la clienți și le retransmite tuturor celorlalți clienți.

    		Dacă un client trimite mesajul "exit" sau închide conexiunea, serverul îl elimină din listă.

    		Trimite mesaje de confirmare/eroare către client la conectare (ex: "Autentificare reusita" sau "Date invalide / Nume ocupat").

Clientul

   	Se conectează la server pe IP și port specificat.

    	Solicită utilizatorului să introducă nickname-ul (și parola, în varianta cu autentificare).

   	Trimite nickname:parola la server imediat după conectare.

   	Așteaptă confirmarea serverului.

      	  	Dacă este acceptat, poate trimite mesaje în chat.

        	Dacă nu este acceptat (date invalide sau nickname folosit), clientul se închide.

    Rulează două fire (threads):

       	Unul care ascultă și afișează mesajele venite de la server.

        Altul care citește mesajele de la tastatură și le trimite la server.

   	Permite ieșirea cu comanda "exit", închizând conexiunea.

Cum functioneaza varianta fara autentificare?

   	Clientul trimite doar un nickname la conectare.

   	Serverul acceptă clientul fără verificări suplimentare.

    	Mesajele sunt etichetate cu nickname-ul clientului.

   	Orice client se poate conecta și participa la chat.

Cum functioneaza varianta cu autentificare?

   	 Clientul trimite nickname și parola separate prin : (ex: user1:pass1).

  	 Serverul verifică dacă nickname-ul și parola sunt valide conform unei liste interne (fixă în cod).

  	 Serverul verifică dacă nickname-ul este deja folosit de un alt client conectat.

   	 Dacă datele sunt corecte și nickname-ul liber, clientul este acceptat.

   	 Dacă nu, conexiunea este închisă și clientul primește mesaj de eroare.

   	 Astfel, se asigură că doar utilizatorii validați pot intra în chat și că nickname-urile sunt unice.

Ce tehnologii și biblioteci am folosit și de ce?
În server și client (Linux):

   	 Socket-uri TCP:
   	 Pentru comunicarea prin rețea între server și client.

    	poll() (server):
    	Pentru a monitoriza mai multe socket-uri simultan (server și clienți), fără a bloca execuția.

    	pthread (client):
    	Pentru a crea două fire de execuție: unul pentru recepția mesajelor și altul pentru trimiterea mesajelor.

    	stdlib, stdio, string:
    	Funcții standard C pentru alocare memorie, input/output și manipulare șiruri.

În Windows (client/server):

   	 Winsock2 (winsock2.h și ws2_32.lib):
    	Pentru socket-uri în Windows, deoarece API-ul este diferit față de Linux.

   	 Threading Windows API (CreateThread etc.)
   	 Sau POSIX Threads portat pentru Windows (în funcție de implementare).

   	 Diferențe de inițializare:
   	 Este necesar WSAStartup() pentru a inițializa Winsock în Windows, și WSACleanup() la final.

Notă finală:
	Pentru securitate reală ar trebui să adaug criptare (TLS/SSL) și să nu trimit parole direct.
	În producție, stocarea parolelor trebuie să fie sigură (hash+salt). 
