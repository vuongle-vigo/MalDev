#include <winternl.h>
#include <Windows.h>
#include <ntdef.h>

typedef enum _SECTION_INHERIT
{
    ViewShare = 1, // The mapped view of the section will be mapped into any child processes created by the process.
    ViewUnmap = 2  // The mapped view of the section will not be mapped into any child processes created by the process.
} SECTION_INHERIT;

typedef NTSTATUS(NTAPI* pNtCreateSection)(
    _Out_ PHANDLE SectionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PLARGE_INTEGER MaximumSize,
    _In_ ULONG SectionPageProtection,
    _In_ ULONG AllocationAttributes,
    _In_opt_ HANDLE FileHandle
    );

typedef NTSTATUS(NTAPI* pNtMapViewOfSection)(
    _In_ HANDLE SectionHandle,
    _In_ HANDLE ProcessHandle,
    _Inout_ _At_(*BaseAddress, _Readable_bytes_(*ViewSize) _Writable_bytes_(*ViewSize) _Post_readable_byte_size_(*ViewSize)) PVOID* BaseAddress,
    _In_ ULONG_PTR ZeroBits,
    _In_ SIZE_T CommitSize,
    _Inout_opt_ PLARGE_INTEGER SectionOffset,
    _Inout_ PSIZE_T ViewSize,
    _In_ SECTION_INHERIT InheritDisposition,
    _In_ ULONG AllocationType,
    _In_ ULONG PageProtection
    );

typedef NTSTATUS(NTAPI* pNtUnmapViewOfSection)(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress
    );