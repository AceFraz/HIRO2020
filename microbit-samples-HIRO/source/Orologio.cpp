#include "Orologio.h"

/*
    GIORNI_MESE[] contiene il numero di giorni per ogni mese dell'anno, 
    in modo da aggiornare correttamente la data, nel caso in cui
    si decida di lasciare il prototipo operativo per lunghi lassi di tempo
*/
const int GIORNI_MESE[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

//La prima istanza della classe Orologio non è inizializzata e non contiene alcuna data
data Orologio::Data = {};
bool Orologio::Inizializzato = false;

/*
    Il metodo statico startClock() si occupa di aggiorare perpetuamente
    l'orologio al passare del tempo (con precisione del secondo)
*/
void Orologio::startClock()
{
    while(true) {
        Orologio::Data.sec++;

        if(Orologio::Data.sec >= 60) {
            Orologio::Data.sec = 0;
            Orologio::Data.min++;
        }

        if(Orologio::Data.min >= 60) {
            Orologio::Data.min = 0;
            Orologio::Data.ora++;
        }

        if(Orologio::Data.ora >= 24) {
            Orologio::Data.ora = 0;
            Orologio::Data.gg++;
        }
        
        if(Orologio::Data.gg > GIORNI_MESE[Orologio::Data.mm-1]) {
            Orologio::Data.gg = 1;
            Orologio::Data.mm++;
        }

        if(Orologio::Data.mm > 12) {
            Orologio::Data.mm = 0;
            Orologio::Data.aaaa++;
        }

        fiber_sleep(1000);
    }
}

Orologio::Orologio() {}

//Inizializza l'orologio inserendo una data iniziale
void Orologio::init(data dataIniziale)
{
    if(!Orologio::Inizializzato) {
        Orologio::Inizializzato = true;

        Orologio::Data.gg = dataIniziale.gg;
        Orologio::Data.mm = dataIniziale.mm;
        Orologio::Data.aaaa = dataIniziale.aaaa;
        Orologio::Data.ora = dataIniziale.ora;
        Orologio::Data.min = dataIniziale.min;
        Orologio::Data.sec = dataIniziale.sec;

        create_fiber(startClock);
    }
}


//Controlla se l'orologio è inizializzato
bool Orologio::isInit()
{
    return Inizializzato;
}

//Ricevi la data dell'orologio, sotto forma di struct 'data'
data Orologio::getData()
{   
    return Orologio::Data;
}

//Reimposta la data dell'orologio
void Orologio::setData(data nuovaData)
{
    Orologio::Data.gg = nuovaData.gg;
    Orologio::Data.mm = nuovaData.mm;
    Orologio::Data.aaaa = nuovaData.aaaa;
    Orologio::Data.ora = nuovaData.ora;
    Orologio::Data.min = nuovaData.min;
    Orologio::Data.sec = nuovaData.sec;
}