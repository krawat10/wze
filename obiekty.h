#include <stdio.h>
#include "kwaternion.h"

#define PI 3.1416

struct StanObiektu
{
  int iID;                  // identyfikator obiektu
	Wektor3 wPol;             // polozenie obiektu (œrodka geometrycznego obiektu) 
	kwaternion qOrient;       // orientacja (polozenie katowe)
	Wektor3 wV,wA;            // predkosc, przyspiesznie liniowe
  Wektor3 wV_kat, wA_kat;   // predkosc i przyspeszenie liniowe
	float masa_calk;          // masa ca³kowita (wa¿na przy kolizjach)
	long pieniadze;
	int iID_wlasc;
	int czy_autonom;
   unsigned int grupa;
   int zbiera;
};

class ObiektRuchomy
{
public:
	int iID;                  // identyfikator obiektu
	Wektor3 wPol;             // polozenie obiektu
	kwaternion qOrient;       // orientacja (polozenie katowe)
	Wektor3 wV,wA;            // predkosc, przyspiesznie liniowe
    Wektor3 wV_kat, wA_kat;   // predkosc i przyspeszenie liniowe
    int zbiera;
    long pieniadze;
    float ilosc_paliwa;

  float umiejetn_sadzenia,    // umiejêtnoœæ sadzenia drzew (1-pe³na, 0- brak)
        umiejetn_zb_paliwa,   // umiejêtnoœæ zbierania paliwa
        umiejetn_zb_monet;  

	float F,Fb;               // si³y dzia³aj¹ce na obiekt: F - pchajaca do przodu, Fb - w prawo
	float ham;                // stopieñ hamowania Fh_max = tarcie*Fy*ham
	float Fy;                 // si³a nacisku na podstawê pojazdu - gdy obiekt styka siê z pod³o¿em (od niej zale¿y si³a hamowania)
	float alfa;               // kat skretu kol w radianach (w lewo - dodatni)
	float masa_wlasna;		  // masa w³asna obiektu (bez paliwa)	
	float masa_calk;          // masa ca³kowita (wa¿na przy kolizjach)
	float dlugosc,szerokosc,wysokosc; // rozmiary w kierunku lokalnych osi x,y,z
	float promien;            // promien minimalnej sfery, w ktora wpisany jest obiekt
	float przeswit;           // wysokoœæ na której znajduje siê podstawa obiektu
	float dl_przod;           // odleg³oœæ od przedniej osi do przedniego zderzaka 
	float dl_tyl;             // odleg³oœæ od tylniej osi do tylniego zderzaka
	long moment_wziecia;        // moment, w ktorym rozpoczeto branie
	long czas_oczekiwania;      // czas oczekiwania na finalizacjê procesu brania
	long nr_przedmiotu;         // numer przedmiotu do wziecia; -1 jesli zaden  
   unsigned int grupa;
	int iID_kolid;            // identyfikator pojazdu z którym nast¹pi³a kolizja (-1  brak kolizji)
	Wektor3 wdV_kolid;        // poprawka prêdkoœci pojazdu koliduj¹cego
	int iID_wlasc; 
	bool czy_autonom;         // czy obiekt jest autonomiczny

public:
	ObiektRuchomy();          // konstruktor
	~ObiektRuchomy();
	void ZmienStan(StanObiektu stan);          // zmiana stanu obiektu
	StanObiektu Stan();        // metoda zwracajaca stan obiektu
    void Symulacja(float dt);  // symulacja ruchu obiektu w oparciu o biezacy stan, przylozone sily
	                           // oraz czas dzialania sil. Efektem symulacji jest nowy stan obiektu 
	void Rysuj();			   // odrysowanie obiektu					
   void SetGrupa(unsigned int);
   unsigned int GetGrupa();

};

enum typ {MONETA, BECZKA, DRZEWO, MUR };

enum podtyp {TOPOLA, SWIERK};

struct Przedmiot
{
	Wektor3 wPol;             // polozenie obiektu
	kwaternion qOrient;       // orientacja (polozenie katowe)

	int typ;
	int podtyp;
	long wartosc;             // w zal. od typu nomina³ monety /ilosc paliwa, itd.
	float srednica;           // np. grubosc pnia u podstawy, srednica monety
	
	bool do_wziecia;          // czy przedmiot mozna wziac
  bool czy_ja_wzialem;      // czy przedmiot wziety przeze mnie
  bool czy_odnawialny;      // czy mo¿e siê odnawiaæ w tym samym miejscu po pewnym czasie
  long czas_wziecia;        // czas wziêcia (potrzebny do przywrócenia)
};

class Teren
{
public:
    float **mapa;          // wysokoœci naro¿ników oraz œrodków pól
    float ***d;            // wartoœci wyrazu wolnego z równania p³aszczyzny dla ka¿dego trójk¹ta
    Wektor3 ***Norm;       // normalne do p³aszczyzn trójk¹tów
    float rozmiar_pola;    // dlugosc boku kwadratowego pola na mapie
    long lwierszy,lkolumn; // liczba wierszy i kolumn mapy (kwadratów na wysokoœæ i szerokoœæ)     
    
    long liczba_przedmiotow;      // liczba przedmiotów na planszy
    long liczba_przedmiotow_max;  // rozmiar tablicy przedmiotów
    Przedmiot *p;          // tablica przedmiotow roznego typu
    
    Teren();    
    ~Teren();   
    float Wysokosc(float x,float z);      // okreœlanie wysokoœci dla punktu o wsp. (x,z) 
    void Rysuj(int szczegolowosc);	      // odrysowywanie terenu (szczeg. pe³na gdy == 1, zminejszona gdy == 0) 
    void PoczatekGrafiki();               // tworzenie listy wyœwietlania
};
