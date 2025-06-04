Tutorial utilizare server de tip 2: cu verificare credentiale;
//Credentialele se afla intr-un array de structuri hardcodate cu nume si parole in fisierul server din motive de securitate;

I. GAZDA

1. Lanseaza in terminal linux comanda:
ssh -R <port>:localhost:<port> serveo.net

2. Compileaza serverul:
gcc server-linux2.c -o server

3. Lanseaza in executie serverul:
./server

// Optional, se poate pune si un al doilea argument care va reprezenta portul ce implicit este 9091.


II. CLIENT LINUX

1. Compileaza clientul:
gcc client-linux2.c -lpthread -o client

2. Lanseaza in executie clientul:
./client <adresa> <port>

// Daca nu esti in aceeasi retea, vei scrie:
./client serveo.net <port>

3. Urmeaza pasii pentru credentiale: Nume si Parola
// Daca nu sunt corecte, autentificarea va esua fara sa zica motivul :)

4. Ca sa iesi, scrii:
exit



III. CLIENT WINDOWS

1. Compileaza clientul:
gcc client-windows2.c -lws2_32 -lpthread client.exe

2. Lanseaza in executie clientul:
.\client <adresa> <port>

// Daca nu esti in aceeasi retea, vei scrie:
.\client serveo.net <port>

3. Urmeaza pasii pentru credentiale: Nume si Parola
// Daca nu sunt corecte, autentificarea va esua fara sa zica motivul :)

4. Ca sa iesi, scrii:
exit




