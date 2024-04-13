#include "framework.h"
#include "resource.h"

#define	R	7				// balls' radius

#define	WIN_WIDTH	1024
#define WIN_HEIGHT	800
#define BOX_WIDTH	600
#define	BOX_HEIGHT	600
#define CELL_WIDTH	20
#define CELL_HEIGHT	20

#define BTN_START	101
#define BTN_STOP	102
#define ID_EDIT1	201			// number of atoms
#define ID_EDIT2	202			// atomic speed

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit);

HINSTANCE g_hInst;
HWND hWndMain;
HBITMAP hBit;
LPCTSTR lpszClass = TEXT("AtomModellingInBox");

double atomSpeed = 5.0;			// the speed of balls'
unsigned int delta = 30;					// time interval in ms
unsigned int atomNum = 2;				// the number of atoms to be run in the demo
RECT brt;								// box rectangle in which atoms move

bool init0 = false;

HWND hEdit1;			// number of atoms
HWND hEdit2;			// atomic speed
HWND hButton1;			// start button
HWND hButton2;			// stop button

HANDLE hThread;

DWORD WINAPI ThreadDrawBitmap(LPVOID temp);

class Atom
{
public:
	int cx, cy;			// center of a ball
	double vx, vy;			// velocity components of a ball in 2 dimension
	//size_t radius;		// atom's radius
	double speed;		// atom's speed

	Atom()
	{
		srand((unsigned)time(NULL));
		//radius = (unsigned int)R;
		speed = atomSpeed;
		int temp = (int)atomSpeed;
		//vx = (double)(rand() % 20000) / 1000.0 - 10.0;
		//vx = (double)(rand() % (2000 * temp)) / 1000.0 - (double)temp;
		//vy = sqrt(atomSpeed * atomSpeed - vx * vx);
		cx = 0;
		cy = 0;
		vx = 0.0;
		vy = 0.0;
	}
	Atom(int cx_, int cy_) : cx(cx_), cy(cy_)
	{
		srand((unsigned)time(NULL));
		//radius = (unsigned int)R;
		speed = atomSpeed;
		int temp = (int)atomSpeed;
		//vx = (double)(rand() % (2000 * temp)) / 1000.0 - (double)temp;
		//vy = sqrt(atomSpeed * atomSpeed - vx * vx);
		vx = 0.0;
		vy = 0.0;
	}
};

bool RangeBetweenAtoms(Atom& at1, Atom& at2, double criticalDistance);
void InitAtoms(std::vector<Atom>& atoms, int numAtoms);
void OnTimer(void);

std::vector<Atom> atoms;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS wdc;
	g_hInst = hInstance;

	wdc.cbClsExtra = 0;
	wdc.cbWndExtra = 0;
	wdc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wdc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wdc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wdc.hInstance = hInstance;
	wdc.lpfnWndProc = WndProc;
	wdc.lpszClassName = lpszClass;
	wdc.lpszMenuName = nullptr;
	wdc.style = CS_HREDRAW | CS_VREDRAW;	// class style

	RegisterClass(&wdc);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, WIN_WIDTH, WIN_HEIGHT, nullptr,
		(HMENU)nullptr, hInstance, nullptr);

	ShowWindow(hWnd, nCmdShow);

	while (GetMessage(&Message, nullptr, 0, 0))
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return (int)Message.wParam;
}

bool RangeBetweenAtoms(Atom& at1, Atom& at2, double criticalDistance)		// determinant of collision
{
	double range = (double)((at2.cx - at1.cx) * (at2.cx - at1.cx) + (at2.cy - at1.cy) * (at2.cy - at1.cy));
	if (range <= criticalDistance)	// collision occured
	{
		return true;
	}
	else
	{
		return false;
	}
}


void InitAtoms(std::vector<Atom>& atoms, int numAtoms)
{
	// arrange atoms
	int xNum = BOX_WIDTH / CELL_WIDTH;
	int yNum = BOX_HEIGHT / CELL_HEIGHT;
	int totalNum = xNum * yNum;

	int x, y;
	int idx;
	std::set<int> randNums;
	srand((unsigned)time(NULL));
	unsigned long seed = rand() % 32767;
	std::mt19937_64 uniformRand(seed);
	std::uniform_int_distribution<int> dist(1, totalNum-1);
	std::uniform_int_distribution<int> perturb(-2*R, 2*R);
	
	while (randNums.size() < numAtoms)
	{
		randNums.insert(dist(uniformRand));
	}
	idx = 0;
	for (auto iter = randNums.begin(); iter != randNums.end(); ++iter)
	{
		y = (*iter) / xNum;
		x = (*iter) % xNum;
		atoms[idx].cx = x * CELL_WIDTH + CELL_WIDTH / 2 + perturb(uniformRand);
		atoms[idx].cy = y * CELL_HEIGHT + CELL_HEIGHT / 2 + perturb(uniformRand);
		idx++;
	}
	
	// impose initial velocity
	std::uniform_real_distribution<double> distV(-atomSpeed, atomSpeed);

	for (idx = 0; idx < numAtoms; idx++)
	{
		atoms[idx].vx = distV(uniformRand);
		atoms[idx].vy = (double)sqrt(atomSpeed * atomSpeed - atoms[idx].vx * atoms[idx].vx);
	}
}

void OnTimer(void)
{
	HDC hdc, hMemDC;
	HBITMAP hOldBit;
	HBRUSH hBrush, hOldBrush;
	int i;
	int tempX, tempY;

	brt.left = 0;
	brt.top = 0;
	brt.right = BOX_WIDTH;
	brt.bottom = BOX_HEIGHT;

	hdc = GetDC(hWndMain);

	if (hBit == nullptr)
	{
		hBit = CreateCompatibleBitmap(hdc, brt.right - brt.left, brt.bottom - brt.top);
	}
	hMemDC = CreateCompatibleDC(hdc);
	hOldBit = (HBITMAP)SelectObject(hMemDC, hBit);

	// initialize bitmap (back buffer)
	FillRect(hMemDC, &brt, GetSysColorBrush(COLOR_WINDOW));

	Rectangle(hMemDC, brt.left, brt.top, brt.right, brt.bottom);
	// draw lattice lines in the background
	for (i = 0; i < brt.right; i += 10)
	{
		MoveToEx(hMemDC, i, 0, nullptr);
		LineTo(hMemDC, i, brt.bottom);
	}
	for (i = 0; i < brt.bottom; i += 10)
	{
		MoveToEx(hMemDC, 0, i, nullptr);
		LineTo(hMemDC, brt.right, i);
	}

	hBrush = CreateSolidBrush(RGB(64, 128, 192));
	hOldBrush = (HBRUSH)SelectObject(hMemDC, hBrush);

	for (int id0 = 0; id0 < atomNum; id0++)
	{
		if (atoms[id0].cx <= R)
		{
			atoms[id0].vx *= -1.0;
			atoms[id0].cx = R - atoms[id0].cx + R;
		}
		if (atoms[id0].cx >= brt.right - R)
		{
			atoms[id0].vx *= -1.0;
			atoms[id0].cx = 2 * brt.right - atoms[id0].cx - 2 * R;
		}
		if (atoms[id0].cy <= R)
		{
			atoms[id0].vy *= -1.0;
			atoms[id0].cy = R - atoms[id0].cy + R;
		}
		if (atoms[id0].cy >= brt.bottom - R)
		{
			atoms[id0].vy *= -1.0;
			atoms[id0].cy = 2 * brt.bottom - atoms[id0].cy - 2 * R;
		}

		for (int id1 = id0+1; id1 < atomNum; id1++)
		{
			//if (id0 == id1)
			//{
			//	continue;
			//}
			if (RangeBetweenAtoms(atoms[id0], atoms[id1], 4 * (double)(R * R)))
			{
				// exchanging two atoms' momentum
				tempX = atoms[id0].vx;
				atoms[id0].vx = atoms[id1].vx;
				atoms[id1].vx = tempX;
				tempY = atoms[id0].vy;
				atoms[id0].vy = atoms[id1].vy;
				atoms[id1].vy = tempY;

				atoms[id1].cx += (int)(atoms[id1].vx);
				atoms[id1].cy += (int)(atoms[id1].vy);
			}
		}
		atoms[id0].cx += atoms[id0].vx;
		atoms[id0].cy += atoms[id0].vy;

		Ellipse(hMemDC, atoms[id0].cx - R, atoms[id0].cy - R, atoms[id0].cx + R, atoms[id0].cy + R);
	}
	
	DeleteObject(SelectObject(hMemDC, hOldBrush));
	SelectObject(hMemDC, hOldBit);
	DeleteDC(hMemDC);

	ReleaseDC(hWndMain, hdc);
}
DWORD WINAPI ThreadDrawBitmap(LPVOID temp)
{
	LPRECT rct = (RECT*)temp;
	while (true)
	{
		OnTimer();
		InvalidateRect(hWndMain, rct, true);
		GdiFlush();
		Sleep(delta);
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	TCHAR desc[64];
	TCHAR temp[64];
	DWORD ThreadID;
	static RECT refresh;

	const static TCHAR* Mes = TEXT("2D Atomic movement in a Box demo");
	const static TCHAR* Mes2 = TEXT("One thread is allocated for drawing atoms.");

	switch (iMessage)
	{
	case WM_CREATE:
		hWndMain = hWnd;

		SetWindowText(hWnd, TEXT("simple atomic motion demo using Win32 GDI"));

		// start, stop buttons
		hButton1 = CreateWindow(TEXT("button"), TEXT("Start"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			700, 600, 100, 25, hWnd, (HMENU)BTN_START, g_hInst, nullptr);
		hButton2 = CreateWindow(TEXT("button"), TEXT("Stop"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			850, 600, 100, 25, hWnd, (HMENU)BTN_STOP, g_hInst, nullptr);

		// control edits
		hEdit1 = CreateWindow(TEXT("edit"), nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER,
			750, 250, 150, 25, hWnd, (HMENU)ID_EDIT1, g_hInst, nullptr);
		hEdit2 = CreateWindow(TEXT("edit"), nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER,
			750, 350, 150, 25, hWnd, (HMENU)ID_EDIT2, g_hInst, nullptr);
		atoms.clear();
		for (int i = 0; i < atomNum; i++)
		{
			atoms.push_back(Atom());
		}
		// initial values of edit controls
		_stprintf_s(temp, TEXT("%d"), atomNum);
		SetWindowText(hEdit1, temp);
		_stprintf_s(temp, TEXT("%f"), 5.0);
		SetWindowText(hEdit2, temp);

		//InitAtoms(atoms, atomNum);
		atomNum = 0;
		OnTimer();
		atomNum = 2;
		atomSpeed = 5.0;
		//InitAtoms(atoms, atomNum);
		refresh.left = 30;
		refresh.top = 80;
		refresh.right = 670;
		refresh.bottom = 720;
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case BTN_START:
			MessageBeep(0);
			//SetTimer(hWnd, 1, delta, nullptr);
			if (init0 == false)
			{
				hThread = CreateThread(nullptr, 0, ThreadDrawBitmap, &refresh, 0, &ThreadID);
				init0 = true;
			}
			else
			{
				ResumeThread(hThread);
			}
			break;
		
		case BTN_STOP:
			MessageBeep(0);
			//KillTimer(hWnd, 1);
			SuspendThread(hThread);
			break;

		case ID_EDIT1:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				GetWindowText(hEdit1, temp, 64);
				atomNum = wcstod(temp, nullptr);
				atoms.clear();
				for (int i = 0; i < atomNum; i++)
				{
					atoms.push_back(Atom());
				}
				InitAtoms(atoms, atomNum);
				break;
			}
		case ID_EDIT2:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				GetWindowText(hEdit2, temp, 64);
				atomSpeed = wcstod(temp, nullptr);
				atoms.clear();
				for (int i = 0; i < atomNum; i++)
				{
					atoms.push_back(Atom());
				}
				InitAtoms(atoms, atomNum);
				break;
			}
		}
		return 0;

	//case WM_TIMER:
	//	OnTimer();
	//	//InvalidateRect(hWnd, nullptr, true);
	//	InvalidateRect(hWnd, &refresh, true);
	//	return 0;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		TextOut(hdc, 230, 40, Mes, lstrlen(Mes));
		TextOut(hdc, 210, 65, Mes2, lstrlen(Mes2));
		LoadString(g_hInst, IDS_STRING1, desc, 64);
		TextOut(hdc, 750, 225, desc, lstrlen(desc));
		LoadString(g_hInst, IDS_STRING2, desc, 64);
		TextOut(hdc, 750, 325, desc, lstrlen(desc));

		if (hBit)
		{
			DrawBitmap(hdc, 50, 100, hBit);
		}
		EndPaint(hWnd, &ps);
		return 0;

	case WM_DESTROY:
		if (hBit)
		{
			DeleteObject(hBit);
		}
		KillTimer(hWnd, 1);
		CloseHandle(hThread);
		PostQuitMessage(0);
		return 0;
	}
	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}


void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit)
{
	HDC MemDC;
	HBITMAP OldBitmap;
	int bx, by;
	BITMAP bit;

	MemDC = CreateCompatibleDC(hdc);
	OldBitmap = (HBITMAP)SelectObject(MemDC, hBit);

	GetObject(hBit, sizeof(BITMAP), &bit);
	bx = bit.bmWidth;
	by = bit.bmHeight;

	BitBlt(hdc, x, y, bx, by, MemDC, 0, 0, SRCCOPY);

	SelectObject(MemDC, OldBitmap);
	DeleteDC(MemDC);
}
