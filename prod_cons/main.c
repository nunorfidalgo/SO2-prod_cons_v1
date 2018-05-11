#define _CRT_RAND_S
//
//#include <Windows.h>
//#include <tchar.h>
//#include <math.h>
//
//#include <stdio.h>
//#include <io.h>
//
//#include <stdlib.h>
//
//#include <fcntl.h>


#include <windows.h>
#include <tchar.h>
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>


void assertHandleIsNot(HANDLE qual, HANDLE valor, TCHAR *msg) {
	if (qual == valor) {
		_tprintf(TEXT("\nAssert: %s\n"), msg);
		exit(1);
	}
}

void gotoxy(short x, short y) {
	static HANDLE hStdout = NULL;
	COORD position = { x, y };
	if (hStdout == NULL) {
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		assertHandleIsNot(hStdout, NULL, TEXT("GetStdHandle falhou"));
	}
	SetConsoleCursorPosition(hStdout, position);
}

#define NUMPROD 2
#define NUMCONS 1
#define MAXITENS 20

int itens[MAXITENS];
int NextIn, NextOut;

HANDLE MutProd, MutCons;
HANDLE SemItens, SemVazios;
HANDLE MutPrint;

int continua;

void inicializa() {
	int i;
	continua = 1;
	NextIn = 0;
	NextOut = 0;
	for (i = 0; i < MAXITENS; i++)
		itens[i] = 0;

	// Security Desc, Owner inicial (S/N), Nome
	MutProd = CreateMutex(NULL, FALSE, NULL);
	assertHandleIsNot(MutProd, NULL, TEXT("MutProd falhou..."));
	MutCons = CreateMutex(NULL, FALSE, NULL);
	assertHandleIsNot(MutCons, NULL, TEXT("MutCons falhou..."));

	// Security Desc, Cont inicial (disp), cont max, Nome
	// MAXTIENS itens nenhum diponivel inicialmente
	SemItens = CreateSemaphore(NULL, 0, MAXITENS, NULL);
	assertHandleIsNot(SemItens, NULL, TEXT("SemItens falhou..."));
	// MAXTIENS posicoes, todos disponiveis inicialmente
	SemVazios = CreateSemaphore(NULL, MAXITENS, MAXITENS, NULL);
	assertHandleIsNot(SemVazios, NULL, TEXT("SemVazios falhou..."));

	// Mutex completemante assessorio, para UI apenas
	MutPrint = CreateMutex(NULL, FALSE, NULL);
	assertHandleIsNot(MutPrint, NULL, TEXT("MutPrint falhou..."));
}

void printItens() {
	int i;
	_tprintf(TEXT("\n "));
	// imprime uma "regua"
	for (i = 0; i < MAXITENS; i++)
		if (i < 10)
			_tprintf(TEXT("  %d "), i);
		else
			_tprintf(TEXT(" %d "), i);

	//imprime os itens
	_tprintf(TEXT("\n |"));
	// imprime uma "regua"
	for (i = 0; i < MAXITENS; i++)
		if (itens[i] < 10)
			_tprintf(TEXT(" %d |"), itens[i]);
		else
			_tprintf(TEXT(" %d|"), itens[i]);

	//imprime os indices: marcas in e out
	_tprintf(TEXT("\n "));
	// imprime uma "regua"
	for (i = 0; i < MAXITENS; i++)
		if ((NextIn == i) && (NextOut == i))
			_tprintf(TEXT("io "));
		else if (NextIn == i)
			_tprintf(TEXT("i  "));
		else if (NextOut == i)
			_tprintf(TEXT("o  "));
		else
			_tprintf(TEXT("   "));
}

void produzItem(int quem) {
	long previous;
	unsigned int r;
	int valor;
	// aguarda pos vazia
	WaitForSingleObject(SemVazios, INFINITE);
	// aguarda acesso seccao critica
	WaitForSingleObject(MutProd, INFINITE);

	rand_s(&r);
	valor = r % 90 + 10; // 10 - 99 ( dois digitos p/ UI)
	itens[NextIn] = valor;
	NextIn = (NextIn + 1) % MAXITENS;

	// pessimo - só para efeitos de visualizacao
	WaitForSingleObject(MutPrint, INFINITE);
	// seccao critica com I/O -> pessimo, pessimo, pessimo
	gotoxy(0, 0);
	_tprintf(TEXT("\n%d produziu %d\t\t\t"), quem, valor);
	printItens(); // so para efeitos de teste
	ReleaseMutex(MutPrint);
	// fim de seccao critica pessima
	ReleaseMutex(MutProd);
	// assinala item novo
	ReleaseSemaphore(SemItens, 1, &previous);
}

void consumeItem(int quem) {
	long previous;
	int valor;
	// aguarda item
	WaitForSingleObject(SemItens, INFINITE);
	// aguarda acesso seccao critica
	WaitForSingleObject(MutCons, INFINITE);
	valor = itens[NextOut];
	itens[NextOut] = 0;
	NextOut = (NextOut + 1) % MAXITENS;

	// pessimo - só para efeitos de visualizacao
	WaitForSingleObject(MutPrint, INFINITE);
	// seccao critica com I/O -> pessimo, pessimo, pessimo
	gotoxy(0, 0);
	_tprintf(TEXT("\n%d consumiu %d\t\t\t"), quem, valor);
	printItens(); // so para efeitos de teste
	ReleaseMutex(MutPrint);
	// fim de seccao critica pessima
	ReleaseMutex(MutCons);
	// assinala nova posicao vazia
	ReleaseSemaphore(SemVazios, 1, &previous);
}

DWORD __stdcall produtor(LPVOID dadosptr) {
	unsigned int pausa;
	int whoami = (int)dadosptr;

	// tprintf(TEXT("\nprodutor %d inicio"), whoami);
	while (continua == 1) {
		rand_s(&pausa);
		Sleep((pausa % 4) * 1000);
		if (!continua) break;
		produzItem(whoami);
	}
	_tprintf(TEXT("\nprodutor %d a encerrar"), whoami);
	return 0;
}

DWORD __stdcall consumidor(LPVOID dadosptr) {
	unsigned int pausa;
	int whoami = (int)dadosptr;

	// tprintf(TEXT("\nconsumidor %d inicio"), whoami);
	while (continua == 1) {
		rand_s(&pausa);
		Sleep((pausa % 4) * 1000);
		if (!continua) break;
		consumeItem(whoami);
	}
	_tprintf(TEXT("\nconsumidor %d a encerrar"), whoami);
	return 0;
}

int _tmain(int argc, TCHAR * argv[]) {
	HANDLE produtores[NUMPROD];
	HANDLE consumidores[NUMCONS];
	int i;
	DWORD id;

	srand((unsigned)time(NULL));

	// coloca consola com suporte para caracteres acentuados
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	system("cls");

	inicializa();
	gotoxy(0, 0);
	_tprintf(TEXT("\nestado inicial - carregue numa tecla..."));
	_gettch();

	printItens();
	for (i = 0; i < NUMPROD; i++) {
		produtores[i] = CreateThread(NULL, 0, produtor, (LPVOID)i, 0, &id);
	}

	for (i = 0; i < NUMCONS; i++) {
		consumidores[i] = CreateThread(NULL, 0, consumidor, (LPVOID)i, 0, &id);
	}

	_gettch();
	continua = 0;

	// comentarios: ver no PDF

	ReleaseSemaphore(SemItens, MAXITENS, NULL); // exercicio: fazer melhor...
	ReleaseSemaphore(SemVazios, MAXITENS, NULL); // exercicio: fazer melhor...
	WaitForMultipleObjects(NUMCONS, consumidores, TRUE, INFINITE);
	WaitForMultipleObjects(NUMPROD, produtores, TRUE, INFINITE);
	_gettch();
	_tprintf(TEXT("\n"));
}