#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

EFI_STATUS
EFIAPI
UefiMain (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  UINTN Columnas, Filas;
  UINT32 Seed;
  EFI_TIME Time;
  CHAR16 Texto[2] = {0, 0}; 
  EFI_INPUT_KEY Key;
  EFI_STATUS Status;

  // 1. Detectar resolución y evitar el scroll del borde
  gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &Columnas, &Filas);
  if (Columnas > 0) Columnas--; 

  // 2. Reservar memoria para columnas y retardos
  UINTN *Gotas = AllocatePool(Columnas * sizeof(UINTN));
  UINTN *Delay = AllocatePool(Columnas * sizeof(UINTN));
  if (Gotas == NULL || Delay == NULL) return EFI_OUT_OF_RESOURCES;

  // 3. Semilla inicial
  gRT->GetTime(&Time, NULL);
  Seed = Time.Nanosecond;
  
  for (UINTN i = 0; i < Columnas; i++) {
    Seed = (Seed * 1103515245 + 12345) & 0x7fffffff;
    Gotas[i] = Seed % Filas;
    Delay[i] = Seed % 5;
  }

  // 4. Preparar pantalla
  gST->ConOut->ClearScreen(gST->ConOut);
  gST->ConOut->EnableCursor(gST->ConOut, FALSE);

  while (TRUE) {
    // --- LÓGICA DE TECLADO: SOLO ESC ---
    Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
    if (!EFI_ERROR(Status)) {
      if (Key.ScanCode == SCAN_ESC) {
        gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
      }
    }

    // --- LÓGICA DE LLUVIA ---
    for (UINTN i = 0; i < Columnas; i++) {
      if (Delay[i] == 0) {
        Seed = (Seed * 1103515245 + 12345) & 0x7fffffff;
        Texto[0] = (CHAR16)((Seed % 94) + 33);

        // Cabeza: Verde claro
        gST->ConOut->SetAttribute(gST->ConOut, EFI_LIGHTGREEN);
        gST->ConOut->SetCursorPosition(gST->ConOut, i, Gotas[i]);
        gST->ConOut->OutputString(gST->ConOut, Texto);

        // Cuerpo: Verde oscuro
        UINTN Cuerpo = (Gotas[i] > 0) ? Gotas[i] - 1 : Filas - 1;
        gST->ConOut->SetAttribute(gST->ConOut, EFI_GREEN);
        gST->ConOut->SetCursorPosition(gST->ConOut, i, Cuerpo);
        gST->ConOut->OutputString(gST->ConOut, Texto);

        // Cola: Borrado
        UINTN Cola = (Gotas[i] >= 12) ? Gotas[i] - 12 : Filas - (12 - Gotas[i]);
        gST->ConOut->SetCursorPosition(gST->ConOut, i, Cola % Filas);
        gST->ConOut->OutputString(gST->ConOut, L" ");

        Gotas[i] = (Gotas[i] >= Filas - 1) ? 0 : Gotas[i] + 1;
        Delay[i] = Seed % 4;
      } else {
        Delay[i]--;
      }
    }
    gBS->Stall(8000);
  }

  return EFI_SUCCESS;
}

