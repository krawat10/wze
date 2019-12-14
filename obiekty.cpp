/*********************************************************************
Symulacja obiektów fizycznych ruchomych np. samochody, statki, roboty, itd. 
+ obs³uga obiektów statycznych np. teren.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include "obiekty.h"
#include "grafika.h"


//#include "wektor.h"
extern FILE *f;
extern Teren teren;
extern int iLiczbaCudzychOb;
extern ObiektRuchomy *CudzeObiekty[1000]; 


ObiektRuchomy::ObiektRuchomy()             // konstruktor                   
{

  //iID = (unsigned int)(clock() % 1000);  // identyfikator obiektu
  iID = (unsigned int)(rand() % 1000);  // identyfikator obiektu
  fprintf(f,"Nowy obiekt: iID = %d\n",iID);
  iID_wlasc = iID;           // identyfikator w³aœciciela obiektu
  czy_autonom = 0;

  pieniadze = 1000;    // np. z³otych
  ilosc_paliwa = 10.0;   // np. kilogramów paliwa 

  F = Fb = alfa  = 0;	      // si³y dzia³aj¹ce na obiekt 
  ham = 0;			      // stopieñ hamowania
  masa_wlasna = 1200.0;     // masa w³asna obiektu [kg] (bez paliwa)
  masa_calk = masa_wlasna + ilosc_paliwa;  // masa ca³kowita
  Fy = masa_wlasna*9.81;    // si³a nacisku na podstawê obiektu (na ko³a pojazdu)
  dlugosc = 7.0;
  szerokosc = 2.8;
  wysokosc = 1.0;
  przeswit = 0.0;           // wysokoœæ na której znajduje siê podstawa obiektu
  dl_przod = 1.0;           // odleg³oœæ od przedniej osi do przedniego zderzaka 
  dl_tyl = 0.2;             // odleg³oœæ od tylniej osi do tylniego zderzaka

  iID_kolid = -1;           // na razie brak kolizji 

  wPol.y = przeswit+wysokosc/2 + 10;
  promien = sqrt(dlugosc*dlugosc + szerokosc*szerokosc + wysokosc*wysokosc)/2/1.15;
  //wV_kat = Wektor3(0,1,0)*40;  // pocz¹tkowa prêdkoœæ k¹towa (w celach testowych)

  moment_wziecia = 0;            // czas ostatniego wziecia przedmiotu
  czas_oczekiwania = 1000000000; // 
  nr_przedmiotu = -1;

  // obrót obiektu o k¹t 30 stopni wzglêdem osi y:
  kwaternion qObr = AsixToQuat(Wektor3(0,1,0),0.1*PI/180.0);
  qOrient = qObr*qOrient;
  
  // losowanie umiejêtnoœci tak by nie by³o bardzo s³abych i bardzo silnych:
  umiejetn_sadzenia = (float)rand()/RAND_MAX;
  umiejetn_zb_paliwa = (float)rand()/RAND_MAX;
  umiejetn_zb_monet = (float)rand()/RAND_MAX;
  float suma_um = umiejetn_sadzenia + umiejetn_zb_paliwa + umiejetn_zb_monet;
  float suma_um_los = 0.7 + 0.8*(float)rand()/RAND_MAX;    // losuje umiejetnoœæ sumaryczn¹
  umiejetn_sadzenia *= suma_um_los/suma_um;
  umiejetn_zb_paliwa *= suma_um_los/suma_um;
  umiejetn_zb_monet *= suma_um_los/suma_um;
  grupa = (unsigned int)rand() % 20;

  zbiera = -1; //-1 = nieokreœlony , 2 monety 3 paliwo
}

ObiektRuchomy::~ObiektRuchomy()            // destruktor
{
}

void ObiektRuchomy::ZmienStan(StanObiektu stan)  // przepisanie podanego stanu 
{                                                // w przypadku obiektów, które nie s¹ symulowane
  this->iID = stan.iID;                        
  this->grupa = stan.grupa;
  this->wPol = stan.wPol;
  this->qOrient = stan.qOrient;
  this->wV = stan.wV;
  this->wA = stan.wA;
  this->wV_kat = stan.wV_kat;
  this->wA_kat = stan.wA_kat;

  this->masa_calk = stan.masa_calk;
  this->pieniadze = stan.pieniadze;
  this->iID_wlasc = stan.iID_wlasc;
  this->czy_autonom = stan.czy_autonom;
  this->zbiera = stan.zbiera;
}

void ObiektRuchomy::SetGrupa(unsigned int grupa){
   this->grupa = grupa;
}

unsigned int ObiektRuchomy::GetGrupa(){
   return this->grupa;
}


StanObiektu ObiektRuchomy::Stan()                // metoda zwracaj¹ca stan obiektu ³¹cznie z iID
{
  StanObiektu stan;

  stan.iID = iID;
  stan.qOrient = qOrient;
  stan.wA = wA;
  stan.wA_kat = wA_kat;
  stan.wPol = wPol;
  stan.wV = wV;
  stan.wV_kat = wV_kat;
  stan.grupa = grupa;
  stan.zbiera = zbiera;

  stan.masa_calk = masa_calk;
  stan.pieniadze = pieniadze;
  stan.iID_wlasc = iID_wlasc;
  stan.czy_autonom = czy_autonom;
  return stan;
}

void ObiektRuchomy::Symulacja(float dt)          // obliczenie nowego stanu na podstawie dotychczasowego,
{                                                // dzia³aj¹cych si³ i czasu, jaki up³yn¹³ od ostatniej symulacji

  if (dt == 0) return;

  float tarcie = 0.6;            // wspó³czynnik tarcia obiektu o pod³o¿e 
  float tarcie_obr = tarcie;     // tarcie obrotowe (w szczególnych przypadkach mo¿e byæ inne ni¿ liniowe)
  float tarcie_toczne = 0.05;    // wspó³czynnik tarcia tocznego
  float sprezystosc = 0.5;       // wspó³czynnik sprê¿ystoœci (0-brak sprê¿ystoœci, 1-doskona³a sprê¿ystoœæ) 
  float g = 9.81;                // przyspieszenie grawitacyjne
  float m = masa_wlasna + ilosc_paliwa;   // masa calkowita


  // obracam uk³ad wspó³rzêdnych lokalnych wed³ug kwaterniona orientacji:
  Wektor3 w_przod = qOrient.obroc_wektor(Wektor3(1,0,0)); // na razie oœ obiektu pokrywa siê z osi¹ x globalnego uk³adu wspó³rzêdnych (lokalna oœ x)
  Wektor3 w_gora = qOrient.obroc_wektor(Wektor3(0,1,0));  // wektor skierowany pionowo w górê od podstawy obiektu (lokalna oœ y)
  Wektor3 w_prawo = qOrient.obroc_wektor(Wektor3(0,0,1)); // wektor skierowany w prawo (lokalna oœ z)

  fprintf(f,"w_przod = (%f, %f, %f)\n",w_przod.x,w_przod.y,w_przod.z);
  fprintf(f,"w_gora = (%f, %f, %f)\n",w_gora.x,w_gora.y,w_gora.z);
  fprintf(f,"w_prawo = (%f, %f, %f)\n",w_prawo.x,w_prawo.y,w_prawo.z);

  fprintf(f,"|w_przod|=%f,|w_gora|=%f,|w_prawo|=%f\n",w_przod.dlugosc(),w_gora.dlugosc(),w_prawo.dlugosc()  );
  fprintf(f,"ilo skalar = %f,%f,%f\n",w_przod^w_prawo,w_przod^w_gora,w_gora^w_prawo  );
  //fprintf(f,"w_przod = (%f, %f, %f) w_gora = (%f, %f, %f) w_prawo = (%f, %f, %f)\n",
  //           w_przod.x,w_przod.y,w_przod.z,w_gora.x,w_gora.y,w_gora.z,w_prawo.x,w_prawo.y,w_prawo.z);


  // rzutujemy wV na sk³adow¹ w kierunku przodu i pozosta³e 2 sk³adowe
  // sk³adowa w bok jest zmniejszana przez si³ê tarcia, sk³adowa do przodu
  // przez si³ê tarcia tocznego
  Wektor3 wV_wprzod = w_przod*(wV^w_przod),
    wV_wprawo = w_prawo*(wV^w_prawo),
    wV_wgore = w_gora*(wV^w_gora); 

  // rzutujemy prêdkoœæ k¹tow¹ wV_kat na sk³adow¹ w kierunku przodu i pozosta³e 2 sk³adowe
  Wektor3 wV_kat_wprzod = w_przod*(wV_kat^w_przod),
    wV_kat_wprawo = w_prawo*(wV_kat^w_prawo),
    wV_kat_wgore = w_gora*(wV_kat^w_gora);         


  // ograniczenia 
  if (F > 4000) F = 4000;
  if (F < -2000) F = -2000;
  if (alfa > PI*45.0/180) alfa = PI*45.0/180 ;
  if (alfa < -PI*45.0/180) alfa = -PI*45.0/180 ;

  // obliczam promien skrêtu pojazdu na podstawie k¹ta skrêtu kó³, a nastêpnie na podstawie promienia skrêtu
  // obliczam prêdkoœæ k¹tow¹ (UPROSZCZENIE! pomijam przyspieszenie k¹towe oraz w³aœciw¹ trajektoriê ruchu)
  if (Fy > 0)
  {
    float V_kat_skret = 0;
    if (alfa != 0)
    {   
      float Rs = sqrt(dlugosc*dlugosc/4 + (fabs(dlugosc/tan(alfa)) + szerokosc/2)*(fabs(dlugosc/tan(alfa)) + szerokosc/2));
      V_kat_skret = wV_wprzod.dlugosc()*(1.0/Rs);
    }	
    Wektor3 wV_kat_skret = w_gora*V_kat_skret*(alfa > 0 ? 1 : -1);
    Wektor3 wV_kat_wgore2 = wV_kat_wgore + wV_kat_skret;
    if (wV_kat_wgore2.dlugosc() <= wV_kat_wgore.dlugosc()) // skrêt przeciwdzia³a obrotowi
    {
      if (wV_kat_wgore2.dlugosc() > V_kat_skret)
        wV_kat_wgore = wV_kat_wgore2; 
      else  
        wV_kat_wgore = wV_kat_skret;
    }    
    else    
    {
      if (wV_kat_wgore.dlugosc() < V_kat_skret)
        wV_kat_wgore = wV_kat_skret;

    }

    // tarcie zmniejsza prêdkoœæ obrotow¹ (UPROSZCZENIE! zamiast masy winienem wykorzystaæ moment bezw³adnoœci)     
    float V_kat_tarcie = Fy*tarcie_obr*dt/m/1.0;      // zmiana pr. k¹towej spowodowana tarciem
    float V_kat_wgore = wV_kat_wgore.dlugosc() - V_kat_tarcie;
    if (V_kat_wgore < V_kat_skret) V_kat_wgore = V_kat_skret;        // tarcie nie mo¿e spowodowaæ zmiany zwrotu wektora pr. k¹towej
    wV_kat_wgore = wV_kat_wgore.znorm()*V_kat_wgore;                     
  }    




  Fy = m*g*w_gora.y;                      // si³a docisku do pod³o¿a 
  if (Fy < 0 ) Fy = 0;
  // ... trzeba j¹ jeszcze uzale¿niæ od tego, czy obiekt styka siê z pod³o¿em!
  float Fh = Fy*tarcie*ham;                  // si³a hamowania (UP: bez uwzglêdnienia poœlizgu)

  float V_wprzod = wV_wprzod.dlugosc();// - dt*Fh/m - dt*tarcie_toczne*Fy/m;
  if (V_wprzod < 0) V_wprzod = 0;

  float V_wprawo = wV_wprawo.dlugosc();// - dt*tarcie*Fy/m;
  if (V_wprawo < 0) V_wprawo = 0;


  // wjazd lub zjazd: 
  //wPol.y = teren.Wysokosc(wPol.x,wPol.z);   // najprostsze rozwi¹zanie - obiekt zmienia wysokoœæ bez zmiany orientacji

  // 1. gdy wjazd na wklês³oœæ: wyznaczam wysokoœci terenu pod naro¿nikami obiektu (ko³ami), 
  // sprawdzam która trójka
  // naro¿ników odpowiada najni¿ej po³o¿onemu œrodkowi ciê¿koœci, gdy przylega do terenu
  // wyznaczam prêdkoœæ podbicia (wznoszenia œrodka pojazdu spowodowanego wklês³oœci¹) 
  // oraz prêdkoœæ k¹tow¹
  // 2. gdy wjazd na wypuk³oœæ to si³a ciê¿koœci wywo³uje obrót przy du¿ej prêdkoœci liniowej

  // punkty zaczepienia kó³ (na wysokoœci pod³ogi pojazdu):
  Wektor3 P = wPol + w_przod*(dlugosc/2-dl_przod) - w_prawo*szerokosc/2 - w_gora*wysokosc/2,
    Q = wPol + w_przod*(dlugosc/2-dl_przod) + w_prawo*szerokosc/2 - w_gora*wysokosc/2,
    R = wPol + w_przod*(-dlugosc/2+dl_tyl) - w_prawo*szerokosc/2 - w_gora*wysokosc/2,
    S = wPol + w_przod*(-dlugosc/2+dl_tyl) + w_prawo*szerokosc/2 - w_gora*wysokosc/2;

  // pionowe rzuty punktów zacz. kó³ pojazdu na powierzchniê terenu:  
  Wektor3 Pt = P, Qt = Q, Rt = R, St = S;
  Pt.y = teren.Wysokosc(P.x,P.z); Qt.y = teren.Wysokosc(Q.x,Q.z);  
  Rt.y = teren.Wysokosc(R.x,R.z); St.y = teren.Wysokosc(S.x,S.z);   
  Wektor3 normPQR = normalna(Pt,Rt,Qt), normPRS = normalna(Pt,Rt,St), normPQS = normalna(Pt,St,Qt),
    normQRS = normalna(Qt,Rt,St);   // normalne do p³aszczyzn wyznaczonych przez trójk¹ty

  fprintf(f,"P.y = %f, Pt.y = %f, Q.y = %f, Qt.y = %f, R.y = %f, Rt.y = %f, S.y = %f, St.y = %f\n",
    P.y, Pt.y, Q.y, Qt.y, R.y,Rt.y, S.y, St.y);

  float sryPQR = ((Qt^normPQR) - normPQR.x*wPol.x - normPQR.z*wPol.z)/normPQR.y, // wys. œrodka pojazdu
    sryPRS = ((Pt^normPRS) - normPRS.x*wPol.x - normPRS.z*wPol.z)/normPRS.y, // po najechaniu na skarpê 
    sryPQS = ((Pt^normPQS) - normPQS.x*wPol.x - normPQS.z*wPol.z)/normPQS.y, // dla 4 trójek kó³
    sryQRS = ((Qt^normQRS) - normQRS.x*wPol.x - normQRS.z*wPol.z)/normQRS.y;
  float sry = sryPQR; Wektor3 norm = normPQR;
  if (sry > sryPRS) {sry = sryPRS; norm = normPRS;}  
  if (sry > sryPQS) {sry = sryPQS; norm = normPQS;}
  if (sry > sryQRS) {sry = sryQRS; norm = normQRS;}  // wybór trójk¹ta o œrodku najni¿ej po³o¿onym    




  Wektor3 wV_kat_wpoziomie = Wektor3(0,0,0);
  // jesli któreœ z kó³ jest poni¿ej powierzchni terenu
  if ((P.y <= Pt.y + wysokosc/2+przeswit)||(Q.y <= Qt.y  + wysokosc/2+przeswit)||
    (R.y <= Rt.y  + wysokosc/2+przeswit)||(S.y <= St.y  + wysokosc/2+przeswit))
  {   
    // obliczam powsta³¹ prêdkoœæ k¹tow¹ w lokalnym uk³adzie wspó³rzêdnych:      
    Wektor3 wobrot = -norm.znorm()*w_gora*0.6;  
    wV_kat_wpoziomie = wobrot/dt; 
  }    

  Wektor3 wAg = Wektor3(0,-1,0)*g;    // przyspieszenie grawitacyjne

  // jesli wiecej niz 2 kola sa na ziemi, to przyspieszenie grawitacyjne jest rownowazone przez opor gruntu:
  if ((P.y <= Pt.y + wysokosc/2+przeswit)+(Q.y <= Qt.y  + wysokosc/2+przeswit)+
    (R.y <= Rt.y  + wysokosc/2+przeswit)+(S.y <= St.y  + wysokosc/2+przeswit) > 2)
  {	
    wAg = wAg + 
      w_gora*(w_gora^wAg)*-1; //przyspieszenie wynikaj¹ce z si³y oporu gruntu
  }
  else   // w przeciwnym wypadku brak sily docisku 
    Fy = 0;





  // sk³adam z powrotem wektor prêdkoœci k¹towej: 
  //wV_kat = wV_kat_wgore + wV_kat_wprawo + wV_kat_wprzod;  
  wV_kat = wV_kat_wgore + wV_kat_wpoziomie;


  float h = sry+wysokosc/2+przeswit - wPol.y;  // ró¿nica wysokoœci jak¹ trzeba pokonaæ  
  float V_podbicia = 0;
  if ((h > 0)&&(wV.y <= 0.01))
    V_podbicia = 0.5*sqrt(2*g*h);  // prêdkoœæ spowodowana podbiciem pojazdu przy wje¿d¿aniu na skarpê 
  if (h > 0) wPol.y = sry+wysokosc/2+przeswit;  

  // lub  w przypadku zag³êbienia siê 
  //fprintf(f,"sry = %f, wPol.y = %f, dt = %f\n",sry,wPol.y,dt);  
  //fprintf(f,"normPQR.y = %f, normPRS.y = %f, normPQS.y = %f, normQRS.y = %f\n",normPQR.y,normPRS.y,normPQS.y,normQRS.y); 

  Wektor3 dwPol = wV*dt;//wA*dt*dt/2; // czynnik bardzo ma³y - im wiêksza czêstotliwoœæ symulacji, tym mniejsze znaczenie 
  wPol = wPol + dwPol;  

  // korekta po³o¿enia w przypadku terenu cyklicznego:
  if (wPol.x < -teren.rozmiar_pola*teren.lkolumn/2) wPol.x += teren.rozmiar_pola*teren.lkolumn;
  else if (wPol.x > teren.rozmiar_pola*(teren.lkolumn-teren.lkolumn/2)) wPol.x -= teren.rozmiar_pola*teren.lkolumn;
  if (wPol.z < -teren.rozmiar_pola*teren.lwierszy/2) wPol.z += teren.rozmiar_pola*teren.lwierszy;
  else if (wPol.z > teren.rozmiar_pola*(teren.lwierszy-teren.lwierszy/2)) wPol.z -= teren.rozmiar_pola*teren.lwierszy;

  // Sprawdzenie czy obiekt mo¿e siê przemieœciæ w zadane miejsce: Jeœli nie, to 
  // przemieszczam obiekt do miejsca zetkniêcia, wyznaczam nowe wektory prêdkoœci
  // i prêdkoœci k¹towej, a nastêpne obliczam nowe po³o¿enie na podstawie nowych
  // prêdkoœci i pozosta³ego czasu. Wszystko powtarzam w pêtli (pojazd znowu mo¿e 
  // wjechaæ na przeszkodê). Problem z zaokr¹glonymi przeszkodami - konieczne 
  // wyznaczenie minimalnego kroku.


  Wektor3 wV_pop = wV;  

  // sk³adam prêdkoœci w ró¿nych kierunkach oraz efekt przyspieszenia w jeden wektor:    (problem z przyspieszeniem od si³y tarcia -> to przyspieszenie 
  //      mo¿e dzia³aæ krócej ni¿ dt -> trzeba to jakoœ uwzglêdniæ, inaczej poazd bêdzie wê¿ykowa³)
  wV = wV_wprzod.znorm()*V_wprzod + wV_wprawo.znorm()*V_wprawo + wV_wgore + 
    Wektor3(0,1,0)*V_podbicia + wA*dt;
  // usuwam te sk³adowe wektora prêdkoœci w których kierunku jazda nie jest mo¿liwa z powodu
  // przeskód:
  // np. jeœli pojazd styka siê 3 ko³ami z nawierzchni¹ lub dwoma ko³ami i œrodkiem ciê¿koœci to
  // nie mo¿e mieæ prêdkoœci w dó³ pod³ogi
  if ((P.y <= Pt.y  + wysokosc/2+przeswit)||(Q.y <= Qt.y  + wysokosc/2+przeswit)||  
    (R.y <= Rt.y  + wysokosc/2+przeswit)||(S.y <= St.y  + wysokosc/2+przeswit))    // jeœli pojazd styka siê co najm. jednym ko³em
  {
    Wektor3 dwV = wV_wgore + w_gora*(wA^w_gora)*dt;
    if ((w_gora.znorm() - dwV.znorm()).dlugosc() > 1 )  // jeœli wektor skierowany w dó³ pod³ogi
      wV = wV - dwV;
  }

  /*fprintf(f," |wV_wprzod| %f -> %f, |wV_wprawo| %f -> %f, |wV_wgore| %f -> %f |wV| %f -> %f\n",
  wV_wprzod.dlugosc(), (wV_wprzod.znorm()*V_wprzod).dlugosc(), 
  wV_wprawo.dlugosc(), (wV_wprawo.znorm()*V_wprawo).dlugosc(),
  wV_wgore.dlugosc(), (wV_wgore.znorm()*wV_wgore.dlugosc()).dlugosc(),
  wV_pop.dlugosc(), wV.dlugosc()); */

  // sk³adam przyspieszenia liniowe od si³ napêdzaj¹cych i od si³ oporu: 
  wA = (w_przod*F + w_prawo*Fb)/m*(Fy>0)*(ilosc_paliwa>0)  // od si³ napêdzaj¹cych
    - wV_wprzod.znorm()*(Fh/m + tarcie_toczne*Fy/m)*(V_wprzod>0.01) // od hamowania i tarcia tocznego (w kierunku ruchu)
    - wV_wprawo.znorm()*tarcie*Fy/m*(V_wprawo>0.01)    // od tarcia w kierunku prost. do kier. ruchu
    + wAg;           // od grawitacji


  // utrata paliwa:
  ilosc_paliwa -= (fabs(F) + fabs(Fb))*(Fy>0)*dt/20000;      
  if (ilosc_paliwa < 0 ) ilosc_paliwa = 0;     
  masa_calk = masa_wlasna + ilosc_paliwa;


  // obliczenie nowej orientacji:
  Wektor3 w_obrot = wV_kat*dt;// + wA_kat*dt*dt/2;    
  kwaternion q_obrot = AsixToQuat(w_obrot.znorm(),w_obrot.dlugosc());
  //fprintf(f,"w_obrot = (x=%f, y=%f, z=%f) \n",w_obrot.x, w_obrot.y, w_obrot.z );
  //fprintf(f,"q_obrot = (w=%f, x=%f, y=%f, z=%f) \n",q_obrot.w, q_obrot.x, q_obrot.y, q_obrot.z );
  qOrient = q_obrot*qOrient; 	
  fprintf(f,"Pol = (%f, %f, %f) V = (%f, %f, %f) A = (%f, %f, %f) V_kat = (%f, %f, %f) ID = %d\n",
    wPol.x,wPol.y,wPol.z,wV.x,wV.y,wV.z,wA.x,wA.y,wA.z,wV_kat.x,wV_kat.y,wV_kat.z,iID);

  // wykrywanie kolizji z drzewami:
  for (long i=0;i<teren.liczba_przedmiotow;i++)    
    if (teren.p[i].typ == DRZEWO)
    {
      // bardzo duze uproszczenie -> traktuje pojazd jako kulê
      Wektor3 wPolDrz = teren.p[i].wPol; 
      wPolDrz.y = (wPolDrz.y + teren.p[i].wartosc > wPol.y ? wPol.y : wPolDrz.y + teren.p[i].wartosc);                        
      if  ((wPolDrz - wPol).dlugosc() < promien + teren.p[i].srednica/2)  // jesli kolizja
      {             
        // od wektora predkosci odejmujemy jego rzut na kierunek od punktu styku do osi drzewa:
        // jesli pojazd juz wjechal w drzewo, to nieco zwiekszamy poprawke     
        // punkt styku znajdujemy laczac krawedz pojazdu z osia drzewa odcinkiem
        // do obu prostopadlym      
        Wektor3 dP = (wPolDrz - wPol).znorm();   // wektor, w ktorego kierunku ruch jest niemozliwy
        float k = wV^dP;
        if (k > 0)     // jesli jest skladowa predkosci w strone drzewa
        {
          wV = wV - dP*k*(1 + sprezystosc);  // odjecie skladowej + odbicie sprezyste 
          //wV_kat = 
        }  
      }                              
    }	

    // kolizje z innymi obiektami
    if (iID_kolid == iID) // ktoœ o numerze iID_kolid wykry³ kolizjê z naszym pojazdem i poinformowa³ nas o tym

    {
      fprintf(f,"ktos wykryl kolizje - modyf. predkosci\n",iID_kolid);
      wV = wV + wdV_kolid;   // modyfikuje prêdkoœæ o wektor obliczony od drugiego (¿yczliwego) uczestnika
      iID_kolid = -1;

    }
    else      
    {
      for (long i=0;i<iLiczbaCudzychOb;i++)
      {
        ObiektRuchomy *inny = CudzeObiekty[i];      

        if ((wPol - inny->wPol).dlugosc() < 2*promien)  // jeœli kolizja (zak³adam, ¿e drugi obiekt ma taki sam promieñ
        {
          // zderzenie takie jak w symulacji kul 
          Wektor3 norm_pl_st = (wPol - inny->wPol).znorm();    // normalna do p³aszczyzny stycznej - kierunek odbicia
          float m1 = masa_calk, m2 = inny->masa_calk;          // masy obiektów
          float W1 = wV^norm_pl_st, W2 = inny->wV^norm_pl_st;  // wartosci prêdkoœci
          if (W2>W1)      // jeœli obiekty siê przybli¿aj¹
          {

            float Wns = (m1*W1 + m2*W2)/(m1+m2);        // prêdkoœæ po zderzeniu ca³kowicie niesprê¿ystym
            float W1s = ((m1-m2)*W1 + 2*m2*W2)/(m1+m2), // prêdkoœæ po zderzeniu ca³kowicie sprê¿ystym
              W2s = ((m2-m1)*W2 + 2*m1*W1)/(m1+m2);
            float W1sp = Wns +(W1s-Wns)*sprezystosc;    // prêdkoœæ po zderzeniu sprê¿ysto-plastycznym
            float W2sp = Wns +(W2s-Wns)*sprezystosc;

            wV = wV + norm_pl_st*(W1sp-W1);    // poprawka prêdkoœci (zak³adam, ¿e inny w przypadku drugiego obiektu zrobi to jego w³asny symulator) 
            iID_kolid = inny->iID;
            wdV_kolid = norm_pl_st*(W2sp-W2);
            fprintf(f,"wykryto i zreal. kolizje z %d W1=%f,W2=%f,W1s=%f,W2s=%f,m1=%f,m2=%f\n",iID_kolid,W1,W2,W1s,W2s,m1,m2); 
          }
          //if (fabs(W2 - W1)*dt < (wPol - inny->wPol).dlugosc() < 2*promien) wV = wV + norm_pl_st*(W1sp-W1)*2;
        }
      }
    } // do else
}

void ObiektRuchomy::Rysuj()
{
  glPushMatrix();


  glTranslatef(wPol.x,wPol.y+przeswit,wPol.z);


  kwaternion k = qOrient.AsixAngle();
  //fprintf(f,"kwaternion = [%f, %f, %f], w = %f\n",qOrient.x,qOrient.y,qOrient.z,qOrient.w);
  //fprintf(f,"os obrotu = [%f, %f, %f], kat = %f\n",k.x,k.y,k.z,k.w*180.0/PI);

  glRotatef(k.w*180.0/PI,k.x,k.y,k.z);
  glTranslatef(-dlugosc/2,-wysokosc/2,-szerokosc/2);
  glScalef(dlugosc,wysokosc,szerokosc);

  glCallList(Cube);
  GLfloat Surface[] = { 2.0f, 2.0f, 1.0f, 1.0f};
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
  glRasterPos2f(0.30,1.20);
  glPrint("%d,%d", iID, grupa);
  glPopMatrix();
}



//**********************
//   Obiekty nieruchome
//**********************
Teren::Teren()
{
  rozmiar_pola = 42;         // d³ugoœæ boku kwadratu w [m]           
  float t[][44] = { {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // ostatni element nieu¿ywany
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 80,170,170,200,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  5,  5,  2,  0,  0,  0,  0,  0,  0,  0,120,220,250,200,200,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  3,  6,  9, 12, 12,  4,  0,  0,  0,  0,  0, 40,130,200,250,200,150,  0,  0,  0,  0,  0,  0,  0,100,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  2,  2,  0,  0, 14,  9,  4,  0,  0,  0, 20, 40,120,200,220,150,150,  0,  0,  0,  0, 50, 50,300,300,300,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  1,  1,  0,  1,  1,  1,  2,  2,  0,  0,  8,  4,  0,  0,  0,  0, 20, 40, 90,120,170,  0,  0,  0,  0,  0,  0, 60,300,350,330,300,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0, -1,  2,  0, -3,  0,  2,  1,  2,  0,  0,  0,  0,  0,  0, 10, 20, 30, 40, 50,100,140,  0,  0,  0,  0,  0,  0, 50,300,300,300,150, 50,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0, -1,  2,  0,  0,  0,  0, -1, -1,  2,  0,  0,  0,  0,  0,  0,  0, 10, 10, 40, 70,100,110,  0,  0,  0,  0,  0,  0, 50, 40,300,200, 50, 50,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  1,  0,  0, -1,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10, 90, 90,  0,  0,  0,  0,  0,  0,  0,100, 40,100, 50, 50,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-10,-10, 10, 10,  0,  0,  0,  0,  0,  0,  0,100,100,  0,100, 70, 40,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-10, 40, 40,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 70, 70, 30,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 70, 70,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 70, 20, 20,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,100,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 70, 70,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0, -2, -2, -2, -2,  0,  0,  0,  0,  0,  0,  0, -1, -1,  0,  0, 70,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0, -6, -5, -5, -3, -3,  0,  0,  0,  0,  0,  0, -2, -2, -1,  0, 70, 70,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -7, -6, -3, -3, -5, -4,  0,  0,  0,  0,  0, -1, -3, -3, -2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -8, -8,  0,  0,  0, -4, -2,  0,  0,  0,  0,  0, -2, -3, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -8,  0,  0,  0,  0, -2,  0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},  
  {0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0},        
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 10,-10,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 10,-20,-10,  0,  0,  0,  7,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, 16, 10,-10,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0, 10, 10, 10,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10, 10, 10,  0,  0,  0,  0,  0,-20,-20,-20,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0, 10, 10, 10,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10, 10,  3,  0,  0,  0,  0,-20,-20,-20,-20,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10, 20, 20, 10,  5,  5,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 10, 10,  5,  0,  0,  0,  0,  0,-20,-40,-40,-20,-20,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 20, 20, 20, 10,  5,  5,  0,  0,  0,  0},
  {0,  0,  0,  0,  0, -3,  0,  0,-10,-10,  0,  2, 10,  5,  0,  0,  1,  0,  0,-20,-40,-40,-40,-40,-40,-20,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20, 10,  5,  5,  0,  0,  0,  0},
  {0,  0,  0,  0,  0, -3,  0,-13,-10, -6,  0,  0,  5,  0,  0,  1,  3,  0,  0,-20,-40,-40,-40,-40,-40,-20,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20, 10,  5,  5,  5,  0,  0,  0},
  {0,  0,  0,  0,  0,  0, -3,  0,-18,-16,  0,  0,  0,  0,  0,  0,  2,  3,  5,-20,-40,-40,-40,-40,-40,-20,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20, 10,  5,  5,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  4,  0,-20,-40,-40,-40,-40,-40,-20,  0,  0,  0,  0,  0,  3,  5, 10, 20, 20, 20, 10,  5,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  2,  3,  5,-20,-20,-20,-20,-20,-20,-20,  0,  0,  0,  0,  0,  3,  5, 10, 20, 20, 20, 10,  5,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  1,  2,  0,  0,-20,-20,-20,-20,-20,-20,-20,  0,  0,  0,  0,  0,  3,  5, 10, 20, 20, 20, 10,  5,  0, 20, 20,  0,  0},
  {0,  0,  0,  0,  0,  0,  0, -3, -3,  0,  0,  0,  0,  0,  0,  0,  4,  4,  0,-20,-20,-20,-20,-20,-20,-20,  0,  0,  0,  0,  0,  0,  3,  0, 20, 20, 20, 10,  0,  0, 20, 20,  0,  0},
  {0,  0,  3,  0,  0,  0, -3, -5, -3,  0,  0,  0,  0,  0,  0,  2,  4,  2,  0,-20,-20,-20,-20,-20,-20,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20, 10,  0,  0, 20, 20,  0,  0},
  {0,  0,  3,  0,  0,  0, -3, -5, -3,  0,  0,  0,  0,  0,  0,  0,  4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 30, 30, 30, 20, 20, 20, 20,  0},
  {0,  0,  3,  0,  0, -3, -3, -3,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 40, 40, 40, 40, 40, 40, 40, 40,  0},
  {0,  0,  0,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 30, 40, 40, 60, 60, 60, 60, 40, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 30, 40, 40, 60, 60, 60, 60, 40, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 30, 40, 40, 60, 60, 60, 60, 40, 40,  0},
  {0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0,-50,  0,  0,  0,  0,  1, -1,  0,  0,  0,  3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0, 30, 20, 30, 40, 60, 60, 60, 60, 60, 40, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-20,-50,  0,  0,  0,  0,  1, -1,  0,  0,  1,  5,  8,  0,  0,  0,  0,  0,  0,  0,  0, 30, 30, 40, 60, 60, 60, 60, 60, 60, 60, 40,  0},
  {0,  0,  0,  0,  0,  0,  2,  2,  2,  1,-20,-20,-30,  0,  0,  0,  0,  1, -1,  0,  0,  2,  5,  9,  0,  0,  0,  0,  0,  0,  0, 20, 30, 40, 60, 60,100,100,100, 60, 60, 60, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-20,-10,  0,  0,  0,  0,  0,  1, -1,  0,  0,  2,  5,  7,  0,  0,  0,  0,  0,  0, 20, 30, 40, 60,100,100,100,100,100, 60, 60, 40,  0},
  {0,  0,  0,  0,  0,  4,  4,  2,  3,  2,  1, -5,  0,  0,  0,  0,  0,  0,  1, -1,  0,  0,  2,  4,  0,  0,  0,  0,  0,  0,  0, 20, 30, 40, 60,100,100,100,120,100,100, 60, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 30, 40, 60,100,100, 80, 80,100,100, 60, 40,  0},
  {0,  0,  0,  0,  0,  4,  3,  2,  2,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 30, 40, 60,100,100,100, 80,100,100, 60, 40,  0},
  {0,  0,  0,  0,  0,  0,  0,0.5,  1,  1,  0,  0,  0,  0,  0,  0,-30,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 40, 60, 60,100,100,100,100, 60, 60, 40,  0},
  {0,  0,  0,  0,  0,  4,  4,  2,  3,  1,  1,  1,  0,  0,  0,-30,-30,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 60, 60,100,100, 60, 60, 60, 40,  0},          
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,-30,-30,-30,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 60, 60, 60, 60, 60, 60, 60, 40,  0},
  {0,  0,  0,  0,  0,  5,  4,  2,  2,  1,  1,  1,  0,  0,  0,-30,-30,-25,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20, 40, 20, 40, 40, 40, 40,  0},  
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,-25,-22,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 40, 20, 20, 40, 40,  0,  0},
  {0,  0,  0,  0,  0,-20,60,-20,  0,  0,  0,  0,  0,  0,  0, 10,  0,-22,-20,  0,  0, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0, 70,60,-20,  0,  0,  0,  0,  0,  0, 10, 10,  0,-19,-18,  0, -6, -3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0, 65,50,  0,  0,  0,  0,  0,  0,  5, 10,  0,  0,-16,-13, -8, -6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,-20,  0,  0,  0,  0,  0,  0,  2,  5,  0,  0,  0,-13,-10, -8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,-10,-20,-60,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,-10,-20,-30,-60,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,-10,-40,-90,-60,-60,  0,  0,  0,  0,  0,  0,  0, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 20,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,-20,-40,-90,-90,-60,  0,  0,  0,  0,  0,  0, 10, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 20, 20, 20,  0,  0,  0,  0,  0,  0},
  {0,-10,-40,-90,-90,-90,-60,  0,  0,  0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 20, 40, 40,  0,  0,  0,  0,  0,  0},
  {0,-10,-50,-90,-90,-90,-60,  0,  0,  0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0, 10, 40, 40,-20,-10,  0,  0,  0,  0,  0},
  {0,-10,-50,-70,-90,-60,-40,  0,  0,  0, 10, 10, 10, 20, 20, 20, 20, 30, 30, 40, 40, 50, 50, 70, 70, 10, 10,  0,  0,  0,  0,  0,  0,  0, 10, 20, 40,-20,-10,  0,  0,  0,  0,  0},
  {0,-10,-20,-40,-40,-40,-40,  0,  0,  0, 10, 10, 10, 20, 20, 20, 20, 30, 30, 40, 40, 50, 50, 70, 70, 10, 10,  0,  0,  0,  0,  0,  0,  0, 10, 10, 40, 40,  0,  0,  0,  0,  0,  0},
  {0,-10,-20,-20,-30,-20,-20,-10,  0,  0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10, 40,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,-10,-20,-20,-10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10, 10,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,-10,-10,-10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}};

  lkolumn = 43;         // o 1 mniej, gdy¿ kolumna jest równowa¿na ci¹gowi kwadratów 
  lwierszy = sizeof(t)/sizeof(float)/(lkolumn+1)/2-1;                 
  mapa = new float*[lwierszy*2+1];
  for (long i=0;i<lwierszy*2+1;i++) {
    mapa[i] = new float[lkolumn+1];
    for (long j=0;j<lkolumn+1;j++) mapa[i][j] = t[i][j];
  }  	

  for (long i=0;i<lwierszy*2+1;i++)
    for (long j=0;j<lkolumn+1;j++)
      mapa[i][j] /= 3;

  d = new float**[lwierszy];
  for (long i=0;i<lwierszy;i++) {
    d[i] = new float*[lkolumn];
    for (long j=0;j<lkolumn;j++) d[i][j] = new float[4];
  }    
  Norm = new Wektor3**[lwierszy];
  for (long i=0;i<lwierszy;i++) {
    Norm[i] = new Wektor3*[lkolumn];
    for (long j=0;j<lkolumn;j++) Norm[i][j] = new Wektor3[4];
  }    

  fprintf(f,"mapa terenu: lwierszy = %d, lkolumn = %d rozmiar_mapy = %d\n",lwierszy,lkolumn,sizeof(t));

  
  liczba_przedmiotow_max = 1000;
  p = new Przedmiot[liczba_przedmiotow_max];
  p[0].typ = MONETA; p[0].wPol = Wektor3(10,0,10); p[0].do_wziecia = 1; p[0].wartosc = 100;
  p[1].typ = MONETA; p[1].wPol = Wektor3(10,0,-10); p[1].do_wziecia = 1; p[1].wartosc = 100;
  p[2].typ = MONETA; p[2].wPol = Wektor3(-10,0,-20); p[2].do_wziecia = 1; p[2].wartosc = 100;
  p[3].typ = MONETA; p[3].wPol = Wektor3(60,0,-60); p[3].do_wziecia = 1; p[3].wartosc = 500;
  p[4].typ = MONETA; p[4].wPol = Wektor3(-40,0,-20); p[4].do_wziecia = 1; p[4].wartosc = 200;
  p[5].typ = MONETA; p[5].wPol = Wektor3(40,0,50); p[5].do_wziecia = 1; p[5].wartosc = 200;
  p[6].typ = DRZEWO; p[6].podtyp = TOPOLA; p[6].wPol = Wektor3(-40,0,50); p[6].do_wziecia = 0; p[6].wartosc = 10;
  p[7].typ = DRZEWO; p[7].podtyp = TOPOLA; p[7].wPol = Wektor3(40,0,55); p[7].do_wziecia = 0; p[7].wartosc = 14;
  p[8].typ = DRZEWO; p[8].podtyp = TOPOLA; p[8].wPol = Wektor3(45,0,55); p[8].do_wziecia = 0; p[8].wartosc = 10;
  p[9].typ = DRZEWO; p[9].podtyp = TOPOLA; p[9].wPol = Wektor3(45,0,37); p[9].do_wziecia = 0; p[9].wartosc = 8;
  p[10].typ = DRZEWO; p[10].podtyp = SWIERK; p[10].wPol = Wektor3(45,0,5); p[10].do_wziecia = 0; p[10].wartosc = 7;
  p[11].typ = DRZEWO; p[11].podtyp = SWIERK; p[11].wPol = Wektor3(38,0,7); p[11].do_wziecia = 0; p[11].wartosc = 8;
  p[12].typ = DRZEWO; p[12].podtyp = SWIERK; p[12].wPol = Wektor3(8,0,57); p[12].do_wziecia = 0; p[12].wartosc = 8;
  p[13].typ = DRZEWO; p[13].podtyp = SWIERK; p[13].wPol = Wektor3(-2,0,60); p[13].do_wziecia = 0; p[13].wartosc = 8;
  p[14].typ = DRZEWO; p[14].podtyp = SWIERK; p[14].wPol = Wektor3(-8,0,52); p[14].do_wziecia = 0; p[14].wartosc = 12;
  p[15].typ = DRZEWO; p[15].podtyp = TOPOLA; p[15].wPol = Wektor3(107,0,-65); p[15].do_wziecia = 0; p[15].wartosc = 18;
  p[16].typ = DRZEWO; p[16].podtyp = TOPOLA; p[16].wPol = Wektor3(110,0,-75); p[16].do_wziecia = 0; p[16].wartosc = 17;
  p[17].typ = DRZEWO; p[17].podtyp = TOPOLA; p[17].wPol = Wektor3(110,0,-85); p[17].do_wziecia = 0; p[17].wartosc = 19;
  p[19].typ = MONETA; p[19].wPol = Wektor3(80,0,-97); p[19].do_wziecia = 1; p[19].wartosc = 500;
  p[20].typ = MONETA; p[20].wPol = Wektor3(-30,0,-78); p[20].do_wziecia = 1; p[20].wartosc = 200;
  p[21].typ = MONETA; p[21].wPol = Wektor3(40,0,-88); p[21].do_wziecia = 1; p[21].wartosc = 100;
  p[22].typ = MONETA; p[22].wPol = Wektor3(-70,0,-88); p[22].do_wziecia = 1; p[22].wartosc = 100;
  p[23].typ = MONETA; p[23].wPol = Wektor3(-70,0,-68); p[23].do_wziecia = 1; p[23].wartosc = 100;
  p[24].typ = DRZEWO; p[24].podtyp = SWIERK; p[24].wPol = Wektor3(-30,0,-35); p[24].do_wziecia = 0; p[24].wartosc = 8;
  p[25].typ = DRZEWO; p[25].podtyp = SWIERK; p[25].wPol = Wektor3(-25,0,-25); p[25].do_wziecia = 0; p[25].wartosc = 11;
  p[26].typ = MONETA; p[26].wPol = Wektor3(-37,0,-34); p[26].do_wziecia = 1; p[26].wartosc = 1000;
  p[27].typ = DRZEWO; p[27].podtyp = TOPOLA; p[27].wPol = Wektor3(140,0,120); p[27].do_wziecia = 0; p[27].wartosc = 20;
  p[28].typ = DRZEWO; p[28].podtyp = TOPOLA; p[28].wPol = Wektor3(113,0,-97); p[28].do_wziecia = 0; p[28].wartosc = 19;
  p[29].typ = BECZKA; p[29].wPol = Wektor3(-90,0,-88); p[29].do_wziecia = 1; p[29].wartosc = 10;
  p[30].typ = BECZKA; p[30].wPol = Wektor3(-100,0,-68); p[30].do_wziecia = 1; p[30].wartosc = 10;
  p[31].typ = BECZKA; p[31].wPol = Wektor3(120,0,-65); p[31].do_wziecia = 1; p[31].wartosc = 10;
  p[32].typ = BECZKA; p[32].wPol = Wektor3(130,0,-99); p[32].do_wziecia = 1; p[32].wartosc = 50;
  p[33].typ = BECZKA; p[33].wPol = Wektor3(150,0,-78); p[33].do_wziecia = 1; p[33].wartosc = 20;
  p[34].typ = BECZKA; p[34].wPol = Wektor3(-110,0,-130); p[34].do_wziecia = 1; p[34].wartosc = 10;
  p[35].typ = BECZKA; p[35].wPol = Wektor3(123,0,-35); p[35].do_wziecia = 1; p[35].wartosc = 10;
  p[36].typ = BECZKA; p[36].wPol = Wektor3(137,0,-110); p[36].do_wziecia = 1; p[36].wartosc = 40;
  p[37].typ = BECZKA; p[37].wPol = Wektor3(140,0,-118); p[37].do_wziecia = 1; p[37].wartosc = 30;
  p[38].typ = BECZKA; p[38].wPol = Wektor3(12,0,-165); p[38].do_wziecia = 1; p[38].wartosc = 10;
  p[39].typ = BECZKA; p[39].wPol = Wektor3(17,0,-110); p[39].do_wziecia = 1; p[39].wartosc = 20;
  p[40].typ = BECZKA; p[40].wPol = Wektor3(14,0,-118); p[40].do_wziecia = 1; p[40].wartosc = 10;
  p[41].typ = MONETA; p[41].wPol = Wektor3(145,0,-12); p[41].do_wziecia = 1; p[41].wartosc = 200;
  p[42].typ = MONETA; p[42].wPol = Wektor3(147,0,-200); p[42].do_wziecia = 1; p[42].wartosc = 200;
  p[43].typ = MONETA; p[43].wPol = Wektor3(140,0,123); p[43].do_wziecia = 1; p[43].wartosc = 200;
  p[44].typ = MONETA; p[44].wPol = Wektor3(137,0,134); p[44].do_wziecia = 1; p[44].wartosc = 200;
  p[45].typ = DRZEWO; p[45].podtyp = TOPOLA; p[45].wPol = Wektor3(178,0,-194); p[45].do_wziecia = 0; p[45].wartosc = 20;
  p[46].typ = DRZEWO; p[46].podtyp = TOPOLA; p[46].wPol = Wektor3(180,0,-188); p[46].do_wziecia = 0; p[46].wartosc = 19;
  p[47].typ = BECZKA; p[47].wPol = Wektor3(-190,0,158); p[47].do_wziecia = 1; p[47].wartosc = 20;
  p[48].typ = BECZKA; p[48].wPol = Wektor3(-176,0,165); p[48].do_wziecia = 1; p[48].wartosc = 20;
  p[49].typ = BECZKA; p[49].wPol = Wektor3(-140,0,-68); p[49].do_wziecia = 1; p[49].wartosc = 20;
  p[50].typ = BECZKA; p[50].wPol = Wektor3(-136,0,-55); p[50].do_wziecia = 1; p[50].wartosc = 20;
  p[51].typ = MONETA; p[51].wPol = Wektor3(31,0,204); p[51].do_wziecia = 1; p[51].wartosc = 100;
  p[52].typ = MONETA; p[52].wPol = Wektor3(31,0,255); p[52].do_wziecia = 1; p[52].wartosc = 500;
  p[53].typ = MONETA; p[53].wPol = Wektor3(-71,0,40); p[53].do_wziecia = 1; p[53].wartosc = 100;
  p[54].typ = MONETA; p[54].wPol = Wektor3(31,0,75); p[54].do_wziecia = 1; p[54].wartosc = 100;
  p[55].typ = DRZEWO; p[55].podtyp = TOPOLA; p[55].wPol = Wektor3(200,0,-194); p[55].do_wziecia = 0; p[55].wartosc = 20;
  p[56].typ = DRZEWO; p[56].podtyp = TOPOLA; p[56].wPol = Wektor3(320,0,74); p[56].do_wziecia = 0; p[56].wartosc = 20;
  p[57].typ = DRZEWO; p[57].podtyp = TOPOLA; p[57].wPol = Wektor3(320,0,74); p[57].do_wziecia = 0; p[57].wartosc = 20;
  p[58].typ = DRZEWO; p[58].podtyp = TOPOLA; p[58].wPol = Wektor3(328,0,74); p[58].do_wziecia = 0; p[58].wartosc = 24;
  p[59].typ = DRZEWO; p[59].podtyp = SWIERK; p[59].wPol = Wektor3(340,0,64); p[59].do_wziecia = 0; p[59].wartosc = 18;
  p[60].typ = DRZEWO; p[60].podtyp = TOPOLA; p[60].wPol = Wektor3(320,0,84); p[60].do_wziecia = 0; p[60].wartosc = 29;
  p[61].typ = MONETA; p[61].wPol = Wektor3(300,0,94); p[61].do_wziecia = 1; p[61].wartosc = 2000;
  p[62].typ = MONETA; p[62].wPol = Wektor3(290,0,107); p[62].do_wziecia = 1; p[62].wartosc = 2000;
  p[63].typ = DRZEWO; p[63].podtyp = SWIERK; p[63].wPol = Wektor3(310,0,110); p[63].do_wziecia = 0; p[63].wartosc = 23;

  
  liczba_przedmiotow = 64;

  //Przedmiot _p = {{Wektor3(1,2,3),kwaternion(0,0,0,1),MONETA,100,1}}; 
  for (long i = 0;i<liczba_przedmiotow;i++) 
  {
    p[i].czy_odnawialny = 1;
    if ((p[i].typ == MONETA)||(p[i].typ == BECZKA))
      p[i].wPol.y = 1.7 + Wysokosc(p[i].wPol.x,p[i].wPol.z);
  }
}

Teren::~Teren()
{
  for (long i = 0;i< lwierszy*2+1;i++) delete mapa[i];             
  delete mapa;   
  for (long i=0;i<lwierszy;i++)  {
    for (long j=0;j<lkolumn;j++) delete d[i][j];
    delete d[i];
  }
  delete d;  
  for (long i=0;i<lwierszy;i++)  {
    for (long j=0;j<lkolumn;j++) delete Norm[i][j];
    delete Norm[i];
  }
  delete Norm;  

  delete p;         
}

float Teren::Wysokosc(float x,float z)      // okreœlanie wysokoœci dla punktu o wsp. (x,z) 
{

  float pocz_x = -rozmiar_pola*lkolumn/2,     // wspó³rzêdne lewego górnego krañca terenu
    pocz_z = -rozmiar_pola*lwierszy/2;        

  long k = (long)((x - pocz_x)/rozmiar_pola), // wyznaczenie wspó³rzêdnych (w,k) kwadratu
    w = (long)((z - pocz_z)/rozmiar_pola);
  if ((k < 0)||(k >= lkolumn)||(w < 0)||(w >= lwierszy)) return 0;  // jeœli poza map¹

  // wyznaczam punkt B - œrodek kwadratu oraz trójk¹t, w którym znajduje siê punkt
  // (rysunek w Teren::PoczatekGrafiki())
  Wektor3 B = Wektor3(pocz_x + (k+0.5)*rozmiar_pola, mapa[w*2+1][k], pocz_z + (w+0.5)*rozmiar_pola); 
  enum tr{ABC=0,ADB=1,BDE=2,CBE=3};       // trójk¹t w którym znajduje siê punkt 
  int trojkat=0; 
  if ((B.x > x)&&(fabs(B.z - z) < fabs(B.x - x))) trojkat = ADB;
  else if ((B.x < x)&&(fabs(B.z - z) < fabs(B.x - x))) trojkat = CBE;
  else if ((B.z > z)&&(fabs(B.z - z) > fabs(B.x - x))) trojkat = ABC;
  else trojkat = BDE;

  // wyznaczam normaln¹ do p³aszczyzny a nastêpnie wspó³czynnik d z równania p³aszczyzny
  float dd = d[w][k][trojkat];
  Wektor3 N = Norm[w][k][trojkat];
  float y;
  if (N.y > 0) y = (-dd - N.x*x - N.z*z)/N.y;
  else y = 0;

  //fprintf(f,"kwadr = (w=%d,k=%d), trojk = %d,  (x,z)=(%f,%f) y = %f\n",w,k,trojkat,x,z,y);
  //fprintf(f,"kwadr = (w=%d,k=%d), trojk = %d, N=(%f, %f, %f)\n",w,k,trojkat,N.x,N.y,N.z);

  return y;    
}

void Teren::PoczatekGrafiki()
{
  // tworze listê wyœwietlania rysuj¹c poszczególne pola mapy za pomoc¹ trójk¹tów 
  // (po 4 trójk¹ty na ka¿de pole):
  enum tr{ABC=0,ADB=1,BDE=2,CBE=3};       
  float pocz_x = -rozmiar_pola*lkolumn/2,     // wspó³rzêdne lewego górnego krañca terenu
    pocz_z = -rozmiar_pola*lwierszy/2;        
  Wektor3 A,B,C,D,E,N;      
  glNewList(PowierzchniaTerenu,GL_COMPILE);
  glBegin(GL_TRIANGLES);
  for (long w=0;w<lwierszy;w++) 
    for (long k=0;k<lkolumn;k++) 
    {
      A = Wektor3(pocz_x + k*rozmiar_pola, mapa[w*2][k], pocz_z + w*rozmiar_pola);
      B = Wektor3(pocz_x + (k+0.5)*rozmiar_pola, mapa[w*2+1][k], pocz_z + (w+0.5)*rozmiar_pola);            
      C = Wektor3(pocz_x + (k+1)*rozmiar_pola, mapa[w*2][k+1], pocz_z + w*rozmiar_pola); 
      D = Wektor3(pocz_x + k*rozmiar_pola, mapa[(w+1)*2][k], pocz_z + (w+1)*rozmiar_pola);       
      E = Wektor3(pocz_x + (k+1)*rozmiar_pola, mapa[(w+1)*2][k+1], pocz_z + (w+1)*rozmiar_pola); 
      // tworzê trójk¹t ABC w górnej czêœci kwadratu: 
      //  A o_________o C
      //    |.       .|
      //    |  .   .  | 
      //    |    o B  | 
      //    |  .   .  |
      //    |._______.|
      //  D o         o E

      Wektor3 AB = B-A;
      Wektor3 BC = C-B;
      N = (AB*BC).znorm();          
      glNormal3f( N.x, N.y, N.z);
      glVertex3f( A.x, A.y, A.z);
      glVertex3f( B.x, B.y, B.z);
      glVertex3f( C.x, C.y, C.z);
      d[w][k][ABC] = -(B^N);          // dodatkowo wyznaczam wyraz wolny z równania plaszyzny trójk¹ta
      Norm[w][k][ABC] = N;          // dodatkowo zapisujê normaln¹ do p³aszczyzny trójk¹ta
      // trójk¹t ADB:
      Wektor3 AD = D-A;
      N = (AD*AB).znorm();          
      glNormal3f( N.x, N.y, N.z);
      glVertex3f( A.x, A.y, A.z);
      glVertex3f( D.x, D.y, D.z);
      glVertex3f( B.x, B.y, B.z);
      d[w][k][ADB] = -(B^N);       
      Norm[w][k][ADB] = N;
      // trójk¹t BDE:
      Wektor3 BD = D-B;
      Wektor3 DE = E-D;
      N = (BD*DE).znorm();          
      glNormal3f( N.x, N.y, N.z);
      glVertex3f( B.x, B.y, B.z);
      glVertex3f( D.x, D.y, D.z);     
      glVertex3f( E.x, E.y, E.z);  
      d[w][k][BDE] = -(B^N);        
      Norm[w][k][BDE] = N;  
      // trójk¹t CBE:
      Wektor3 CB = B-C;
      Wektor3 BE = E-B;
      N = (CB*BE).znorm();          
      glNormal3f( N.x, N.y, N.z);
      glVertex3f( C.x, C.y, C.z);
      glVertex3f( B.x, B.y, B.z);
      glVertex3f( E.x, E.y, E.z);      
      d[w][k][CBE] = -(B^N);        
      Norm[w][k][CBE] = N;
    }		
    glEnd();
    glEndList(); 

    /*glNewList(PowierzchniaTerenu,GL_COMPILE);
    glBegin(GL_POLYGON);
    glNormal3f( 0.0, 1.0, 0.0);
    glVertex3f( -100, 0, -300.0);
    glVertex3f( -100, 0, 100.0);
    glVertex3f( 100, 0, 100.0);
    glVertex3f( 100, 0, -300.0);
    glEnd();
    glEndList();*/

}



void Teren::Rysuj(int szczegolowosc)
{
  int sz = szczegolowosc;

  // rysowanie powierzchni terenu:
  glCallList(PowierzchniaTerenu);

  // rysowanie ró¿nych przedmiotów (monet, budynków, drzew)
  GLUquadricObj *Qcyl = gluNewQuadric();
  GLUquadricObj *Qdisk = gluNewQuadric();
  GLUquadricObj *Qsph = gluNewQuadric();
  for (long i=0;i<liczba_przedmiotow;i++)
  {
    glPushMatrix();

    glTranslatef(p[i].wPol.x,p[i].wPol.y,p[i].wPol.z);
    //glRotatef(k.w*180.0/PI,k.x,k.y,k.z);
    //glScalef(dlugosc,wysokosc,szerokosc);
    switch (p[i].typ)
    {
    case MONETA:
      //gluCylinder(Qcyl,promien1,promien2,wysokosc,10,20);
      if (p[i].do_wziecia)
      {
        GLfloat Surface[] = { 2.0f, 2.0f, 1.0f, 1.0f};
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
        glRotatef(90,1,0,0);
        p[i].srednica = powf(p[i].wartosc/100,0.4); float grubosc = 0.2*p[i].srednica;
        p[i].wPol.y = grubosc + Wysokosc(p[i].wPol.x,p[i].wPol.z);
        gluDisk(Qdisk,0,p[i].srednica,5*(1+sz),5*(1+sz));
        gluCylinder(Qcyl,p[i].srednica,p[i].srednica,grubosc,5*(1+sz),10*(1+sz)); 
        if (szczegolowosc == 1)
        {
          glRasterPos2f(0.30,1.20);
          glPrint("%d",p[i].wartosc); 
        }
      }
      break;
    case BECZKA:
      //gluCylinder(Qcyl,promien1,promien2,wysokosc,10,20);
      if (p[i].do_wziecia)
      {
        GLfloat Surface[] = { 0.50f, 0.45f, 0.0f, 1.0f};
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
        glRotatef(90,1,0,0);
        p[i].srednica = powf((float)p[i].wartosc/50,0.4); float grubosc = 2*p[i].srednica;
        p[i].wPol.y = grubosc + Wysokosc(p[i].wPol.x,p[i].wPol.z);
        gluDisk(Qdisk,0,p[i].srednica,5*(1+sz),5*(1+sz));
        gluCylinder(Qcyl,p[i].srednica,p[i].srednica,grubosc,5*(1+sz),10*(1+sz)); 
        if (szczegolowosc == 1)
        {
          glRasterPos2f(0.30,1.20);
          glPrint("%d",p[i].wartosc); 
        }
      }
      break;
    case DRZEWO:
      switch (p[i].podtyp)
      {
      case TOPOLA:  
        {     
          GLfloat Surface[] = { 0.5f, 0.5f, 0.0f, 1.0f};
          glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
          glPushMatrix();
          glRotatef(90,1,0,0);
          p[i].srednica = 0.8; float wysokosc = p[i].wartosc;
          p[i].wPol.y = wysokosc + Wysokosc(p[i].wPol.x,p[i].wPol.z);
          gluCylinder(Qcyl,p[i].srednica/2,p[i].srednica,wysokosc,5*(1+sz),10*(1+sz)); 
          glPopMatrix();
          GLfloat Surface2[] = { 0.0f, 0.9f, 0.0f, 1.0f};
          glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface2);
          //glTranslatef(0,wysokosc,0);
          glScalef(1,2,1);
          gluSphere(Qsph,3,5*(1+3*sz),5*(1+3*sz));
          break;
        }	
      case SWIERK:	
        {
          GLfloat Surface[] = { 0.65f, 0.3f, 0.0f, 1.0f};
          glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
          glPushMatrix();
          glRotatef(90,1,0,0);
          float wysokosc = p[i].wartosc;
          p[i].srednica = wysokosc/10;
          float promien = p[i].srednica/2;
          p[i].wPol.y = wysokosc + Wysokosc(p[i].wPol.x,p[i].wPol.z);
          gluCylinder(Qcyl,promien,promien*2,wysokosc,5*(1+sz),10*(1+sz)); 

          GLfloat Surface2[] = { 0.0f, 0.70f, 0.2f, 1.0f};
          glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface2);
          glTranslatef(0,0,wysokosc*2/4);
          gluCylinder(Qcyl,promien*2,promien*8,wysokosc/4,5*(1+sz),10*(1+sz)); 
          glTranslatef(0,0,-wysokosc*1/4);
          gluCylinder(Qcyl,promien*2,promien*6,wysokosc/4,5*(1+sz),10*(1+sz)); 
          glTranslatef(0,0,-wysokosc*1/3);
          gluCylinder(Qcyl,0,promien*4,wysokosc/3,5*(1+sz),10*(1+sz)); 
          glPopMatrix(); 
        }
      }	
    }

    glPopMatrix();
  }
  gluDeleteQuadric(Qcyl);gluDeleteQuadric(Qdisk);gluDeleteQuadric(Qsph);
  //glCallList(Floor);                   
}


