/*********************************************************************************
    Operacje na wektorach 3- wymiarowych + kilka przydatnych funkcji graficznych
**********************************************************************************/
#include <math.h>
#ifndef _WEKTOR__H
  #include "wektor.h"
#endif

float pii = 3.14159265358979;

Wektor3::Wektor3(float x,float y,float z)
{
	this->x=x;
	this->y=y;
	this->z=z;
}

Wektor3::Wektor3()
{
	x=0;
	y=0;
	z=0;
}

Wektor3 Wektor3::operator* (float stala)  // mnozenie przez wartosc
{
  return Wektor3(x*stala,y*stala,z*stala);
}


Wektor3 Wektor3::operator/ (float stala)			// operator* is used to scale a Vector3D by a value. This value multiplies the Vector3D's x, y and z.
{
  if (stala!=0)
    return Wektor3(x/stala,y/stala,z/stala);
  else return Wektor3(x,y,z);
}

Wektor3 Wektor3::operator+ (float stala)  // dodanie vec+vec
{
  return Wektor3(x+stala,y+stala,z+stala);
};


Wektor3 Wektor3::operator+=(float stala)  // dodanie vec+vec
{
  x += stala;
  y += stala;
  z += stala;
  return *this;
};

Wektor3 Wektor3:: operator+= (Wektor3 ww)			// operator+= is used to add another Vector3D to this Vector3D.
{
  x += ww.x;
  y += ww.y;
  z += ww.z;
  return *this;
}



Wektor3 Wektor3::operator+ (Wektor3 ww)  // dodanie vec+vec
{
  return Wektor3(x+ww.x,y+ww.y,z+ww.z);
};


Wektor3 Wektor3::operator-=(Wektor3 ww)  // dodanie vec+vec
{
  x -= ww.x;
  y -= ww.y;
  z -= ww.z;
  return *this;
};

Wektor3 Wektor3::operator- (Wektor3 ww)  // dodanie vec+vec
{
  return Wektor3(x-ww.x,y-ww.y,z-ww.z);
};

Wektor3 Wektor3::operator- ()             // znak (-)
{
  return Wektor3(-x,-y,-z);
};

Wektor3 Wektor3::operator*(Wektor3 ww)  // iloczyn wektorowy
{
   Wektor3 w;
   w.x=y*ww.z-z*ww.y;
   w.y=z*ww.x-x*ww.z;
   w.z=x*ww.y-y*ww.x;
   return w;
}

bool Wektor3::operator==(Wektor3 v2)
{
  return (x==v2.x)&&(y==v2.y)&&(z==v2.z);
}

Wektor3 Wektor3::obrot(float alfa,float vn0,float vn1,float vn2)
{
  float s = sin(alfa), c = cos(alfa);

  Wektor3 w;
  w.x = x*(vn0*vn0+c*(1-vn0*vn0)) + y*(vn0*vn1*(1-c)-s*vn2) + z*(vn0*vn2*(1-c)+s*vn1);
  w.y = x*(vn1*vn0*(1-c)+s*vn2) + y*(vn1*vn1+c*(1-vn1*vn1)) + z*(vn1*vn2*(1-c)-s*vn0);
  w.z = x*(vn2*vn0*(1-c)-s*vn1) + y*(vn2*vn1*(1-c)+s*vn0) + z*(vn2*vn2+c*(1-vn2*vn2));

  return w;
}

Wektor3 Wektor3::znorm()
{
  float d = dlugosc();      
  if (d > 0)
    return Wektor3(x/d,y/d,z/d);       
  else return Wektor3(0,0,0);
}         

Wektor3 Wektor3::znorm2D()
{
  float d = sqrt(x*x+y*y);  
  if (d > 0)      
    return Wektor3(x/d,y/d,0); 
  else return Wektor3(0,0,0);        
}   

float Wektor3::operator^(Wektor3 w) // iloczyn skalarny
{
	return w.x*x+w.y*y+w.z*z;
}

float Wektor3::dlugosc() // dlugosc aktualnego wektora
{
	return sqrt(x*x+y*y+z*z);
}



// ****************************
//       DODATKI
//*****************************


// Funkcja obliczajaca normalna do plaszczyzny wyznaczanej przez trojkat ABC
// zgodnie z ruchem wskazowek zegara:
Wektor3 normalna(Wektor3 A,Wektor3 B, Wektor3 C)
{
  Wektor3 w1 = B-A, w2 = C-A;
  return (w1*w2).znorm();
}



// zwraca kat pomiedzy wektorami w zakresie <0,2pi) w kierunku przeciwnym
// do ruchu wskazowek zegara. Zakladam, ze Wa.z = Wb.z = 0
float kat_pom_wekt2D(Wektor3 Wa, Wektor3 Wb)  
{
    
  Wektor3 ilo = Wa.znorm2D() * Wb.znorm2D();  // iloczyn wektorowy wektorow o jednostkowej dlugosci
  float sin_kata = ilo.z;        // problem w tym, ze sin(alfa) == sin(pi-alfa)   
  if (sin_kata == 0)
  {
    if (Wa.znorm2D() == Wb.znorm2D()) return 0; 
    if (Wa.znorm2D() == -Wb.znorm2D()) return pii; 
  }  
  // dlatego trzeba  jeszcze sprawdzic czy to kat rozwarty
  Wektor3 wso_n = Wa.znorm2D() + Wb.znorm2D();          
  Wektor3 ilo1 = wso_n.znorm2D() * Wb.znorm2D();
  bool kat_rozwarty = (ilo1.dlugosc() > sqrt(2.0)/2);
  
  float kat = asin(fabs(sin_kata));
  if (kat_rozwarty) kat = pii-kat;
  if (sin_kata < 0) kat = 2*pii - kat;
  
  return kat;
}      

/*
   wyznaczanie punktu przeciecia sie 2 odcinkow AB i CD lub ich przedluzen
   zwraca 1 jesli odcinki sie przecinaja 
*/
bool punkt_przeciecia2D(float *x,float *y,float xA,float yA, float xB, float yB,
                        float xC,float yC, float xD, float yD)
{
  float a1,b1,c1,a2,b2,c2;  // rownanie prostej 2D: ax+by+c=0 
  a1 = (yB-yA); b1 = -(xB-xA); c1 = (xB-xA)*yA - (yB-yA)*xA;               
  a2 = (yD-yC); b2 = -(xD-xC); c2 = (xD-xC)*yC - (yD-yC)*xC;
  float mian = a1*b2-a2*b1;
  if (mian == 0) {*x=0;*y=0;return 0;}  // odcinki rownolegle
  float xx = (c2*b1 - c1*b2)/mian,
        yy = (c1*a2 - c2*a1)/mian;
  *x = xx; *y = yy;
  if (  (((xx > xA)&&(xx < xB))||((xx < xA)&&(xx > xB))||
         ((yy > yA)&&(yy < yB))||((yy < yA)&&(yy > yB))) &&
        (((xx > xC)&&(xx < xD))||((xx < xC)&&(xx > xD))||
         ((yy > yC)&&(yy < yD))||((yy < yC)&&(yy > yD)))
     )    
     return 1;
  else
     return 0;      
}                        

/*
    Minimalna odleglosc pomiedzy odcinkami AB i CD obliczana jako odleglosc 2 plaszczyzn
    wzajemnie rownoleglych , z ktorych kazda zawiera jeden z odcinkow (jesli rzuty odcinkow
    sie przecinaja) lub minimalna odleglosc ktoregos z punktow A,B,C lub D do drugiego odcinka 
    
    dodatkowo na wejsciu punkty na prostych AB i CD ktore mozna polaczyc 
    odcinkiem o minimalnej dlugosci oraz info czy rzuty odcinkow sie przecinaja
    (wowczas oba punkty Xab oraz Xcd leza na odcinkach AB i CD)
*/
float odleglosc_pom_odcinkami(Wektor3 A, Wektor3 B, Wektor3 C, Wektor3 D, 
             Wektor3 *Xab, Wektor3 *Xcd, bool *czy_przeciecie)
{
  Wektor3 AB = A-B, CD = C-D;
  Wektor3 N = (AB*CD).znorm();             // wektor normalny do obu plaszczyzn rownoleglych
  if (N.dlugosc() == 0)  // odcinki rownolegle -> trzeba znalezc odleglosc pomiedzy prostymi
  {
      //Wektor3 AC = A-C;
      //Wektor3 W = AC*AB;
      Wektor3 N2 = AB.znorm();             // normalna do plaszczyzny prostopadlej do odcinkow
      float d = -A^N;                      // wspolczynnik d w rownaniu plaszczyzny ax+by+cz+d=0
                                           // przechodzacej przez punkt A 
                                           
      float kC = (C^N2 + d)/(N2^N2);       // odleglosc pomiedzy C a C'
      Wektor3 Cprim = C - N2*kC;           // rzut punktu C na plaszczyzne przechodzaca przez A                         
      float k = (A-Cprim).dlugosc();       // odleglosc pomiedzy prostymi
                               
      // trzeba jeszcze sprawdzic czy odcinki sa na tej samej wysokosci, jesli nie, to odlegloscia
      // pomiedzy odcinkami jest minimalna odleglosc pomiedzy parami punktow
      float kD = (D^N2 + d)/(N2^N2),
            kB = (B^N2 + d)/(N2^N2),
            kA = 0;
      
      if ( ((kA > kC)&&(kA > kD)&&(kB > kC)&&(kB > kD))||  // odcinki nie sa na tej samej wysokosci
           ((kA < kC)&&(kA < kD)&&(kB < kC)&&(kB < kD)) )
      {
           float d[] = {(A-C).dlugosc(), (A-D).dlugosc(), (B-C).dlugosc(), (B-D).dlugosc()};
           float d_min = 1e10;
           for (long i=0;i<4;i++) 
             if (d[i] < d_min) d_min = d[i];  
           k = d_min;  
      }     
      *Xab = A;
      *Xcd = Cprim; 
      *czy_przeciecie = 0;       
                                            
      return k;
  } // jesli odcinki rownolegle
  
  float d1 = -A^N, d2 = -C^N;  // wspolczynniki d w rownaniu plaszczyzny ax+by+cz+d=0
  float k = (d1-d2)/(N^N);     // |k| - odleglosc plaszczyzn -> minimalna odleglosc pomiedzy prostymi
  Wektor3 Ap = N*k + A, Bp = N*k + B; // rzuty punktow A i B na plaszczyzne prostej CD
  float xx,yy;                        // wspolrzedne punktu przeciecia X na plaszczyznie rzutowania
  float propAX_AB,propCX_CD;          // proporcje |AX|/|AB| i |CX|/|CD| 
  
  // rzutuje wszystkie punkty na jedna z 3 plaszczyzn globalnego ukl. wspolrzednych
  // tak, aby zaden z odcinkow nie zamienil sie w punkt (wtedy, gdy obie skladowe rzutu wektora ==0)
  if (  ( ((AB.x == 0)&&(AB.y == 0))||((CD.x == 0)&&(CD.y == 0)) )||
        ( ((AB.x == 0)&&(AB.z == 0))||((CD.x == 0)&&(CD.z == 0)) )  ) // rzutowanie na x==0 
  {
       *czy_przeciecie = punkt_przeciecia2D(&xx,&yy,Ap.y,Ap.z,Bp.y,Bp.z,C.y,C.z,D.y,D.z);

       (*Xcd).y = xx;
       (*Xcd).z = yy;
       if (CD.y != 0)     
           propCX_CD = ((*Xcd).y - C.y)/CD.y;         
       else
           propCX_CD = ((*Xcd).z - C.z)/CD.z;  
       (*Xcd).x = C.x + propCX_CD*CD.x;
        
       (*Xab).y = xx;
       (*Xab).z = yy;
       if (AB.y != 0)     
           propAX_AB = ((*Xab).y - Ap.y)/AB.y;
       else
           propAX_AB = ((*Xab).z - Ap.z)/AB.z;
       (*Xab).x = Ap.x + propAX_AB*AB.x;  
       *Xab = *Xab - N*k;    // rzutowania powrotne na plaszczyzne odcinka AB
  }
  else  // rzutowanie na z==0 (choc moglo by byc rowniez na y==0) 
  {
       *czy_przeciecie = punkt_przeciecia2D(&xx,&yy,Ap.x,Ap.y,Bp.x,Bp.y,C.x,C.y,D.x,D.y);

       (*Xcd).x = xx;
       (*Xcd).y = yy;
       if (CD.y != 0)     
           propCX_CD = ((*Xcd).y - C.y)/CD.y;         
       else
           propCX_CD = ((*Xcd).x - C.x)/CD.x;  
       (*Xcd).z = C.z + propCX_CD*CD.z;
        
       (*Xab).x = xx;
       (*Xab).y = yy;
       if (AB.y != 0)     
           propAX_AB = ((*Xab).y - Ap.y)/AB.y;
       else
           propAX_AB = ((*Xab).x - Ap.x)/AB.x;
       (*Xab).z = Ap.z + propAX_AB*AB.z;  
       *Xab = *Xab - N*k;    // rzutowania powrotne na plaszczyzne odcinka AB
  }
  
  float odl_min = fabs(k);
  
  if (*czy_przeciecie == 0)  // jesli rzuty sie nie przecinaja, to musze sprawdzic, czy 
  {         // jeden z rz. odcinkow przecina sie z prosta, na ktorej lezy drugi
       odl_min = 1e10;
       if ((fabs(propAX_AB) > 1)&& (fabs(propCX_CD) > 1))  // jesli nie, to biore minimalna
       {                                // odleglosc pomiedzy koncami odcinkow
           float d[] = {(A-C).dlugosc(), (A-D).dlugosc(), (B-C).dlugosc(), (B-D).dlugosc()};
           for (long i=0;i<4;i++) 
             if (d[i] < odl_min) odl_min = d[i];                        
       }    
       else                           
         if (fabs(propAX_AB) > 1)    // punkt X poza odcinkiem AB
         {
           float d[] = {(A-*Xcd).dlugosc(), (B-*Xcd).dlugosc()};
           for (long i=0;i<2;i++) 
             if (d[i] < odl_min) odl_min = d[i];                        
         }
         else                        // punkt X musi byc poza odc. CD
         {
           float d[] = {(C-*Xab).dlugosc(), (D-*Xab).dlugosc()};
           for (long i=0;i<2;i++) 
             if (d[i] < odl_min) odl_min = d[i];      
         }
  }   
  
  return odl_min;  
}
