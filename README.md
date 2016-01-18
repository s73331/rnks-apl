# rnks-apl  
um das local.h und manipulation.h nicht herumschwirren zu haben
git update-index --assume-unchanged local.h  
git update-index --assume-unchanged manipulation.h

##aktueller Stand
Client schickt an Server  
beliebig viele Hellos gehen verloren, Client schickt alle 300ms bis HelloACK ankommt  
Server schickt HelloACK  
Client bekommt HelloACK, liest File und schickt Daten(1)  
Client schickt alle 300ms Daten(n), wenn kein NACK kommt  
Server speichert Paket, das nicht reihenfolgetreu ist  
Bei NACK schickt der Client das betreffende Paket nochmal.  
Server ordnet Paket wieder ein  
Client schickt einen Close  
Server schickt CloseACK, noch fehlende NACKs oder NACK für Close, wenn timeout  
Client schickt nach, bis CloseACK kommt  
Server printet ins file  

##todo  
in file.c/readfile: free(buf); corrupts heap  
in serverSyUDP.c  : free memory  
in client.c       : free memory
  
##Notizen aus letzter Vorlesung vor Weihnachten  
####Sender
*UDPv6 Socket öffnen  
*Port wählen (50000)  
*sende Daten an Multicast  

####Empfänger
*UDP Socket  
*Port  
*MC-Gruppe beitreten  
*warte auf Nachricht  

####1. Phase
*Hello MC  
*Hello ACK  
*Unicast Addresse des Empfängers merken  
*Warten  

####2. Phase
*SQ=2*f-1  

*send(1)  
*fail(2)  
*send(3)  
*NACK(2)  
*send(2)  
*...  
*fail(-1)  
*close  
*NACK(-1)  
*send(-1)  
*closeACK  