/****************************************************
Wirtualne zespoly robocze - przykladowy projekt w C++
Do zadañ dotycz¹cych wspó³pracy, ekstrapolacji i
autonomicznych obiektów
****************************************************/

#include <windows.h>
#include <math.h>
#include <time.h>

#include <gl\gl.h>
#include <gl\glu.h>

#include "obiekty.h"
#include "grafika.h"
#include "interakcja.h"


//deklaracja funkcji obslugi okna
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ObslugaZamien(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ObslugaNego(HWND, UINT, WPARAM, LPARAM);

#define ID_EDIT 1
#define ID_BUTTON_TAK 2
#define ID_BUTTON_NIE 3
int nrDoZam = 0;
float propozycja = -1;

void RegistryNewPanel();
void RegistryNewPanel2();
int miniPow(int, int);
int WideToInt(wchar_t[]);

HWND okno3;
HWND okno2;
HWND okno;                   // uchwyt do okna aplikacji
HDC g_context = NULL;        // uchwyt kontekstu graficznego



//funkcja Main - dla Windows
int WINAPI WinMain(HINSTANCE hInstance,
   HINSTANCE hPrevInstance,
   LPSTR     lpCmdLine,
   int       nCmdShow)
{
   MSG meldunek;		  //innymi slowy "komunikat"
   WNDCLASS nasza_klasa; //klasa g³ównego okna aplikacji


   static char nazwa_klasy[] = "Podstawowa";

   //Definiujemy klase g³ównego okna aplikacji
   //Okreslamy tu wlasciwosci okna, szczegoly wygladu oraz
   //adres funkcji przetwarzajacej komunikaty
   nasza_klasa.style = CS_HREDRAW | CS_VREDRAW;
   nasza_klasa.lpfnWndProc = WndProc; //adres funkcji realizuj¹cej przetwarzanie meldunków 
   nasza_klasa.cbClsExtra = 0;
   nasza_klasa.cbWndExtra = 0;
   nasza_klasa.hInstance = hInstance; //identyfikator procesu przekazany przez MS Windows podczas uruchamiania programu
   nasza_klasa.hIcon = 0;
   nasza_klasa.hCursor = LoadCursor(0, IDC_ARROW);
   nasza_klasa.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
   nasza_klasa.lpszMenuName = "Menu";
   nasza_klasa.lpszClassName = nazwa_klasy;

   //teraz rejestrujemy klasê okna g³ównego
   RegisterClass(&nasza_klasa);

   /*tworzymy okno g³ówne
   okno bêdzie mia³o zmienne rozmiary, listwê z tytu³em, menu systemowym
   i przyciskami do zwijania do ikony i rozwijania na ca³y ekran, po utworzeniu
   bêdzie widoczne na ekranie */
   okno = CreateWindow(nazwa_klasy, "WZR-lab 2016, temat 3, wersja a   [F1-pomoc][F2-propozycja wspolpracy]", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
      100, 50, 700, 700, NULL, NULL, hInstance, NULL);


   ShowWindow(okno, nCmdShow);

   //odswiezamy zawartosc okna
   UpdateWindow(okno);

   // G£ÓWNA PÊTLA PROGRAMU

   // pobranie komunikatu z kolejki jeœli funkcja PeekMessage zwraca wartoœæ inn¹ ni¿ FALSE,
   // w przeciwnym wypadku symulacja wirtualnego œwiata wraz z wizualizacj¹
   ZeroMemory(&meldunek, sizeof(meldunek));
   while (meldunek.message != WM_QUIT)
   {
      if (PeekMessage(&meldunek, NULL, 0U, 0U, PM_REMOVE))
      {
         TranslateMessage(&meldunek);
         DispatchMessage(&meldunek);
      }
      else
      {
         Cykl_WS(nrDoZam, propozycja);    // Cykl wirtualnego œwiata
         nrDoZam = 0;
         propozycja = -1;
         InvalidateRect(okno, NULL, FALSE);
      }
   }

   return (int)meldunek.wParam;
}

/********************************************************************
FUNKCJA OKNA realizujaca przetwarzanie meldunków kierowanych do okna aplikacji*/
LRESULT CALLBACK WndProc(HWND okno, UINT kod_meldunku, WPARAM wParam, LPARAM lParam)
{

   // PONI¯SZA INSTRUKCJA DEFINIUJE REAKCJE APLIKACJI NA POSZCZEGÓLNE MELDUNKI 
   KlawiszologiaSterowania(kod_meldunku, wParam, lParam);

   switch (kod_meldunku)
   {
   case WM_CREATE:  //meldunek wysy³any w momencie tworzenia okna
   {

                       g_context = GetDC(okno);

                       srand((unsigned)time(NULL));
                       int wynik = InicjujGrafike(g_context);
                       if (wynik == 0)
                       {
                          printf("nie udalo sie otworzyc okna graficznego\n");
                          //exit(1);
                       }

                       PoczatekInterakcji();
                       RegistryNewPanel();
                       RegistryNewPanel2();
                       SetTimer(okno, 1, 10, NULL);

                       return 0;
   }
   case WM_KEYDOWN:
   {
                     switch (LOWORD(wParam))
                     {
                     case VK_F1:  // wywolanie systemu pomocy
                     {
                                     char lan[1024], lan_bie[1024];
                                     //GetSystemDirectory(lan_sys,1024);
                                     GetCurrentDirectory(1024, lan_bie);
                                     strcpy(lan, "C:\\Program Files\\Internet Explorer\\iexplore ");
                                     strcat(lan, lan_bie);
                                     strcat(lan, "\\pomoc.htm");
                                     int wyni = WinExec(lan, SW_NORMAL);
                                     if (wyni < 32)  // proba uruchominia pomocy nie powiodla sie
                                     {
                                        strcpy(lan, "C:\\Program Files\\Mozilla Firefox\\firefox ");
                                        strcat(lan, lan_bie);
                                        strcat(lan, "\\pomoc.htm");
                                        wyni = WinExec(lan, SW_NORMAL);
                                        if (wyni < 32)
                                        {
                                           char lan_win[1024];
                                           GetWindowsDirectory(lan_win, 1024);
                                           strcat(lan_win, "\\notepad pomoc.txt ");
                                           wyni = WinExec(lan_win, SW_NORMAL);
                                        }
                                     }
                                     break;
                     }
                     case VK_ESCAPE:   // wyjœcie z programu
                     {
                                          SendMessage(okno, WM_DESTROY, 0, 0);
                                          break;
                     }
                     case VK_F2:
                     {
                                  okno2 = CreateWindowW(L"BluePanelClass", L"",
                                     WS_CHILD | WS_VISIBLE,
                                     0, 20, 250, 125, okno, NULL,
                                     NULL, 0);
                                  break;
                     }
                     case VK_F3:
                     {
                                  okno3 = CreateWindowW(L"NegocjacjeClass", L"",
                                     WS_CHILD | WS_VISIBLE,
																		 10, 40, 280, 120, okno, NULL,
																		 //0, 20, 250, 125, okno, NULL,
                                     NULL, 0);
                                  break;
                     }
                     }
                     return 0;
   }
	 case WM_INPUT:
	 {
			 okno3 = CreateWindowW(L"NegocjacjeClass", L"",
				 WS_CHILD | WS_VISIBLE,
				 10, 40, 280, 120, okno, NULL,
				 NULL, 0);
			 return 0;
	 }
   case WM_PAINT:
   {
                   PAINTSTRUCT paint;
                   HDC kontekst;
                   kontekst = BeginPaint(okno, &paint);

                   RysujScene();
                   SwapBuffers(kontekst);

                   EndPaint(okno, &paint);



                   return 0;
   }

   case WM_SIZE:
   {
                  int cx = LOWORD(lParam);
                  int cy = HIWORD(lParam);

                  ZmianaRozmiaruOkna(cx, cy);

                  return 0;
   }

   case WM_DESTROY: //obowi¹zkowa obs³uga meldunku o zamkniêciu okna
      if (lParam == 100)
         MessageBox(okno, "Zosta³eœ uznany za spamera! Lub jest zbyt póŸno na do³¹czenie do wirtualnego œwiata. Trzeba to zrobiæ zanim inni uczestnicy zmieni¹ jego stan.", "Zamkniêcie programu", MB_OK);

      ZakonczenieInterakcji();

      ZakonczenieGrafiki();


      ReleaseDC(okno, g_context);
      KillTimer(okno, 1);



      PostQuitMessage(0);
      return 0;

   default: //standardowa obs³uga pozosta³ych meldunków
      return DefWindowProc(okno, kod_meldunku, wParam, lParam);
   }


}

void RegistryNewPanel()
{
   HBRUSH hbrush = CreateSolidBrush(RGB(128, 128, 128));

   WNDCLASSW rwc = { 0 };

   rwc.lpszClassName = L"BluePanelClass";
   rwc.hbrBackground = hbrush;
   rwc.lpfnWndProc = ObslugaZamien;
   rwc.hCursor = LoadCursor(0, IDC_ARROW);
   RegisterClassW(&rwc);
}
void RegistryNewPanel2()
{
   HBRUSH hbrush = CreateSolidBrush(RGB(128, 128, 128));

   WNDCLASSW rwc = { 0 };

   rwc.lpszClassName = L"NegocjacjeClass";
   rwc.hbrBackground = hbrush;
   rwc.lpfnWndProc = ObslugaNego;
   rwc.hCursor = LoadCursor(0, IDC_ARROW);
   RegisterClassW(&rwc);
}


LRESULT CALLBACK ObslugaZamien(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   HWND button;
   static HWND edit;

   switch (msg)
   {
   case WM_CREATE:
   {
		
        edit = CreateWindowW(L"Edit", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            50, 30, 150, 20, hwnd, (HMENU)ID_EDIT,
            NULL, NULL);
        button = CreateWindowW(L"button", L"Wspolpraca",
            WS_VISIBLE | WS_CHILD,
            120, 70, 80, 25, hwnd, (HMENU)ID_BUTTON_TAK,
            NULL, NULL);
        break;
   }
   case WM_COMMAND:
      switch (LOWORD(wParam))
      {
      case ID_BUTTON_TAK:
      {
          MessageBeep(0xFFFFFFFF);
          wchar_t text[5];

          GetWindowTextW(edit, (LPWSTR)text, 5);
          nrDoZam = WideToInt(text);

          SendMessage(hwnd, WM_CLOSE, 0, 0);
          break;
      }
      default:
         break;
      }
      break;
   case WM_CLOSE:
   {
                   DestroyWindow(hwnd);
                   break;
   }
   default:
      break;
   }
   return DefWindowProcW(hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK ObslugaNego(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   HWND button, info_text, button2;
   static HWND edit;

   switch (msg)
   {
   case WM_CREATE:
   {
		 info_text = CreateWindow("STATIC", "Twoja propozycja: [%]",
					WS_VISIBLE | WS_CHILD | SS_LEFT, 
					20, 20, 240, 60, hwnd, NULL, NULL, NULL);
      edit = CreateWindowW(L"Edit", NULL,
          WS_CHILD | WS_VISIBLE | WS_BORDER,
          60, 45, 160, 20, hwnd, (HMENU)ID_EDIT,
          NULL, NULL);
      button = CreateWindowW(L"button", L"Negocjuj",
          WS_VISIBLE | WS_CHILD,
          50, 85, 80, 25, hwnd, (HMENU)ID_BUTTON_TAK,
          NULL, NULL);
			button2 = CreateWindowW(L"button", L"Zerwij",
					WS_VISIBLE | WS_CHILD,
					160, 85, 80, 25, hwnd, (HMENU)ID_BUTTON_NIE,
					NULL, NULL);
      break;
   }
   case WM_COMMAND:
      switch (LOWORD(wParam))
      {
				case ID_BUTTON_TAK:
				{
						MessageBeep(0xFFFFFFFF);
						wchar_t text[5];

						GetWindowTextW(edit, (LPWSTR)text, 5);
						propozycja = WideToInt(text);

						SendMessage(hwnd, WM_CLOSE, 0, 0);
						break;
				}
				case ID_BUTTON_NIE:
				{
						propozycja = -2;
						SendMessage(hwnd, WM_CLOSE, 0, 0);
						break;
				}
				default:
					 break;
				}
				break;
   case WM_CLOSE:
   {
                   DestroyWindow(hwnd);
                   break;
   }
   default:
      break;
   }
   return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int miniPow(int a, int b)    // power with small number - just demo
{
   int r = 1;
   for (int i = 0; i < b; i++)
   {
      r *= a;
   }
   return r;
}
int WideToInt(wchar_t a[])
{
   int i = 0;
   int len = wcslen(a);
   int coefficient = 0;
   int RawNumber = 0;
   int Number = 0;

   for (int k = 0; k < len; k++)
   {
      coefficient = miniPow(10, len - 1 - k);
      RawNumber = (int)a[k];
      Number = RawNumber - 48;
      i += coefficient * Number;
   }
   return i;
}

