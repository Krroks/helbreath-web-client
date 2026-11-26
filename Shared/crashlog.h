// Escribe crash.txt con la pila (funcion y linea) cuando el programa se cae.
#pragma once
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#pragma comment(lib, "dbghelp.lib")

static LONG WINAPI HBCrashFilter(EXCEPTION_POINTERS* ep)
{
	static volatile LONG busy = 0; if (InterlockedExchange(&busy, 1)) return EXCEPTION_CONTINUE_SEARCH;
	FILE* f = fopen("crash.txt", "a");
	if (!f) return EXCEPTION_CONTINUE_SEARCH;
	SYSTEMTIME st; GetLocalTime(&st);
	fprintf(f, "==== %02d:%02d:%02d excepcion 0x%08lX en 0x%p", st.wHour, st.wMinute, st.wSecond,
		ep->ExceptionRecord->ExceptionCode, ep->ExceptionRecord->ExceptionAddress);
	if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2)
		fprintf(f, " (%s direccion 0x%p)", ep->ExceptionRecord->ExceptionInformation[0] ? "escritura" : "lectura", (void*)ep->ExceptionRecord->ExceptionInformation[1]);
	fprintf(f, "\n");

	HANDLE p = GetCurrentProcess(), t = GetCurrentThread();
	SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
	SymInitialize(p, NULL, TRUE);
	CONTEXT c = *ep->ContextRecord;
	STACKFRAME64 sf; ZeroMemory(&sf, sizeof(sf));
	sf.AddrPC.Offset = c.Eip;    sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrFrame.Offset = c.Ebp; sf.AddrFrame.Mode = AddrModeFlat;
	sf.AddrStack.Offset = c.Esp; sf.AddrStack.Mode = AddrModeFlat;
	for (int i = 0; i < 40; i++) {
		if (!StackWalk64(IMAGE_FILE_MACHINE_I386, p, t, &sf, &c, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) break;
		if (sf.AddrPC.Offset == 0) break;
		char buf[sizeof(SYMBOL_INFO) + 256]; SYMBOL_INFO* s = (SYMBOL_INFO*)buf;
		ZeroMemory(buf, sizeof(buf)); s->SizeOfStruct = sizeof(SYMBOL_INFO); s->MaxNameLen = 255;
		DWORD64 disp = 0; DWORD ld = 0; IMAGEHLP_LINE64 ln; ZeroMemory(&ln, sizeof(ln)); ln.SizeOfStruct = sizeof(ln);
		if (SymFromAddr(p, sf.AddrPC.Offset, &disp, s)) {
			if (SymGetLineFromAddr64(p, sf.AddrPC.Offset, &ld, &ln))
				fprintf(f, "  #%d %s+0x%llx  %s:%lu\n", i, s->Name, disp, ln.FileName, ln.LineNumber);
			else fprintf(f, "  #%d %s+0x%llx\n", i, s->Name, disp);
		} else fprintf(f, "  #%d 0x%08llx\n", i, sf.AddrPC.Offset);
	}
	fclose(f);
	return EXCEPTION_CONTINUE_SEARCH;
}
static LONG WINAPI HBVeh(EXCEPTION_POINTERS* ep)
{
	DWORD c = ep->ExceptionRecord->ExceptionCode;
	if (c == 0xC0000005 || c == 0xC0000374 || c == 0xC0000409 || c == 0xC00000FD || c == 0xC0000096 || c == 0xC000001D)
		HBCrashFilter(ep);
	return EXCEPTION_CONTINUE_SEARCH;
}
inline void InstallCrashLog() { SetUnhandledExceptionFilter(HBCrashFilter); AddVectoredExceptionHandler(1, HBVeh); }
