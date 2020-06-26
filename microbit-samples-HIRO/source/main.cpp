#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "MicroBit.h"
#include "MicroBitFile.h"
#include "MicroBitUARTService.h"
#include "Orologio.h"

MicroBit uBit;
MicroBitUARTService *uart;
Orologio orologio;

const int TEMP_MIN_MESE[] = {17, 18, 19, 20, 20, 26, 27, 28, 25, 23, 20, 18};
const int TEMP_MAX_MESE[] = {21, 22, 23, 24, 25, 30, 31, 31, 29, 26, 24, 21};

const ManagedString NOME_FILE("Logs.csv");
const ManagedString EOM(":");
const int RECORD_LEN = 29;
const int SEND_LEN = 16;
const char DLM = ',';

bool bluetooth = false;
bool connesso = false;
bool statoTrasferimento = false;

/*
    Le funzioni vaiAvanti, giraSinistra e giraDestra impostano i valori digitali dei 
    PIN del Micro:bit al valore 1 per un lasso di tempo in sec, per far muovere il prototipo
*/
void vaiAvanti(int sec) {
    MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ALL);
    MicroBitPin P1(MICROBIT_ID_IO_P1, MICROBIT_PIN_P1, PIN_CAPABILITY_ALL);

    P0.setDigitalValue(1);
    P1.setDigitalValue(1);
    uBit.sleep(sec*1000);
    P0.setDigitalValue(0);
    P1.setDigitalValue(0);
}

void giraDestra(int sec) {
    MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ALL);

    P0.setDigitalValue(1);
    uBit.sleep(sec*1000);
    P0.setDigitalValue(0);
}

void giraSinistra(int sec) {
    MicroBitPin P1(MICROBIT_ID_IO_P1, MICROBIT_PIN_P1, PIN_CAPABILITY_ALL);

    P1.setDigitalValue(1);
    uBit.sleep(sec*1000);
    P1.setDigitalValue(0);
}

/*
    La funzione mStrToData converte una stringa contenente le informazioni di data e orario
    in una struct di tipo data, per poi utilizzarla nell'orologio interno
*/
data mStrToData(ManagedString mStrData)
{
    int tmp[6] = {};
    int posTmp = 0;
    int lenNum = 0;
    int lenStr = mStrData.length();

    for(int k = 0; k<lenStr; k++) {
        if(mStrData.charAt(k) == DLM) {
            tmp[posTmp] = atoi(mStrData.substring(k-lenNum, lenNum).toCharArray());
            posTmp++, lenNum = 0;
        } else {
            lenNum++;
        }
    }

    data retData = {tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]};

    return retData;
}

//Questa funzione viene richiamata quando un dispositivo si connette al modulo Bluetooth del Micro:bit
void onConnected(MicroBitEvent)
{   
    //Se c'è la possibilità di connettersi al modulo Bluetooth
    if(bluetooth) {
        //Entriamo nello stato di connesso
        connesso = true;

        //Sul display del Micro:bit viene trasmesso il simbolo del Bluetooth
        MicroBitImage bluetooth("255,0,255,255,0 \n 0,255,255,0,255 \n 0,0,255,255,0 \n 0,255,255,0,255 \n 255,0,255,255,0 \n");
        uBit.display.print(bluetooth);

        uBit.sleep(2000);

        //Il Micro:bit richiede all'applicazione la data corrente
        uart->send("tempo", SYNC_SLEEP);
        //e aspetta di riceverla
        ManagedString mStrData = uart->readUntil(EOM, SYNC_SLEEP);
        
        data nuovaData = mStrToData(mStrData);

        //Se nell'orologio è già presente la data corrente, la aggiorna
        if(orologio.isInit()) {
            orologio.setData(nuovaData);
        } else { //In alternativa la inserisce per la prima volta, inizializzando l'orologio
            orologio.init(nuovaData);
        }
        
        //Prova ad aprire il file contenente le rilevazioni
        MicroBitFile fileRead(NOME_FILE, READ);
        
        //Se il file esiste, trasferiamo tutti i record del file all'applicazione
        if(fileRead.isValid()) {
            uart->send("fileTr", SYNC_SLEEP);
            uBit.sleep(500);
            ManagedString tmp = fileRead.read(SEND_LEN);

            while(tmp.length() > 0) {
                uart->send(tmp, SYNC_SLEEP);
                tmp = fileRead.read(SEND_LEN);
            }
            uart->send("fileTrStop", SYNC_SLEEP);
            fileRead.close();
            uBit.sleep(500);
        }
        
        //Inviamo un messaggio per chiudere la comunicazione con l'applicazione
        uart->send("stop", SYNC_SLEEP);

        //Ormai possiamo rimuovere dal Micro:bit le rilevazioni scaricate, per liberare spazio in memoria
        MicroBitFileSystem::defaultFileSystem->remove(NOME_FILE.toCharArray());

        //Il trasferimento è terminato con successo, quindi impostiamo il suo stato a 'true'
        statoTrasferimento = true;
        //La connessione è terminata, quindi impostiamo il suo stato a 'false'
        connesso = false;
    } else {
        /*
            Invece, se non c'è possibilità di connettersi al modulo Bluetooth, 
            inviamo un messaggio all'applicazione per comunicarlo
        */
        uart->send("occupato", SYNC_SLEEP);
    }
}

//Questa funzione viene richiamata quando un dispositivo si disconnette dal modulo Bluetooth del Micro:bit
void onDisconnect(MicroBitEvent)
{
    statoTrasferimento = true;
    connesso = false;
}

int main()
{
    //Inizializziamo il runtime del Micro:bit e il servizio UART
    uBit.init();
    uart = new MicroBitUARTService(*uBit.ble, 32, 32);

    //Entriamo in ascolto per possibili connessioni Bluetooth
    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_CONNECTED, onConnected);
    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_CONNECTED, onDisconnect);

    //Definiamo le varie MicroBitImage da proiettare nel display
    MicroBitImage clessidra("255,255,255,255,255 \n 0,255,0,255,0 \n 0,0,255,0,0 \n 0,255,0,255,0 \n 255,255,255,255,255 \n");
    MicroBitImage clessidraSkip("255,255,255,255,255 \n 0,255,0,255,0 \n 0,0,255,0,255 \n 0,255,0,255,0 \n 255,255,255,255,255 \n");
    MicroBitImage spunta("0,0,0,0,0 \n 0,0,0,0,255 \n 0,0,0,255,0 \n 255,0,255,0,0 \n 0,255,0,0,0 \n");
    MicroBitImage frecciaSinstra("0,0,255,0,0 \n 0,255,0,0,0 \n 255,255,255,255,255 \n 0,255,0,0,0 \n 0,0,255,0,0 \n");
    MicroBitImage domanda("0,255,255,0,0 \n 255,0,0,255,0 \n 255,0,0,255,0 \n 0,0,255,0,0 \n 0,0,255,0,0 \n");
    MicroBitImage calore("0,255,0,0,255 \n 255,0,0,255,0 \n 255,0,0,255,0 \n 0,255,0,0,255 \n 0,255,0,0,255 \n");
    MicroBitImage neve("255,0,255,0,255 \n 0,255,0,255,0 \n 255,255,255,255,255 \n 0,255,0,255,0 \n 255,0,255,0,255 \n");

    while(true) {
        //Aspettiamo per delle possibili connessioni Bluetooth
        bluetooth = true;
        
        //Se l'orologio del prototipo è già inizializzato
        if(orologio.isInit()) {
            uBit.display.print(clessidraSkip);
            //Si attende per possibili connessioni Bluetooth, oppure si può procedere premendo il tasto B
            while(!statoTrasferimento && !(uBit.buttonB.isPressed() && !connesso)) {
                uBit.sleep(100);
            }
        } else {
            uBit.display.print(clessidra);
            //Altrimenti, si attende la prima connessione Bluetooth, che servirà per inizializzare la data
            while(!statoTrasferimento) {
                uBit.sleep(100);
            }

            //Proiettiamo sul display una freccia che indica il tasto da premere per proseguire
            uBit.display.print(frecciaSinstra);

            //Una volta premuto il tasto A, si procede con le operazioni successive
            while(!uBit.buttonA.isPressed()) {
                uBit.sleep(100);
            }
        }

        //Il trasferimento è finito, quindi impostiamo il suo stato a 'false'
        statoTrasferimento = false;

        //Non siamo più in attesa di possibili connessioni Bluetooth
        bluetooth = false;
        
        //Diamo un feedback all'utente tramite il display
        uBit.display.print(spunta);
        uBit.sleep(3000);
        uBit.display.clear();
        
        int lastMin = -1;
        /*
            Da qui in poi il prototipo raccoglierà dati sul benessere dell'utente
            e cercherà di dargli consigli sulla base di valori predefiniti.
            Per tornare alla fase delle connessioni Bluetooth, premere B
        */
        while(!uBit.buttonB.isPressed()) {
            //Otteniamo la data attuale
            data attuale = orologio.getData();

            /*
                Ogni ora, all'inizio dell'ora, il prototipo chiede all'utente se si trova in:
                - Uno stato di benessere
                - Avverte caldo
                - Avverte freddo
            */
            if((attuale.min != lastMin) && (attuale.min == 0)) {
                lastMin = attuale.min;

                ManagedString statoUtente("?");
                //Sul display appare un '?' per avvisare l'utente
                uBit.display.print(domanda);
                //Inoltre il prototipo si muove in avanti per 15sec per attirare l'attenzione dell'utilizzatore
                vaiAvanti(15);

                //Adesso aspettiamo per 5 minuti una risposta da parte dell'utente
                while(attuale.min - 5 > lastMin) {
                    //Premendo entrambi i bottoni, l'utente segnalerà uno stato di benessere
                    if(uBit.buttonAB.isPressed()) {
                        statoUtente = "bene";
                        break;
                    }
                    //Premendo il bottone A, l'utente segnalerà di avvertire freddo
                    if(uBit.buttonA.isPressed()) {
                        statoUtente = "freddo";
                        break;
                    }
                    //Premendo il bottone B, l'utente segnalerà di avvertire caldo
                    if(uBit.buttonB.isPressed()) {
                        statoUtente = "caldo";
                        break;
                    }

                    uBit.sleep(100);
                }

                //Una volta passati i 5 minuti, resettiamo il display
                uBit.display.clear();
                
                //Facciamo girare il prototipo e lo mandiamo avanti, per farlo tornare alla posizione di partenza
                giraSinistra(2);
                vaiAvanti(15);
                giraSinistra(2);

                //Se c'è stata una risposta da parte dell'utente
                if(!(statoUtente == "?")) {
                    //Rileviamo la temperatura
                    int temp = uBit.thermometer.getTemperature()-1;
                    /*
                        Creiamo un record che contenga:
                        - Temperatura
                        - Ora e data
                        - Stato dell'utente
                    */
                    char recordTmp[RECORD_LEN+1];
                    sprintf(recordTmp, "\n%04d,%02d,%02d,%02d,%02d,%04d,%s", temp, attuale.ora, attuale.min, attuale.gg, attuale.mm, attuale.aaaa, statoUtente.toCharArray());
                    ManagedString record(recordTmp);

                    //Apriamo il file delle rilevazioni
                    MicroBitFile fileOpen(NOME_FILE, READ_AND_WRITE);

                    //e se il file esiste
                    if(fileOpen.isValid()) {
                        //Scriviamo il record all'interno del file
                        fileOpen.append(record);
                        //e lo chiudiamo
                        fileOpen.close();
                    } else {
                        //Altrimenti, creiamo il file
                        MicroBitFile fileCreate(NOME_FILE, CREATE);
                        //Scriviamo la legenda e il record all'interno del file
                        ManagedString legenda("temp,hs,ms,gg,mm,aaaa,stato");
                        fileCreate.append(legenda);
                        fileCreate.append(record);
                        //e lo chiudiamo
                        fileCreate.close();
                    }
                }
            }
            
            /*
                Ogni quarto d'ora, tranne all'inizio dell'ora,
                segnaliamo all'utente in che stato ambientale si trova
            */
            if((attuale.min != lastMin) && (attuale.min%15 == 0)) {
                lastMin = attuale.min;

                //Rileviamo la temperatura
                int temp = uBit.thermometer.getTemperature()-1;

                //E a seconda di valori predefiniti per ogni mese, il prototipo avvisa l'utente.

                //Se la temperatura interna è troppo bassa
                if(temp < TEMP_MIN_MESE[attuale.mm-1]) {
                    //Avverte l'utente proiettando sul display un fiocco di neve
                    uBit.display.print(neve);
                //Se la temperatura interna è troppo alta
                } else if(temp > TEMP_MAX_MESE[attuale.mm-1]) {
                    //Avverte l'utente proiettando sul display un simbolo che ricorda il calore
                    uBit.display.print(calore);
                //Se la temperatura interna è regolare
                } else {
                    //Proietta una spunta, indicando che lo stato ambientale è ottimale
                    uBit.display.print(spunta);
                }
            }
            uBit.sleep(100);
        }
        uBit.sleep(3000);
    }
	release_fiber();
}
