#include "HookLib/HookLib.h"

#include <ntifs.h>

PCHAR PsGetProcessImageFileName(__in PEPROCESS Process);

typedef NTSTATUS(NTAPI* P_NtCreateFile)(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength);
P_NtCreateFile NtCreateFile_Orig = 0;
NTSTATUS NTAPI NtCreateFile_Hook(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength)
{
    DbgPrintEx(0, 0, "[lysm][%s] \n", __FUNCTION__);

    NTSTATUS ret = NtCreateFile_Orig(
        FileHandle,
        DesiredAccess,
        ObjectAttributes,
        IoStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        EaBuffer,
        EaLength);

    return ret;
}

typedef enum _WINDOWINFOCLASS {
    WindowProcess,
    WindowThread,
    WindowActiveWindow,
    WindowFocusWindow,
    WindowIsHung,
    WindowClientBase,
    WindowIsForegroundThread,
} WINDOWINFOCLASS;
typedef HANDLE(NTAPI* P_NtUserQueryWindow)(
    _In_ HANDLE hWnd,
    _In_ WINDOWINFOCLASS WindowInfo);
P_NtUserQueryWindow NtUserQueryWindow_Orig = 0;
HANDLE NTAPI NtUserQueryWindow_Hook(
    _In_ HANDLE hWnd,
    _In_ WINDOWINFOCLASS WindowInfo)
{
    DbgPrintEx(0, 0, "[lysm][%s] \n", __FUNCTION__);

    HANDLE ret = NtUserQueryWindow_Orig(hWnd, WindowInfo);

    return ret;
}

VOID FindProcessByName(PCHAR ProcessName, ULONG_PTR OutArray[], ULONG Size)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PEPROCESS Process = 0;
    PCHAR Name = 0;
    ULONG Index = 0;

    for (ULONG Pid = 4; Pid < 100000; Pid += 4)
    {
        if (Index >= Size)
        {
            break;
        }

        Status = PsLookupProcessByProcessId((HANDLE)Pid, &Process);
        if (!NT_SUCCESS(Status) || Process == 0)
        {
            continue;
        }

        if (PsGetProcessExitStatus(Process) != STATUS_PENDING)
        {
            continue;
        }

        Name = (PCHAR)PsGetProcessImageFileName(Process);
        if (!Name)
        {
            continue;
        }

        if (strstr(Name, ProcessName) != 0)
        {
            OutArray[Index] = (ULONG_PTR)Process;
            Index++;
        }
    }
}

BOOLEAN AttachProcess(PCHAR Name)
{
    ULONG_PTR ProcArray[1] = { 0 };
    FindProcessByName(Name, ProcArray, sizeof(ProcArray) / sizeof(ProcArray[0]));
    if (ProcArray[0])
    {
        KeAttachProcess((PEPROCESS)ProcArray[0]);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID DetachProcess()
{
    KeDetachProcess();
}

VOID UnInitHook()
{
    if (NtCreateFile_Orig)
        unhook(NtCreateFile_Orig);

    AttachProcess("explorer.exe");
    if (NtUserQueryWindow_Orig)
        unhook(NtUserQueryWindow_Orig);
    DetachProcess();
}

VOID InitHook()
{
    PVOID pNtCreateFile = (PVOID)0xfffff80646e4f7e0;
    hook(pNtCreateFile, NtCreateFile_Hook, (PVOID*)&NtCreateFile_Orig);

    AttachProcess("explorer.exe");
    PVOID pNtUserQueryWindow = (PVOID)0xffff87637f4b17b0;
    hook(pNtUserQueryWindow, NtUserQueryWindow_Hook, (PVOID*)&NtUserQueryWindow_Orig);
    DetachProcess();
}

NTSTATUS DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	DbgPrintEx(0, 0, "[lysm][%s] \n", __FUNCTION__);

	UnInitHook();

	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DbgPrintEx(0, 0, "[lysm][%s] \n", __FUNCTION__);

	DriverObject->DriverUnload = (PDRIVER_UNLOAD)DriverUnload;

	InitHook();

	return STATUS_SUCCESS;
}