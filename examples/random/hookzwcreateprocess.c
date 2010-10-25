/* 

hookzwcreateprocess.C 

Author: <X-STAR/heartdbg> 
Last Updated: 2007-11-17 

This framework is generated by EasySYS 0.3.0 
This template file is copying from QuickSYS 0.3.0 written by Chunhua Liu 

*/ 
// *************************************************************** 
// hookzwcreateprocess version: 1.0 ? date: 11/17/2007 
// ------------------------------------------------------------- 
// Author:X-STAR/heartdbg 
// E-MAIL:qqshow@live.com 
// BLOG :http://hi.baidu.com/heartdbg 
// ------------------------------------------------------------- 
// Copyright (C) 2007 - All Rights Reserved 
// *************************************************************** 
// 
// *************************************************************** 
#include "dbghelp.h" 
#include "hookzwcreateprocess.h" 
#include <stdio.h> 
#include <stdarg.h> 
#include <ntimage.h> 
#include <ntiologc.h> 

// 
// A structure representing the instance information associated with 
// a particular device 
// 

#define DWORD unsigned long 
#define WORD unsigned short 
#define BOOL unsigned long 
#define BYTE unsigned char 
#define MAXPATHLEN 256 
#define SEC_IMAGE 0x01000000 
int position; 
int pos; 
int      po; 
KEVENT event ; 
char *output; 

extern NTSTATUS 
ZwCreateSection( 
                    OUT PHANDLE SectionHandle, 
                    IN ACCESS_MASK DesiredAccess, 
                    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, 
                    IN PLARGE_INTEGER MaximumSize OPTIONAL, 
                    IN ULONG SectionPageProtection, 
                    IN ULONG AllocationAttributes, 
                    IN HANDLE FileHandle OPTIONAL 
); 



NTSTATUS 
ObQueryNameString( 
                     IN PVOID Object, 
                     OUT POBJECT_NAME_INFORMATION ObjectNameInfo, 
                     IN ULONG Length, 
                     OUT PULONG ReturnLength 
); 



NTSTATUS 
DevCreateClose( 
                          IN PDEVICE_OBJECT DeviceObject, 
                          IN PIRP Irp 
); 

NTSTATUS 
DevDispatch( 
                          IN PDEVICE_OBJECT DeviceObject, 
                          IN PIRP Irp 
); 

VOID RegMoniterOn(); 
VOID RegMoniterOff(); 
VOID ProcMoniterOn(); 
VOID ProcMoniterOff(); 
VOID ModMonitorOn(); 
VOID ModMonitorOff(); 

BOOLEAN bRegMon = FALSE; 
BOOLEAN bProcMon= FALSE; 
BOOLEAN bModMon = FALSE; 

typedef struct ServiceDescriptorEntry { 
unsigned int *ServiceTableBase; 
unsigned int *ServiceCounterTableBase; //Used only in checked build 
unsigned int NumberOfServices; 
unsigned char *ParamTableBase; 
} ServiceDescriptorTableEntry, *PServiceDescriptorTableEntry; 

extern PServiceDescriptorTableEntry KeServiceDescriptorTable; 

/* 
NTSYSAPI 
NTSTATUS 
NTAPI 
ZwCreateProcess( 
                    OUT PHANDLE ProcessHandle, 
                    IN ACCESS_MASK DesiredAccess, 
                    IN POBJECT_ATTRIBUTES ObjectAttributes, 
                    IN HANDLE InheritFromProcessHandle, 
                    IN BOOLEAN InheritHandles, 
                    IN HANDLE SectionHandle OPTIONAL, 
                    IN HANDLE DebugPort OPTIONAL, 
                    IN HANDLE ExceptionPort OPTIONAL, 
                    IN HANDLE Unknown 
);*/ 
/* 
NTSTATUS 
ZwLoadDriver( 
                IN PUNICODE_STRING DriverServiceName 
);*/ 



typedef NTSTATUS (*ZWCREATEPROCESS)( 
                                         OUT PHANDLE ProcessHandle, 
                                         IN ACCESS_MASK DesiredAccess, 
                                         IN POBJECT_ATTRIBUTES ObjectAttributes, 
                                         IN HANDLE InheritFromProcessHandle, 
                                         IN BOOLEAN InheritHandles, 
                                         IN HANDLE SectionHandle OPTIONAL, 
                                         IN HANDLE DebugPort OPTIONAL, 
                                         IN HANDLE ExceptionPort OPTIONAL, 
                                         IN HANDLE Unknown 
); 

NTSTATUS FakedZwCreateProcess( 
                                    OUT PHANDLE ProcessHandle, 
                                    IN ACCESS_MASK DesiredAccess, 
                                    IN POBJECT_ATTRIBUTES ObjectAttributes, 
                                    IN HANDLE InheritFromProcessHandle, 
                                    IN BOOLEAN InheritHandles, 
                                    IN HANDLE SectionHandle OPTIONAL, 
                                    IN HANDLE DebugPort OPTIONAL, 
                                    IN HANDLE ExceptionPort OPTIONAL, 
                                    IN HANDLE Unknown 
                                    ); 



typedef NTSTATUS (*ZWSETVALUEKEY) 
( 
                                    IN HANDLE KeyHandle, 
                                    IN PUNICODE_STRING ValueName, 
                                    IN ULONG TitleIndex OPTIONAL, 
                                    IN ULONG Type, 
                                    IN PVOID Data, 
                                    IN ULONG DataSize 
); 

NTSTATUS FakedZwSetValueKey 
( 
                                    IN HANDLE KeyHandle, 
                                    IN PUNICODE_STRING ValueName, 
                                    IN ULONG TitleIndex OPTIONAL, 
                                    IN ULONG Type, 
                                    IN PVOID Data, 
                                    IN ULONG DataSize 
); 

typedef NTSTATUS (*ZWLOADDRIVER) 
( 
                                IN PUNICODE_STRING DriverServiceName 
); 

NTSTATUS FakedZwLoadDriver 
(                               
                                    IN PUNICODE_STRING DriverServiceName 
); 

ZWSETVALUEKEY RealZwSetValueKey; 
ZWCREATEPROCESS RealZwCreateProcess; 
ZWLOADDRIVER RealZwLoadDriver; 


typedef struct _SECTION_IMAGE_INFORMATION { 
PVOID EntryPoint; 
ULONG StackZeroBits; 
ULONG StackReserved; 
ULONG StackCommit; 
ULONG ImageSubsystem; 
WORD SubsystemVersionLow; 
WORD SubsystemVersionHigh; 
ULONG Unknown1; 
ULONG ImageCharacteristics; 
ULONG ImageMachineType; 
ULONG Unknown2[3]; 
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION; 

// Length of process name (rounded up to next DWORD) 
#define PROCNAMELEN 20 
// Maximum length of NT process name 
#define NT_PROCNAMELEN 16 
ULONG gProcessNameOffset; 
void GetProcessNameOffset() 
{ 

PEPROCESS curproc; 
int i; 
curproc = PsGetCurrentProcess(); 
for( i = 0; i < 3*PAGE_SIZE; i++ ) 
{ 
if( !strncmp( "System", (PCHAR) curproc + i, strlen("System") )) 
{ 
gProcessNameOffset = i; 
} 
} 
} 


BOOLEAN GetFullName2(HANDLE handle,char * pch) 
{ 
      
     ULONG uactLength; 
     POBJECT_NAME_INFORMATION pustr; 
     ANSI_STRING astr; 
     PVOID pObj; 
     NTSTATUS ns; 
     ns = ObReferenceObjectByHandle( handle, 0, NULL, KernelMode, &pObj, NULL ); 
     if (!NT_SUCCESS(ns)) 
     { 
          return FALSE; 
     } 
     pustr = ExAllocatePool(NonPagedPool,1024+4); 
      
     if (pObj==NULL||pch==NULL) 
          return FALSE; 
      
     ns = ObQueryNameString(pObj,pustr,512,&uactLength); 
      
     if (NT_SUCCESS(ns)) 
     { 
          RtlUnicodeStringToAnsiString(&astr,(PUNICODE_STRING)pustr,TRUE); 
          strcpy(pch,astr.Buffer); 
     } 
     ExFreePool(pustr); 
     RtlFreeAnsiString( &astr ); 
     if (pObj) 
     { 
          ObDereferenceObject(pObj); 
     } 
      
     return TRUE; 
} 

/* 
KeyHandle：hSection 

*/ 

NTSTATUS GetFullName(HANDLE KeyHandle,char *fullname) 
{ 
     NTSTATUS ns; 
     PVOID pKey=NULL,pFile=NULL; 
     UNICODE_STRING fullUniName; 
     ANSI_STRING akeyname; 
     ULONG actualLen; 
     UNICODE_STRING dosName; 

     fullUniName.Buffer=NULL; 
     fullUniName.Length=0; 
     fullname[0]=0x00; 
     ns= ObReferenceObjectByHandle( KeyHandle, 0, NULL, KernelMode, &pKey, NULL ) ; 
     if( !NT_SUCCESS(ns)) return ns; 
      
     fullUniName.Buffer = ExAllocatePool( PagedPool, MAXPATHLEN*2);//1024*2 
     fullUniName.MaximumLength = MAXPATHLEN*2; 

     __try 
     { 
           
          pFile=(PVOID)*(ULONG *)((char *)pKey+20); 
          pFile=(PVOID)*(ULONG *)((char *)pFile); 
          pFile=(PVOID)*(ULONG *)((char *)pFile+36); 
           
           
          ObReferenceObjectByPointer(pFile, 0, NULL, KernelMode); 
          RtlVolumeDeviceToDosName(((PFILE_OBJECT)pFile)->DeviceObject,&dosName); 
          //ns=ObQueryNameString( pFile, fullUniName, MAXPATHLEN, &actualLen ); 
          RtlCopyUnicodeString(&fullUniName, &dosName); 
          RtlAppendUnicodeStringToString(&fullUniName,&((PFILE_OBJECT)pFile)->FileName); 
           
          ObDereferenceObject(pFile); 
          ObDereferenceObject(pKey ); 
           
          RtlUnicodeStringToAnsiString( &akeyname, &fullUniName, TRUE ); 
          if(akeyname.Length<MAXPATHLEN) 
          { 
               memcpy(fullname,akeyname.Buffer,akeyname.Length); 
               fullname[akeyname.Length]=0x00; 
          } 
          else 
          { 
               memcpy(fullname,akeyname.Buffer,MAXPATHLEN); 
               fullname[MAXPATHLEN-1]=0x00; 
          } 
           
          RtlFreeAnsiString( &akeyname ); 
          ExFreePool(dosName.Buffer); 
          ExFreePool( fullUniName.Buffer ); 
           
          return STATUS_SUCCESS; 
           
     } 

     __except(1) 
     { 
          if(fullUniName.Buffer) ExFreePool( fullUniName.Buffer ); 
          if(pKey) ObDereferenceObject(pKey ); 
          return STATUS_SUCCESS; 
           
     } 

} 

BOOL GoOrNot(char *fathername,char *procname) 
{ 
     char buff[256] = {0}; 
     ULONG a; 
     LARGE_INTEGER li;li.QuadPart=-10000; 
     KeWaitForSingleObject(&event,Executive,KernelMode,0,0); 
      
     strcpy(buff,fathername); 
     strcat(buff,procname); 
      
     strncpy(&output[8],buff,sizeof(buff)); 
      
     a = 1; 
     memmove(&output[0],&a,4); 
     while (1) 
     { 
          KeDelayExecutionThread(KernelMode,0,&li); 
          memmove(&a,&output[0],4); 
          if (!a) 
          { 
               break; 
          } 
     } 
      
     memmove(&a,&output[4],4); 
     KeSetEvent(&event,0,0); 
     return a; 
} 


BOOL GetProcessName( PCHAR theName ) 
{ 
PEPROCESS curproc; 
char *nameptr; 
ULONG i; 
KIRQL oldirql; 
      
if( gProcessNameOffset ) 
{ 
curproc = PsGetCurrentProcess(); 
nameptr = (PCHAR) curproc + gProcessNameOffset; 
strncpy( theName, nameptr, NT_PROCNAMELEN ); 
theName[NT_PROCNAMELEN] = 0; /* NULL at end */ 
return TRUE; 
} 
return FALSE; 
} 

DWORD GetDllFunctionAddress(char* lpFunctionName, PUNICODE_STRING pDllName) 
{ 
HANDLE hThread, hSection, hFile, hMod; 
SECTION_IMAGE_INFORMATION sii; 
IMAGE_DOS_HEADER* dosheader; 
IMAGE_OPTIONAL_HEADER* opthdr; 
IMAGE_EXPORT_DIRECTORY* pExportTable; 
DWORD* arrayOfFunctionAddresses; 
DWORD* arrayOfFunctionNames; 
WORD* arrayOfFunctionOrdinals; 
DWORD functionOrdinal; 
DWORD Base, x, functionAddress; 
char* functionName; 
STRING ntFunctionName, ntFunctionNameSearch; 
PVOID BaseAddress = NULL; 
SIZE_T size=0; 

OBJECT_ATTRIBUTES oa = {sizeof oa, 0, pDllName, OBJ_CASE_INSENSITIVE}; 

IO_STATUS_BLOCK iosb; 

//_asm int 3; 
ZwOpenFile(&hFile, FILE_EXECUTE | SYNCHRONIZE, &oa, &iosb, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT); 

oa.ObjectName = 0; 

ZwCreateSection(&hSection, SECTION_ALL_ACCESS, &oa, 0,PAGE_EXECUTE, SEC_IMAGE, hFile); 

ZwMapViewOfSection(hSection, NtCurrentProcess(), &BaseAddress, 0, 1000, 0, &size, (SECTION_INHERIT)1, MEM_TOP_DOWN, PAGE_READWRITE); 

ZwClose(hFile); 

hMod = BaseAddress; 

dosheader = (IMAGE_DOS_HEADER *)hMod; 

opthdr =(IMAGE_OPTIONAL_HEADER *) ((BYTE*)hMod+dosheader->e_lfanew+24); 

pExportTable =(IMAGE_EXPORT_DIRECTORY*)((BYTE*) hMod + opthdr->DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT]. VirtualAddress); 

// now we can get the exported functions, but note we convert from RVA to address 
arrayOfFunctionAddresses = (DWORD*)( (BYTE*)hMod + pExportTable->AddressOfFunctions); 

arrayOfFunctionNames = (DWORD*)( (BYTE*)hMod + pExportTable->AddressOfNames); 

arrayOfFunctionOrdinals = (WORD*)( (BYTE*)hMod + pExportTable->AddressOfNameOrdinals); 

Base = pExportTable->Base; 

RtlInitString(&ntFunctionNameSearch, lpFunctionName); 

for(x = 0; x < pExportTable->NumberOfFunctions; x++) 
{ 
functionName = (char*)( (BYTE*)hMod + arrayOfFunctionNames[x]); 

RtlInitString(&ntFunctionName, functionName); 

functionOrdinal = arrayOfFunctionOrdinals[x] + Base - 1; // always need to add base, -1 as array counts from 0 
// this is the funny bit. you would expect the function pointer to simply be arrayOfFunctionAddresses[x]... 
// oh no... thats too simple. it is actually arrayOfFunctionAddresses[functionOrdinal]!! 
functionAddress = (DWORD)( (BYTE*)hMod + arrayOfFunctionAddresses[functionOrdinal]); 
if (RtlCompareString(&ntFunctionName, &ntFunctionNameSearch, TRUE) == 0) 
{ 
ZwClose(hSection); 
return functionAddress; 
} 
} 

ZwClose(hSection); 
return 0; 
} 



VOID OnUnload( IN PDRIVER_OBJECT DriverObject ) 
{ 
     UNICODE_STRING devlink; 
DbgPrint("PRMonitor: OnUnload called\n"); 
     if (bRegMon) 
     { 
          RegMoniterOff(); 
     } 
     if(bProcMon) 
     { 
          ProcMoniterOff(); 
     } 
     if (bModMon) 
     { 
          ModMonitorOff(); 
     } 
     RtlInitUnicodeString(&devlink,HOOKZWCREATEPROCESS_DOS_DEVICE_NAME_W); 
     IoDeleteSymbolicLink(&devlink); 
     if (DriverObject->DeviceObject) 
     { 
          IoDeleteDevice(DriverObject->DeviceObject); 
     } 
      
} 

NTSTATUS DriverEntry( IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath ) 
{ 
int i; 
UNICODE_STRING dllName; 
DWORD functionAddress; 
     UNICODE_STRING devname; 
     UNICODE_STRING devlink; 
     PDEVICE_OBJECT devob ; 
     NTSTATUS status ; 
     //_asm int 3; 
     DbgPrint("My Driver Loaded!"); 

     RtlInitUnicodeString(&devname,HOOKZWCREATEPROCESS_DEVICE_NAME_W); 
     RtlInitUnicodeString(&devlink,HOOKZWCREATEPROCESS_DOS_DEVICE_NAME_W); 

     status = IoCreateDevice(theDriverObject, 
                                   256, 
                                   &devname, 
                                   FILE_DEVICE_HOOKZWCREATEPROCESS, 
                                   0, 
                                   TRUE, 
                                   &devob); 

     if (!NT_SUCCESS(status)) 
     { 
          KdPrint(("Failed to create device .....")); 
          return status ; 
     } 

     status = IoCreateSymbolicLink(&devlink,&devname); 

     if (!NT_SUCCESS(status)) 
     { 
          KdPrint(("Failed to create symboliclink .......")); 
          IoDeleteDevice(devob); 
          return status; 
     } 

     theDriverObject->MajorFunction[IRP_MJ_CREATE] = 
     theDriverObject->MajorFunction[IRP_MJ_CLOSE] = DevCreateClose; 
     theDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DevDispatch ; 
     theDriverObject->DriverUnload = OnUnload; 
     KeInitializeEvent(&event,SynchronizationEvent,1); 



GetProcessNameOffset(); 


RtlInitUnicodeString(&dllName, L"\\Device\\HarddiskVolume1\\Windows\\System32\\ntdll.dll"); 
functionAddress = GetDllFunctionAddress("ZwCreateProcessEx", &dllName); 
position = *((WORD*)(functionAddress+1)); 

DbgPrint("ZwCreateProcessEx's Id:%d\n", position); 

     RealZwCreateProcess = (ZWCREATEPROCESS)(*(((PServiceDescriptorTableEntry)KeServiceDescriptorTable)->ServiceTableBase + position)); 
     functionAddress = GetDllFunctionAddress("ZwLoadDriver",&dllName); 
     pos = *((WORD*)((DWORD)ZwSetValueKey+1)); 
     po = *((WORD *)(functionAddress+1)); 

     DbgPrint("ZwSetValueKey's Id:%d\n", pos); 
     DbgPrint("ZwLoadDriver's Id:%d\n",po); 
     //DbgPrint("ZwLoadDriver's address is %d\n",ZwLoadDriver); 

     RealZwSetValueKey = (ZWSETVALUEKEY)(*(((PServiceDescriptorTableEntry)KeServiceDescriptorTable)->ServiceTableBase + pos)); 
     RealZwLoadDriver =      (ZWLOADDRIVER)(*(((PServiceDescriptorTableEntry)KeServiceDescriptorTable)->ServiceTableBase + po)); 
      

return STATUS_SUCCESS; 
} 


NTSTATUS FakedZwCreateProcess( 
                                    OUT PHANDLE ProcessHandle, 
                                    IN ACCESS_MASK DesiredAccess, 
                                    IN POBJECT_ATTRIBUTES ObjectAttributes, 
                                    IN HANDLE InheritFromProcessHandle, 
                                    IN BOOLEAN InheritHandles, 
                                    IN HANDLE SectionHandle OPTIONAL, 
                                    IN HANDLE DebugPort OPTIONAL, 
                                    IN HANDLE ExceptionPort OPTIONAL, 
                                    IN HANDLE Unknown 
                                    ) 
{ 
     char aProcessName[PROCNAMELEN]; 
     char aPathName[MAXPATHLEN]; 
      
     GetFullName(SectionHandle,aPathName); 

     GetProcessName(aProcessName); 
      
     DbgPrint("ZwCreateProcess is called by %s\n",aProcessName); 
     DbgPrint("The name is %s\n",aPathName); 
     strcat(aProcessName,"##"); 
     if (GoOrNot(aProcessName,aPathName)) 
     { 
          return RealZwCreateProcess( 
               ProcessHandle, 
               DesiredAccess, 
               ObjectAttributes, 
               InheritFromProcessHandle, 
               InheritHandles, 
               SectionHandle, 
               DebugPort, 
               ExceptionPort, 
               Unknown 
                                    ); 

     } 
     else 
     { 

          ProcessHandle = NULL; 
          return STATUS_SUCCESS; 
     } 
           
} 

NTSTATUS FakedZwSetValueKey 
( 
IN HANDLE KeyHandle, 
IN PUNICODE_STRING ValueName, 
IN ULONG TitleIndex OPTIONAL, 
IN ULONG Type, 
IN PVOID Data, 
IN ULONG DataSize 
) 
{ 
     char pch[MAXPATHLEN]; 
     char regValue[MAXPATHLEN]; 
     ANSI_STRING ansi; 
     char aProcessName[PROCNAMELEN]; 
     GetFullName2(KeyHandle,pch); 
     GetProcessName(aProcessName); 
     DbgPrint("ZwSetValueKey is called by %s\n",aProcessName); 

     RtlUnicodeStringToAnsiString(&ansi,ValueName,TRUE); 
          if(ansi.Length<MAXPATHLEN) 
          { 
               memcpy(regValue,ansi.Buffer,ansi.Length); 
               regValue[ansi.Length]=0x00; 
          } 
          else 
          { 
               memcpy(regValue,ansi.Buffer,MAXPATHLEN); 
               regValue[MAXPATHLEN-1]=0x00; 
          } 
           
          RtlFreeAnsiString( &ansi ); 
     strcat(aProcessName,"$$"); 
     strcat(pch,regValue); 
     if (GoOrNot(aProcessName,pch)) 
     { 
          return RealZwSetValueKey( 
                                         KeyHandle, 
                                         ValueName, 
                                         TitleIndex, 
                                         Type, 
                                         Data, 
                                         DataSize); 
     } 
     else 
     { 
          return STATUS_ACCESS_DENIED; 
     } 

} 

NTSTATUS FakedZwLoadDriver(IN PUNICODE_STRING DriverServiceName ) 
{ 
     char aProcessName[PROCNAMELEN]; 
     char aDrvname[MAXPATHLEN]; 
     ANSI_STRING ansi ; 
     GetProcessName(aProcessName); 
     RtlUnicodeStringToAnsiString(&ansi,DriverServiceName,TRUE); 
     if(ansi.Length<MAXPATHLEN) 
     { 
          memcpy(aDrvname,ansi.Buffer,ansi.Length); 
          aDrvname[ansi.Length]=0x00; 
     } 
     else 
     { 
          memcpy(aDrvname,ansi.Buffer,MAXPATHLEN); 
          aDrvname[MAXPATHLEN-1]=0x00; 
     } 
      
          RtlFreeAnsiString( &ansi ); 
     DbgPrint("ZwLoadDriver is called by %s\n",aProcessName); 
     DbgPrint("Driver name is %s\n",aDrvname); 

     strcat(aProcessName,"&&"); 

     if (GoOrNot(aProcessName,aDrvname)) 
     { 
          return RealZwLoadDriver( 
                                        DriverServiceName 
                                        ); 
     } 
     else 
     { 
               return STATUS_ACCESS_DENIED; 
     } 


} 

NTSTATUS 
DevCreateClose( 
                IN PDEVICE_OBJECT DeviceObject, 
                IN PIRP Irp 
) 
{ 

          Irp->IoStatus.Status = 0; 
          Irp->IoStatus.Information = 0 ; 

          IoCompleteRequest(Irp,IO_NO_INCREMENT); 

          return STATUS_SUCCESS ; 

} 

NTSTATUS 
DevDispatch( 
                IN PDEVICE_OBJECT DeviceObject, 
                IN PIRP Irp 
) 
{ 
     UCHAR *buff =0; 
     ULONG a; 
     PIO_STACK_LOCATION psloc= IoGetCurrentIrpStackLocation(Irp); 

      
     switch(psloc->Parameters.DeviceIoControl.IoControlCode) 
     { 
     case 1000: 
          DbgPrint("IoControlCode 1000\n"); 
          if(!bProcMon) 
          { 
               buff = (UCHAR *)Irp->AssociatedIrp.SystemBuffer ; 
                
               ProcMoniterOn(); 
                
               memmove(&a,&buff[4],4); 
               output=(char*)MmMapIoSpace(MmGetPhysicalAddress((void*)a),256,0); 
           
           
      
          } 
               break; 

          case 1001: 
               DbgPrint("IoControlCode 1001\n"); 
               if (bProcMon) 
               { 
                    ProcMoniterOff(); 
                     
               } 
               break; 

          case 1002: 
               DbgPrint("IoControlCode 1002\n"); 
               if (!bRegMon) 
               { 
                    buff = (UCHAR *)Irp->AssociatedIrp.SystemBuffer ; 

                    RegMoniterOn(); 

                    memmove(&a,&buff[4],4); 
                    output=(char*)MmMapIoSpace(MmGetPhysicalAddress((void*)a),256,0); 

               } 
               break; 

          case 1003: 
               DbgPrint("IoControlCode 1003\n"); 
               if (bRegMon) 
               { 
                    RegMoniterOff(); 
                     
               } 
               break; 

          case 1004: 
               DbgPrint("IoControlCode 1004\n"); 
               if (!bModMon) 
               { 
                    buff = (UCHAR *)Irp->AssociatedIrp.SystemBuffer ; 
                     
                    ModMonitorOn(); 
                     
                    memmove(&a,&buff[4],4); 
                    output=(char*)MmMapIoSpace(MmGetPhysicalAddress((void*)a),256,0); 
               } 
               break; 
                
          case 1005: 
               DbgPrint("IoControlCode 1005\n"); 
               if (bModMon) 
               { 
                    ModMonitorOff(); 
               } 
               break; 
          } 
           
                                              

     Irp->IoStatus.Status = 0; 
     IoCompleteRequest(Irp,IO_NO_INCREMENT); 
     return STATUS_SUCCESS ; 
} 

VOID ModMonitorOn() 
{ 
     DbgPrint("ModMonitorOn\n"); 
     _asm 
     { 
               CLI //disable interrupt 
               MOV EAX, CR0 //move CR0 register into EAX 
               AND EAX, NOT 10000H //disable WP bit 
               MOV CR0, EAX 

     } 

     (ZWLOADDRIVER)(*(((PServiceDescriptorTableEntry)KeServiceDescriptorTable)->ServiceTableBase + po)) = FakedZwLoadDriver ; 


     _asm 
     { 
           
               MOV EAX, CR0 //move CR0 register into EAX 
               OR EAX, 10000H //enable WP bit 
               MOV CR0, EAX //write register back 
               STI //enable interrupt 
     } 
     bModMon =1 ; 
} 

VOID ModMonitorOff() 
{ 
     DbgPrint("ModMonitorOff\n"); 
     _asm 
     { 
          CLI //disable interrupt 
               MOV EAX, CR0 //move CR0 register into EAX 
               AND EAX, NOT 10000H //disable WP bit 
               MOV CR0, EAX 
                
     } 
      
     (ZWLOADDRIVER)(*(((PServiceDescriptorTableEntry)KeServiceDescriptorTable)->ServiceTableBase + po)) = RealZwLoadDriver ; 
      
      
     _asm 
     { 
           
          MOV EAX, CR0 //move CR0 register into EAX 
               OR EAX, 10000H //enable WP bit 
               MOV CR0, EAX //write register back 
               STI //enable interrupt 
     } 
     bModMon =0 ; 
} 

VOID RegMoniterOn() 
{ 
     DbgPrint("RegMonitorON\n"); 
     _asm 
     { 
           
               CLI //disable interrupt 
               MOV EAX, CR0 //move CR0 register into EAX 
               AND EAX, NOT 10000H //disable WP bit 
               MOV CR0, EAX //write register back 
                
     } 
                     
          (ZWSETVALUEKEY)(*(((PServiceDescriptorTableEntry)KeServiceDescriptorTable)->ServiceTableBase + pos)) = FakedZwSetValueKey ; 
                     
                     
     _asm 
     { 
                          
               MOV EAX, CR0 //move CR0 register into EAX 
               OR EAX, 10000H //enable WP bit 
               MOV CR0, EAX //write register back 
               STI //enable interrupt 
     } 
     bRegMon = 1 ; 


} 

VOID RegMoniterOff() 
{ 
     DbgPrint("RegMonitorOff\n"); 
     _asm 
     { 
           
               CLI //disable interrupt 
               MOV EAX, CR0 //move CR0 register into EAX 
               AND EAX, NOT 10000H //disable WP bit 
               MOV CR0, EAX //write register back 
                
     } 
                     
               (ZWSETVALUEKEY)(*(((PServiceDescriptorTableEntry)KeServiceDescriptorTable)->ServiceTableBase + pos)) = RealZwSetValueKey ; 
                     
                     
     _asm 
     { 
                          
               MOV EAX, CR0 //move CR0 register into EAX 
               OR EAX, 10000H //enable WP bit 
               MOV CR0, EAX //write register back 
               STI //enable interrupt 
     } 
     bRegMon = 0; 

} 

VOID ProcMoniterOn() 
{ 
     DbgPrint("ProcMonitorOn\n"); 
     _asm 
           
     { 
           
               CLI //disable interrupt 
               MOV EAX, CR0 //move CR0 register into EAX 
               AND EAX, NOT 10000H //disable WP bit 
               MOV CR0, EAX //write register back 
                
     } 
      
     (ZWCREATEPROCESS)(*(((PServiceDescriptorTableEntry)KeServiceDescriptorTable)->ServiceTableBase + position)) = FakedZwCreateProcess ; 
      
      
     _asm 
     { 
           
               MOV EAX, CR0 //move CR0 register into EAX 
               OR EAX, 10000H //enable WP bit 
               MOV CR0, EAX //write register back 
               STI //enable interrupt 
     } 


     bProcMon = 1; 

} 

VOID ProcMoniterOff() 
{ 
     DbgPrint("ProcMonitorOff\n"); 
     _asm 
           
     { 
           
               CLI //disable interrupt 
               MOV EAX, CR0 //move CR0 register into EAX 
               AND EAX, NOT 10000H //disable WP bit 
               MOV CR0, EAX //write register back 
                
     } 
      
     (ZWCREATEPROCESS)(*(((PServiceDescriptorTableEntry)KeServiceDescriptorTable)->ServiceTableBase + position)) = RealZwCreateProcess ; 
      
      
     _asm 
     { 
           
               MOV EAX, CR0 //move CR0 register into EAX 
               OR EAX, 10000H //enable WP bit 
               MOV CR0, EAX //write register back 
               STI //enable interrupt 
     } 

     bProcMon = 0; 

}