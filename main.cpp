/**
 * IMS - T5: SHO Model logistiky
 * 
 * autori: Tomas Valik - xvalik04, Stepan Barta - xbarta50
 * 
*/

#include <simlib.h>
#include <iostream>

using namespace std;

Facility hlavniRidic;     // Hlavni ridic - VOLVO
Facility sekundarniRidic; // Sekundarni ridi - MAN
Facility ridicDodavky;    // ridic dodavky - IVECO
/* LINKY KTERE ZNACI PORUCHU */
Facility linkaPoruchaHlavnihoKamionu;         
Facility linkaPoruchaSekundarnihoKamionu;
Facility linkaPoruchaDodavky;

Facility muzeOdvestKontejner; // priznak toho ze se kontejner vyprazdnil takze se muze jet do Zdaru
/* Fronty pro to aby ridici nemohli jezdit kdyz neni pracovni doba
   Pokud ale uz je po pracovni dobe a oni jsou zrovna na ceste tak ji musi dojet */
Queue frontaProOdchazeniZPrace;         
Queue frontaProOdchazeniZPraceSekundarnihoRidice;  
Queue frontaProOdchazeniZPraceDodavkyRidice;  
/* Fronty do kterych se vkladaji poruchy */
Queue frontaPoruchHlavnihoKamionu;
Queue frontaPoruchSekundarnihoKamionu;
Queue frontaPoruchDodavky;

double casStravenyNaKafiHlavniho = 0;
double casStravenyNaKafiDodavky = 0;
double casStravenyOdvazenimKontejneruZpatkyDoZdaru = 0;
double casStravenyVKamionuOdvazenimKontejneruZpatkyDoZdaru = 0;
double casStravenyCestouDoBrna = 0;

class denHlavnihoRidiceNormal : public Process
{
    void Behavior()
    {
        Seize(linkaPoruchaHlavnihoKamionu);
        Seize(hlavniRidic);
        Wait(Uniform(15, 30)); // priprava k vyjezdu

        Wait(Exponential(30)); // cesta na misto

        Wait(Exponential(60)); // nakladani

        double daSiKaves = Random(); // ted se rozhoduje jesi si da kafe

        if (daSiKaves >= 0.5)
        { //da si kafe
            double casNaKave = Uniform(5, 10);
            casStravenyNaKafiHlavniho += casNaKave;
            Wait(casNaKave);
            Wait(Exponential(30));
        }
        else
        { //neda si kafe
            Wait(Exponential(30));
        }

        Wait(Exponential(15)); // jsme zpet na zakladne vylozime naklad

        // na 10 % se musime vratit protoze jeden kontejner nestacil

        double jesteZbejvaNaklad = Random();

        if (jesteZbejvaNaklad >= 0.90)
        { // musi pro naklad jet znovu
            // cesta na misto
            Wait(Exponential(30));
            // nakladani
            Wait(Exponential(30)); // uz je mene nakladu
            // cesta na zpet
            Wait(Exponential(30));
            // vykladani
            Wait(Exponential(15));
        } 
        Release(hlavniRidic);
        Release(linkaPoruchaHlavnihoKamionu);
    }
};

class hlavnihoRidicOdvaziKontejner : public Process
{
    void Behavior()
    {
        Priority = 6;
        Seize(linkaPoruchaHlavnihoKamionu);
        Seize(hlavniRidic);
        double caspredtim = Time;
        double casNakladani = Uniform(5,10); //nalozeni kontejneru
        Wait(casNakladani);
        Wait(Exponential(45));//cesta do Zdaru
        double casVykladani = Uniform(15,20); //vykladani kontejneru
        Wait(casVykladani);
        Wait(Exponential(45));//cesta zpatky
        double casCelkovy = Time - caspredtim; //vypocet jak dlouho stravil celou cestou
        double casVKamionu = Time - caspredtim - casNakladani - casVykladani; //vypocet jak dlouho stravil jenom v kamionu pri jizde
        casStravenyOdvazenimKontejneruZpatkyDoZdaru += casCelkovy;
        casStravenyVKamionuOdvazenimKontejneruZpatkyDoZdaru += casVKamionu;
        Release(hlavniRidic);
        Release(linkaPoruchaHlavnihoKamionu);
    }
};

class hlavnihoRidiceCestaDoZdaru : public Process
{
    void Behavior()
    {
        Priority = 5;
        Seize(muzeOdvestKontejner);
        Seize(linkaPoruchaHlavnihoKamionu);
        Seize(hlavniRidic);

        Wait(Uniform(5, 15)); // priprava na cestu

        Wait(Exponential(45)); // cesta na misto

        Wait(Uniform(10,15)); // nakladani

        Wait(Exponential(45)); // cesta zpatky

        Wait(Uniform(5, 10)); // vykladani veci ze zdaru

        Release(hlavniRidic);
        Release(linkaPoruchaHlavnihoKamionu);
        Wait(Uniform(60 * 24 * 3, 60 * 24 * 4)); // za 3 - 4 dny pujde vratit kontejner
        (new hlavnihoRidicOdvaziKontejner)->Activate();
        Release(muzeOdvestKontejner);
    }
};

class hlavnihoRidiceCestaDoBrna : public Process
{
    void Behavior()
    {
        Priority = 8;
        Seize(linkaPoruchaHlavnihoKamionu);
        Seize(hlavniRidic);
        double caspredtim = Time;
        Wait(Exponential(60)); // nakladani kovu
        double zacpanadalnici = Random();
        if (zacpanadalnici >= 0.85)
        {//bude zacpa
            Wait(Uniform(60*3,60*5));
        }
        else
        {//nebude zacpa
            Wait(Uniform(60*1.5,60*2.5));
        }
        double casVykladani = Exponential(60);
        Wait(casVykladani); // vykladani v Brne

        Wait(Exponential(90)); // Cesta z Brna

        double cas = Time - caspredtim;
        casStravenyCestouDoBrna += cas;

        Release(hlavniRidic);
        Release(linkaPoruchaHlavnihoKamionu);
    }
};

class sekundarnihoRidiceCesta : public Process
{
    void Behavior()
    {
        Seize(linkaPoruchaSekundarnihoKamionu);
        Seize(sekundarniRidic);

        Wait(Uniform(10, 20)); // priprava

        Wait(Exponential(60)); // cesta tam

        Wait(Uniform(60*4,60*6)); // zpracovava a naklada material 

        Wait(Exponential(75)); // cesta zpatky delsi protoze ma nalozeny kamion

        Release(sekundarniRidic);
        Release(linkaPoruchaSekundarnihoKamionu);
    }
};

class ridiceDodavkyCestaNaRezani : public Process
{
    void Behavior()
    {
        Priority = 1;
        Seize(linkaPoruchaDodavky);
        Seize(ridicDodavky);
        Wait(Uniform(5, 10));//priprava

        Wait(Exponential(45)); // cesta tam

        Wait(Uniform(60*5,60*7)); //kraceni zeleza  

        Wait(Exponential(60)); // cesta zpatky delsi protoze ma zelezo v dodavce

        Release(ridicDodavky);
        Release(linkaPoruchaDodavky);

    }
};

class ridiceDodavkyCestaRandom : public Process
{
    void Behavior()
    {
        Seize(linkaPoruchaDodavky);
        Seize(ridicDodavky);

        Wait(Uniform(5, 10));//priprava

        Wait(Exponential(45)); // cesta tam

        Wait(Exponential(60)); // reze a naklada

        Wait(Exponential(60)); // cesta zpatky delsi protoze ma plnou dodavku

        Release(ridicDodavky);
        Release(linkaPoruchaDodavky);
    }
};

class ridiceDodavkyCestaDoOpatova : public Process
{
    void Behavior()
    {
        Seize(linkaPoruchaDodavky);
        Seize(ridicDodavky);

        Wait(Uniform(5, 10));//priprava

        Wait(Exponential(15)); // cesta do pulky cesty

        double casNakave = Uniform(5, 10);
        casStravenyNaKafiDodavky += casNakave;
        Wait(casNakave); // dava si kavu

        Wait(Exponential(15)); // cesta druha pulka

        Wait(Exponential(30)); // nakladani

        Wait(Exponential(18)); // cesta zpatky pulka

        double dasizmrzlinu = Random();
        if (dasizmrzlinu >= 0.3)
        {// na 70% si da zmrzlinu
            double casZmrzlina = Uniform(10, 15);
            Wait(casZmrzlina); // uziva si zmrzlinu
        }
        Wait(Exponential(18)); // druha pulka cesty zpatky

        Release(ridicDodavky);
        Release(linkaPoruchaDodavky);
    }
};

class ridiceDodavkyCestaDoLuk : public Process
{
    void Behavior()
    {
        Seize(linkaPoruchaDodavky);
        Seize(ridicDodavky);
        
        Wait(Uniform(5, 10));//priprava

        Wait(Exponential(10)); // cesta do pulky cesty
        
        double casNaKave = Uniform(5, 10);
        casStravenyNaKafiDodavky +=casNaKave;
        Wait(casNaKave); // dava si kavu

        Wait(Exponential(10)); // naklada papir

        Wait(Exponential(10)); // cesta zpatky

        Release(ridicDodavky);
        Release(linkaPoruchaDodavky);
    }
};

class ridiceDodavkyCestaDoBrtnice : public Process
{
    void Behavior()
    {
        Seize(linkaPoruchaDodavky);
        Seize(ridicDodavky);
        
        Wait(Uniform(5, 10));//priprava

        Wait(Exponential(15)); // pulka cesty

        //na 90% procent si zajde na kavu
        double zajdesinakavu = Random();
        if (zajdesinakavu >= 0.1)
        {//zajde si na kavu
            double casNaKave = Uniform(10, 15);
            casStravenyNaKafiDodavky +=casNaKave;
            Wait(casNaKave); // uziva si kavu
        }
        Wait(Exponential(15)); // druha pulka
        Wait(Exponential(10)); // naklada papir
        Wait(Exponential(30)); // cesta zpatky

        Release(ridicDodavky);
        Release(linkaPoruchaDodavky);
    }
};

class ridicDeDom : public Process
{
    void Behavior()
    {
        Priority = 100;
        Wait(7 * 60);//7 hodin pracovni doba
        Seize(hlavniRidic);
        frontaProOdchazeniZPrace.Insert(this);
        Passivate();
        Release(hlavniRidic);
    }
};
class ridicSekundarniDeDom : public Process
{
    void Behavior()
    {
        Priority = 101;
        Wait(7 * 60);//7 hodin pracovni doba
        Seize(sekundarniRidic);
        frontaProOdchazeniZPraceSekundarnihoRidice.Insert(this);
        Passivate();
        Release(sekundarniRidic);
    }
};
class ridicDodavkyDeDom : public Process
{
    void Behavior()
    {
        Priority = 99;
        Wait(7 * 60);//7 hodin pracovni doba
        Seize(ridicDodavky);
        frontaProOdchazeniZPraceDodavkyRidice.Insert(this);
        Passivate();
        Release(ridicDodavky);
    }
};

class generatorCasu : public Event
{
    void Behavior()
    {
        if (frontaProOdchazeniZPrace.Length() > 0)
        {//pokud nekdo je ted v passivate tak ho probudime
            (frontaProOdchazeniZPrace.GetFirst())->Activate();
        }
        if (frontaProOdchazeniZPraceSekundarnihoRidice.Length() > 0)
        {//pokud nekdo je ted v passivate tak ho probudime
            (frontaProOdchazeniZPraceSekundarnihoRidice.GetFirst())->Activate();
        }
        if (frontaProOdchazeniZPraceDodavkyRidice.Length() > 0)
        {//pokud nekdo je ted v passivate tak ho probudime
            (frontaProOdchazeniZPraceDodavkyRidice.GetFirst())->Activate();
        }
        (new ridicDeDom)->Activate(); //ridic jde do prace a po 7 hodinach jde domu
        (new ridicSekundarniDeDom)->Activate(); //ridic jde do prace a po 7 hodinach jde domu
        (new ridicDodavkyDeDom)->Activate(); //ridic jde do prace a po 7 hodinach jde domu
        Activate(Time + (24 * 60)); // kazdych 24 hodin musi jit do prace 
    }
};

class generatorNaNormalniJizdu : public Event
{
    void Behavior()
    {
        (new denHlavnihoRidiceNormal)->Activate();
        Activate(Time + Exponential(60 * 8)); // kazdych 8 hodin
    }
};

class generatorZdaru : public Event
{
    void Behavior()
    {
        (new hlavnihoRidiceCestaDoZdaru)->Activate();
        Activate(Time + Exponential((60 * 24 * 10))); // jednou za dva tydny
    }
};

class generatorBrna : public Event
{
    void Behavior()
    {
        (new hlavnihoRidiceCestaDoBrna)->Activate();
        Activate(Time + Exponential((60 * 24 * 30))); // kazdych 6 tydnu
    }
};

class generatorJizdyProSekundarniKamion : public Event
{
    void Behavior()
    {
        (new sekundarnihoRidiceCesta)->Activate();
        Activate(Time + Exponential((2057))); // 24*60*(5/3.5) neboli za 5 pracovnich dnu 3-4 jizdy
    }
};
class generatorDodavkyCestyNaRezani : public Event
{
    void Behavior()
    {
        (new ridiceDodavkyCestaNaRezani)->Activate();
        Activate(Time + Exponential((60 * 24 * 5))); // jednou za tyden
    }
};
class generatorDodavkyCestyRandom : public Event
{
    void Behavior()
    {
        (new ridiceDodavkyCestaRandom)->Activate();
        Activate(Time + Exponential((60 * 24 * 10))); // jednou za dva tydny
    }
};
class generatorDodavkyCestyDoOpatova : public Event
{
    void Behavior()
    {
        (new ridiceDodavkyCestaDoOpatova)->Activate();
        Activate(Time + Exponential((60 * 24 * 30))); // 6 tydnu a pul pracovniho dne
    }
};
class generatorDodavkyCestyDoLuk : public Event
{
    void Behavior()
    {
        (new ridiceDodavkyCestaDoLuk)->Activate();
        Activate(Time + Exponential((60 * 24 * 10))); // jednou za dva tydny
    }
};
class generatorDodavkyCestyDoBrtnice : public Event
{
    void Behavior()
    {
        (new ridiceDodavkyCestaDoBrtnice)->Activate();
        Activate(Time + Exponential((60 * 24 * 10))); // jednou za dva tydny
    }
};

class poruchaHlavnihoKamionu : public Process
{
    void Behavior()
    {
        Priority = 50;
        frontaPoruchHlavnihoKamionu.Insert(this);
        Passivate();
        Seize(linkaPoruchaHlavnihoKamionu);
        Wait(Uniform((24 * 60 * 3),(24 * 60 * 4)));//oprava kamionu VOLVO 3-4 dny
        Release(linkaPoruchaHlavnihoKamionu);
    }
};

class generatorPoruchyHlavnihoKamionu : public Event
{
    void Behavior()
    {
        if (frontaPoruchHlavnihoKamionu.Length() > 0)
        {
            (frontaPoruchHlavnihoKamionu.GetFirst())->Activate();
        }
        (new poruchaHlavnihoKamionu)->Activate();
        Activate(Time + Exponential(60 * 24 * 365)); // jednou za rok porucha hlavniho vozidla VOLVO
    }
};

class poruchaSekundarnihoKamionu : public Process
{
    void Behavior()
    {
        Priority = 50;
        frontaPoruchSekundarnihoKamionu.Insert(this);
        Passivate();
        Seize(linkaPoruchaSekundarnihoKamionu);
        Wait(Uniform((24 * 60 * 3),(24 * 60 * 4)));//oprava kamionu MAN 3-4 dny
        Release(linkaPoruchaSekundarnihoKamionu);
    }
};

class generatorPoruchySekundarnihoKamionu : public Event
{
    void Behavior()
    {
        if (frontaPoruchSekundarnihoKamionu.Length() > 0)
        {
            (frontaPoruchSekundarnihoKamionu.GetFirst())->Activate();
        }
        (new poruchaSekundarnihoKamionu)->Activate();
        Activate(Time + Exponential(60 * 24 * 365)); // jednou za rok porucha sekundarniho kamoinu MAN
    }
};

class poruchaDodavky : public Process
{
    void Behavior()
    {
        Priority = 50;
        frontaPoruchDodavky.Insert(this);
        Passivate();
        Seize(linkaPoruchaDodavky);
        Wait(Uniform((24 * 60 * 2),(24 * 60 * 3)));// oprava  dodavky 2-3 dny
        Release(linkaPoruchaDodavky);
    }
};

class generatorPoruchyDodavky : public Event
{
    void Behavior()
    {
        if (frontaPoruchDodavky.Length() > 0)
        {
            (frontaPoruchDodavky.GetFirst())->Activate();
        }
        (new poruchaDodavky)->Activate();
        Activate(Time + Exponential(60 * 24 * 90)); // jednou za 3 mesice
    }
};

int main(int argc, char **argv)
{
    RandomSeed(time(NULL)); //seed na to aby se program spustil pokazde jinak random
    Init(0, 60 * 24 * 365 * 5); //5 let pouze pracovnich dnu bez vikendu
    /* generator na to aby ridici rano chodili do prace a pote sli domu po pracovni dobe */
    (new generatorCasu)->Activate();
    /* 3 generatory pro cesty hlavniho ridice VOLVO*/
    (new generatorBrna)->Activate();
    (new generatorZdaru)->Activate();
    (new generatorNaNormalniJizdu)->Activate();
    /* generator pro cestu sekundarniho ridice MAN*/
    (new generatorJizdyProSekundarniKamion)->Activate();
    /* 5 generatoru pro cesty ridice dodavky */
    (new generatorDodavkyCestyNaRezani)->Activate();
    (new generatorDodavkyCestyRandom)->Activate();
    (new generatorDodavkyCestyDoOpatova)->Activate();
    (new generatorDodavkyCestyDoLuk)->Activate();
    (new generatorDodavkyCestyDoBrtnice)->Activate();
    /* generatory pro poruchy vozidel */
    (new generatorPoruchyHlavnihoKamionu)->Activate();
    (new generatorPoruchySekundarnihoKamionu)->Activate();
    (new generatorPoruchyDodavky)->Activate();
    Run();
    double vsichniRidiciCelkoveNaKafiZaRok= ((casStravenyNaKafiHlavniho/5)/60)+((casStravenyNaKafiDodavky/5)/60);
    std::cout << "Ridic VOLVA stravil v kamionu pri ceste do Brna: " << ((casStravenyCestouDoBrna/5)/60)<< "hodin"<<std::endl;
    std::cout << "--------------------------------------KAFE--------------------------------------" <<std::endl;
    std::cout << "Ridic VOLVA stravi na kafi za rok: " << (casStravenyNaKafiHlavniho/5)/60 << "hodin "<<std::endl;
    std::cout << "Rocni vydaje ktere firma plati ridici VOLVA za to ze sedi na kafi: " << ((casStravenyNaKafiHlavniho/5)/60)*230 << "Kc" <<std::endl;
    std::cout << "Ridic dodavky stravi na kafi za rok: " << (casStravenyNaKafiDodavky/5)/60 << "hodin "<<std::endl;
    std::cout << "Rocni vydaje ktere firma plati ridici dodavky za to ze sedi na kafi: " << ((casStravenyNaKafiDodavky/5)/60)*230 << "Kc" <<std::endl;
    std::cout << "Celkove ridici stravi na kafi za rok: " << vsichniRidiciCelkoveNaKafiZaRok <<"hodin " <<std::endl;
    std::cout << "Celkova cena ktera stoji firmu za to ze misto jezdeni sedi u kafe: " << vsichniRidiciCelkoveNaKafiZaRok*230 <<"Kc"<<std::endl;
    std::cout << "--------------------VRACENI SE DO ZDARU KVULI VRACENI KONTEJNERU--------------------" <<std::endl;
    double casZaRokStravenyNaCestachDoZdaru = ((casStravenyOdvazenimKontejneruZpatkyDoZdaru/5)/60);
    double casZaRokStravenyVKamoinuNaCestachDoZdaru =  ((casStravenyVKamionuOdvazenimKontejneruZpatkyDoZdaru/5)/60);
    std::cout << "Ridic VOLVA stravil odvazenim kontejneru zpatky do Zdaru za rok: " << casZaRokStravenyNaCestachDoZdaru <<"hodin " <<std::endl;
    std::cout << "Ridic VOLVA stravil v kamionu cestou do zdaru: " << casZaRokStravenyVKamoinuNaCestachDoZdaru << std::endl;
    double bezVraceniDoZdaruBySeUsetrilo = (casZaRokStravenyNaCestachDoZdaru*230) + (casZaRokStravenyVKamoinuNaCestachDoZdaru*0.6*25*38);
    std::cout << "Usetrilo by se za plat: " << (casZaRokStravenyNaCestachDoZdaru*230) << "Kc" <<std::endl;
    std::cout << "Usetrilo by se za naftu: " << (casZaRokStravenyVKamoinuNaCestachDoZdaru*0.6*25*38) << "Kc"<< std::endl;
    std::cout << "Celkove by se za koupeni druheho kontejneru usetrilo: " << bezVraceniDoZdaruBySeUsetrilo<< "Kc" << std::endl;
    std::cout << "Pokud by firma koupila automat na kavu a koupila druhy kontejner pro Zdar tak by se usetrilo: " << bezVraceniDoZdaruBySeUsetrilo+(vsichniRidiciCelkoveNaKafiZaRok*230) << "Kc" << std::endl;
    return 0;
}