/* TinyOS V1 — Self-Contained UEFI Type Definitions
 *
 * Defines all UEFI types, protocols, and structures needed by the kernel.
 * No GNU-EFI dependency required. All struct layouts follow the
 * UEFI Specification v2.x for x86_64 (Microsoft x64 calling convention).
 *
 * EFIAPI = __attribute__((ms_abi)) on clang/gcc/x86_64
 */

#ifndef TINYOS_UEFI_TYPES_H
#define TINYOS_UEFI_TYPES_H

/* ============================================================================
 * 1. Fundamental Types
 * ============================================================================ */

typedef unsigned long long UINT64;
typedef unsigned int       UINT32;
typedef unsigned short     UINT16;
typedef unsigned char      UINT8;
typedef long long          INT64;
typedef int                INT32;
typedef short              INT16;
typedef char               INT8;
typedef unsigned long long UINTN;   /* 64-bit on x86_64 */
typedef long long          INTN;
typedef unsigned char      BOOLEAN;
typedef unsigned short     CHAR16;
typedef UINTN              EFI_STATUS;
typedef void*              EFI_HANDLE;
typedef void               VOID;
typedef UINT64             EFI_PHYSICAL_ADDRESS;
typedef VOID               *EFI_EVENT;    /* Event handle — defined early; used by many protocols */

/* Forward declaration of EFI_SYSTEM_TABLE for use in EFI_LOADED_IMAGE_PROTOCOL */
typedef struct _EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;

/* EFIAPI: UEFI uses the Microsoft x64 calling convention on x86_64 */
#if defined(__x86_64__) || defined(_M_X64)
  #define EFIAPI __attribute__((ms_abi))
#else
  #define EFIAPI
#endif

/* Common EFI return codes */
#define EFI_SUCCESS               0x0000000000000000ULL
#define EFI_LOAD_ERROR            0x0000000000000001ULL
#define EFI_INVALID_PARAMETER     0x0000000000000002ULL
#define EFI_UNSUPPORTED           0x0000000000000003ULL
#define EFI_BAD_BUFFER_SIZE       0x0000000000000004ULL
#define EFI_BUFFER_TOO_SMALL      0x0000000000000005ULL
#define EFI_NOT_READY             0x0000000000000006ULL
#define EFI_DEVICE_ERROR          0x0000000000000007ULL
#define EFI_WRITE_PROTECTED       0x0000000000000008ULL
#define EFI_OUT_OF_RESOURCES      0x0000000000000009ULL
#define EFI_VOLUME_CORRUPTED      0x000000000000000AULL
#define EFI_VOLUME_FULL           0x000000000000000BULL
#define EFI_NO_MEDIA              0x000000000000000CULL
#define EFI_MEDIA_CHANGED         0x000000000000000DULL
#define EFI_NOT_FOUND             0x000000000000000EULL
#define EFI_ACCESS_DENIED         0x000000000000000FULL
#define EFI_NO_RESPONSE           0x0000000000000010ULL
#define EFI_NO_MAPPING            0x0000000000000011ULL
#define EFI_TIMEOUT               0x0000000000000012ULL
#define EFI_NOT_STARTED           0x0000000000000013ULL
#define EFI_ALREADY_STARTED       0x0000000000000014ULL
#define EFI_ABORTED               0x0000000000000015ULL
#define EFI_ICMP_ERROR            0x0000000000000016ULL
#define EFI_TFTP_ERROR            0x0000000000000017ULL
#define EFI_PROTOCOL_ERROR        0x0000000000000018ULL
#define EFI_INCOMPATIBLE_VERSION  0x0000000000000019ULL
#define EFI_SECURITY_VIOLATION    0x000000000000001AULL
#define EFI_CRC_ERROR             0x000000000000001BULL
#define EFI_END_OF_MEDIA          0x000000000000001CULL
#define EFI_END_OF_FILE           0x000000000000001DULL
#define EFI_INVALID_LANGUAGE      0x000000000000001EULL
#define EFI_COMPROMISED_DATA      0x000000000000001FULL

#define EFI_ERROR(x)              (((INTN)(x)) < 0)

/* ============================================================================
 * 2. Common UEFI Structures
 * ============================================================================ */

typedef struct {
    UINT64  Signature;
    UINT32  Revision;
    UINT32  HeaderSize;
    UINT32  CRC32;
    UINT32  Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    UINT32  Data1;
    UINT16  Data2;
    UINT16  Data3;
    UINT8   Data4[8];
} EFI_GUID;

/* ============================================================================
 * 3. Memory Types
 * ============================================================================ */

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

/* ============================================================================
 * 4. EFI_SIMPLE_TEXT_INPUT_PROTOCOL
 * ============================================================================ */

typedef struct {
    UINT16  ScanCode;
    CHAR16  UnicodeChar;
} EFI_INPUT_KEY;

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_INPUT_RESET)(
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    BOOLEAN ExtendedVerification
);

typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY)(
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    EFI_INPUT_KEY *Key
);

struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_INPUT_RESET     Reset;
    EFI_INPUT_READ_KEY  ReadKeyStroke;
    EFI_EVENT           WaitForKey;   /* EFI_EVENT is void* */
};

/* ============================================================================
 * 5. EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
 * ============================================================================ */

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_TEXT_RESET)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    BOOLEAN ExtendedVerification
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    CHAR16 *String
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_TEST_STRING)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    CHAR16 *String
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_QUERY_MODE)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN ModeNumber,
    UINTN *Columns,
    UINTN *Rows
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_MODE)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN ModeNumber
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN Attribute
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN Column,
    UINTN Row
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_ENABLE_CURSOR)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    BOOLEAN Visible
);

typedef struct {
    INT32   MaxMode;
    INT32   Mode;
    INT32   Attribute;
    INT32   CursorColumn;
    INT32   CursorRow;
    BOOLEAN CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET                  Reset;
    EFI_TEXT_STRING                 OutputString;
    EFI_TEXT_TEST_STRING            TestString;
    EFI_TEXT_QUERY_MODE             QueryMode;
    EFI_TEXT_SET_MODE               SetMode;
    EFI_TEXT_SET_ATTRIBUTE          SetAttribute;
    EFI_TEXT_CLEAR_SCREEN           ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION    SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR          EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE         *Mode;
};

/* ============================================================================
 * 6. EFI_GRAPHICS_OUTPUT_PROTOCOL (GOP)
 * ============================================================================ */

typedef struct {
    UINT32  RedMask;
    UINT32  GreenMask;
    UINT32  BlueMask;
    UINT32  ReservedMask;
} EFI_PIXEL_BITMASK;

typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    UINT32                      Version;
    UINT32                      HorizontalResolution;
    UINT32                      VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT   PixelFormat;
    EFI_PIXEL_BITMASK           PixelInformation;
    UINT32                      PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT32                              MaxMode;
    UINT32                              Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN                               SizeOfInfo;
    EFI_PHYSICAL_ADDRESS                FrameBufferBase;  /* UINT64 */
    UINTN                               FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    UINT32 ModeNumber,
    UINTN *SizeOfInfo,
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
);

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    UINT32 ModeNumber
);

/* BLT operation and pixel types (defined before Blt function pointer) */
typedef enum {
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

typedef struct {
    UINT8 Blue;
    UINT8 Green;
    UINT8 Red;
    UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
    EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    UINTN SourceX,
    UINTN SourceY,
    UINTN DestinationX,
    UINTN DestinationY,
    UINTN Width,
    UINTN Height,
    UINTN Delta
);

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE  QueryMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE    SetMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT         Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE        *Mode;
};

/* ============================================================================
 * 7. EFI_DEVICE_PATH_PROTOCOL (minimal)
 * ============================================================================ */

typedef struct {
    UINT8   Type;
    UINT8   SubType;
    UINT8   Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

/* ============================================================================
 * 8. EFI_LOADED_IMAGE_PROTOCOL
 * ============================================================================ */

typedef struct {
    UINT32      Revision;
    EFI_HANDLE  ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;   /* forward declared below */

    EFI_HANDLE              DeviceHandle;
    EFI_DEVICE_PATH_PROTOCOL *FilePath;
    VOID                    *Reserved;

    UINT32  LoadOptionsSize;
    VOID    *LoadOptions;

    VOID    *ImageBase;
    UINT64  ImageSize;
    EFI_MEMORY_TYPE ImageDataType;
    EFI_STATUS (EFIAPI *Unload)(EFI_HANDLE ImageHandle);
} EFI_LOADED_IMAGE_PROTOCOL;

/* ============================================================================
 * 9. EFI_FILE_PROTOCOL
 * ============================================================================ */

typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN)(
    EFI_FILE_PROTOCOL *This,
    EFI_FILE_PROTOCOL **NewHandle,
    CHAR16 *FileName,
    UINT64 OpenMode,
    UINT64 Attributes
);

typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE)(
    EFI_FILE_PROTOCOL *This
);

typedef EFI_STATUS (EFIAPI *EFI_FILE_DELETE)(
    EFI_FILE_PROTOCOL *This
);

typedef EFI_STATUS (EFIAPI *EFI_FILE_READ)(
    EFI_FILE_PROTOCOL *This,
    UINTN *BufferSize,
    VOID *Buffer
);

typedef EFI_STATUS (EFIAPI *EFI_FILE_WRITE)(
    EFI_FILE_PROTOCOL *This,
    UINTN *BufferSize,
    VOID *Buffer
);

typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_POSITION)(
    EFI_FILE_PROTOCOL *This,
    UINT64 *Position
);

typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_POSITION)(
    EFI_FILE_PROTOCOL *This,
    UINT64 Position
);

typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_INFO)(
    EFI_FILE_PROTOCOL *This,
    EFI_GUID *InformationType,
    UINTN *BufferSize,
    VOID *Buffer
);

typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_INFO)(
    EFI_FILE_PROTOCOL *This,
    EFI_GUID *InformationType,
    UINTN BufferSize,
    VOID *Buffer
);

typedef EFI_STATUS (EFIAPI *EFI_FILE_FLUSH)(
    EFI_FILE_PROTOCOL *This
);

struct _EFI_FILE_PROTOCOL {
    UINT64              Revision;
    EFI_FILE_OPEN       Open;
    EFI_FILE_CLOSE      Close;
    EFI_FILE_DELETE     Delete;
    EFI_FILE_READ       Read;
    EFI_FILE_WRITE      Write;
    EFI_FILE_GET_POSITION GetPosition;
    EFI_FILE_SET_POSITION SetPosition;
    EFI_FILE_GET_INFO   GetInfo;
    EFI_FILE_SET_INFO   SetInfo;
    EFI_FILE_FLUSH      Flush;
};

/* File open modes */
#define EFI_FILE_MODE_READ      0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE     0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE    0x8000000000000000ULL

/* File attributes */
#define EFI_FILE_READ_ONLY      0x0000000000000001ULL
#define EFI_FILE_HIDDEN         0x0000000000000002ULL
#define EFI_FILE_SYSTEM         0x0000000000000004ULL
#define EFI_FILE_RESERVED       0x0000000000000008ULL
#define EFI_FILE_DIRECTORY      0x0000000000000010ULL
#define EFI_FILE_ARCHIVE        0x0000000000000020ULL

/* EFI_FILE_INFO GUID: {09576e92-6d3f-11d2-8e39-00a0c969723b} */
#define EFI_FILE_INFO_ID \
    {0x09576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}

typedef struct {
    UINT64      Size;
    UINT64      FileSize;
    UINT64      PhysicalSize;
    /* Time fields omitted for brevity — fields below still need correct offset */
    UINT8       _pad1[48];  /* CreateTime, LastAccessTime, ModificationTime */
    UINT64      Attribute;
    CHAR16      FileName[1]; /* variable length */
} EFI_FILE_INFO;

/* EFI_FILE_SYSTEM_INFO GUID: {09576e93-6d3f-11d2-8e39-00a0c969723b} */
#define EFI_FILE_SYSTEM_INFO_ID \
    {0x09576e93, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}

/* ============================================================================
 * 10. EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
 * ============================================================================ */

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME)(
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
    EFI_FILE_PROTOCOL **Root
);

struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    UINT64                                      Revision;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume;
};

/* ============================================================================
 * 11. EFI_BOOT_SERVICES
 * ============================================================================ */

typedef EFI_STATUS (EFIAPI *EFI_RAISE_TPL)(UINTN NewTpl);
typedef VOID     (EFIAPI *EFI_RESTORE_TPL)(UINTN OldTpl);

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_PAGES)(
    UINTN Type, UINTN MemoryType, UINTN Pages, UINT64 *Memory
);
typedef EFI_STATUS (EFIAPI *EFI_FREE_PAGES)(UINT64 Memory, UINTN Pages);
typedef EFI_STATUS (EFIAPI *EFI_GET_MEMORY_MAP)(
    UINTN *MemoryMapSize, VOID *MemoryMap, UINTN *MapKey,
    UINTN *DescriptorSize, UINT32 *DescriptorVersion
);
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(
    EFI_MEMORY_TYPE PoolType, UINTN Size, VOID **Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(VOID *Buffer);

typedef EFI_STATUS (EFIAPI *EFI_CREATE_EVENT)(
    UINT32 Type, UINTN NotifyTpl, VOID *NotifyFunction,
    VOID *NotifyContext, EFI_EVENT *Event
);
typedef EFI_STATUS (EFIAPI *EFI_SET_TIMER)(EFI_EVENT Event, UINTN Type, UINT64 TriggerTime);
typedef EFI_STATUS (EFIAPI *EFI_WAIT_FOR_EVENT)(
    UINTN NumberOfEvents, EFI_EVENT *Event, UINTN *Index
);
typedef EFI_STATUS (EFIAPI *EFI_SIGNAL_EVENT)(EFI_EVENT Event);
typedef EFI_STATUS (EFIAPI *EFI_CLOSE_EVENT)(EFI_EVENT Event);
typedef EFI_STATUS (EFIAPI *EFI_CHECK_EVENT)(EFI_EVENT Event);

typedef EFI_STATUS (EFIAPI *EFI_INSTALL_PROTOCOL_INTERFACE)(
    EFI_HANDLE *Handle, EFI_GUID *Protocol, UINTN InterfaceType, VOID *Interface
);
typedef EFI_STATUS (EFIAPI *EFI_REINSTALL_PROTOCOL_INTERFACE)(
    EFI_HANDLE Handle, EFI_GUID *Protocol, VOID *OldInterface, VOID *NewInterface
);
typedef EFI_STATUS (EFIAPI *EFI_UNINSTALL_PROTOCOL_INTERFACE)(
    EFI_HANDLE Handle, EFI_GUID *Protocol, VOID *Interface
);
typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(
    EFI_HANDLE Handle, EFI_GUID *Protocol, VOID **Interface
);
typedef EFI_STATUS (EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY)(
    EFI_GUID *Protocol, EFI_EVENT Event, VOID **Registration
);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE)(
    UINTN SearchType, EFI_GUID *Protocol, VOID *SearchKey,
    UINTN *BufferSize, EFI_HANDLE *Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_DEVICE_PATH)(
    EFI_GUID *Protocol, EFI_DEVICE_PATH_PROTOCOL **DevicePath, EFI_HANDLE *Device
);
typedef EFI_STATUS (EFIAPI *EFI_INSTALL_CONFIGURATION_TABLE)(
    EFI_GUID *Guid, VOID *Table
);

typedef EFI_STATUS (EFIAPI *EFI_IMAGE_LOAD)(
    BOOLEAN BootPolicy, EFI_HANDLE ParentImageHandle,
    EFI_DEVICE_PATH_PROTOCOL *DevicePath, VOID *SourceBuffer,
    UINTN SourceSize, EFI_HANDLE *ImageHandle
);
typedef EFI_STATUS (EFIAPI *EFI_IMAGE_START)(
    EFI_HANDLE ImageHandle, UINTN *ExitDataSize, CHAR16 **ExitData
);
typedef EFI_STATUS (EFIAPI *EFI_EXIT)(
    EFI_HANDLE ImageHandle, EFI_STATUS ExitStatus,
    UINTN ExitDataSize, CHAR16 *ExitData
);
typedef EFI_STATUS (EFIAPI *EFI_IMAGE_UNLOAD)(EFI_HANDLE ImageHandle);
typedef EFI_STATUS (EFIAPI *EFI_EXIT_BOOT_SERVICES)(
    EFI_HANDLE ImageHandle, UINTN MapKey
);

typedef EFI_STATUS (EFIAPI *EFI_GET_NEXT_MONOTONIC_COUNT)(UINT64 *Count);
typedef EFI_STATUS (EFIAPI *EFI_STALL)(UINTN Microseconds);
typedef EFI_STATUS (EFIAPI *EFI_SET_WATCHDOG_TIMER)(
    UINTN Timeout, UINT64 WatchdogCode, UINTN DataSize, CHAR16 *WatchdogData
);

typedef EFI_STATUS (EFIAPI *EFI_CONNECT_CONTROLLER)(
    EFI_HANDLE ControllerHandle, EFI_HANDLE *DriverImageHandle,
    EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath, BOOLEAN Recursive
);
typedef EFI_STATUS (EFIAPI *EFI_DISCONNECT_CONTROLLER)(
    EFI_HANDLE ControllerHandle, EFI_HANDLE DriverImageHandle,
    EFI_HANDLE ChildHandle
);

typedef EFI_STATUS (EFIAPI *EFI_OPEN_PROTOCOL)(
    EFI_HANDLE Handle, EFI_GUID *Protocol,
    VOID **Interface, EFI_HANDLE AgentHandle,
    EFI_HANDLE ControllerHandle, UINT32 Attributes
);
typedef EFI_STATUS (EFIAPI *EFI_CLOSE_PROTOCOL)(
    EFI_HANDLE Handle, EFI_GUID *Protocol,
    EFI_HANDLE AgentHandle, EFI_HANDLE ControllerHandle
);
typedef EFI_STATUS (EFIAPI *EFI_OPEN_PROTOCOL_INFORMATION)(
    EFI_HANDLE Handle, EFI_GUID *Protocol,
    VOID **EntryBuffer, UINTN *EntryCount
);

typedef EFI_STATUS (EFIAPI *EFI_PROTOCOLS_PER_HANDLE)(
    EFI_HANDLE Handle, EFI_GUID ***ProtocolBuffer, UINTN *ProtocolBufferCount
);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(
    UINTN SearchType, EFI_GUID *Protocol, VOID *SearchKey,
    UINTN *NoHandles, EFI_HANDLE **Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(
    EFI_GUID *Protocol, VOID *Registration, VOID **Interface
);
typedef EFI_STATUS (EFIAPI *EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES)(
    EFI_HANDLE *Handle, ...
);
typedef EFI_STATUS (EFIAPI *EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)(
    EFI_HANDLE Handle, ...
);

typedef EFI_STATUS (EFIAPI *EFI_CALCULATE_CRC32)(
    VOID *Data, UINTN DataSize, UINT32 *Crc32
);

typedef VOID (EFIAPI *EFI_COPY_MEM)(VOID *Destination, VOID *Source, UINTN Length);
typedef VOID (EFIAPI *EFI_SET_MEM)(VOID *Buffer, UINTN Size, UINT8 Value);

typedef EFI_STATUS (EFIAPI *EFI_CREATE_EVENT_EX)(
    UINT32 Type, UINTN NotifyTpl, VOID *NotifyFunction,
    VOID *NotifyContext, EFI_GUID *EventGroup, EFI_EVENT *Event
);

/* LocateProtocol search types */
#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001U
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002U
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004U
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008U
#define EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010U
#define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020U

typedef struct {
    EFI_TABLE_HEADER                    Hdr;

    /* Task Priority */
    EFI_RAISE_TPL                       RaiseTPL;
    EFI_RESTORE_TPL                     RestoreTPL;

    /* Memory */
    EFI_ALLOCATE_PAGES                  AllocatePages;
    EFI_FREE_PAGES                      FreePages;
    EFI_GET_MEMORY_MAP                  GetMemoryMap;
    EFI_ALLOCATE_POOL                   AllocatePool;
    EFI_FREE_POOL                       FreePool;

    /* Events */
    EFI_CREATE_EVENT                    CreateEvent;
    EFI_SET_TIMER                       SetTimer;
    EFI_WAIT_FOR_EVENT                  WaitForEvent;
    EFI_SIGNAL_EVENT                    SignalEvent;
    EFI_CLOSE_EVENT                     CloseEvent;
    EFI_CHECK_EVENT                     CheckEvent;

    /* Protocol handlers */
    EFI_INSTALL_PROTOCOL_INTERFACE      InstallProtocolInterface;
    EFI_REINSTALL_PROTOCOL_INTERFACE    ReinstallProtocolInterface;
    EFI_UNINSTALL_PROTOCOL_INTERFACE    UninstallProtocolInterface;
    EFI_HANDLE_PROTOCOL                 HandleProtocol;
    VOID                                *Reserved;
    EFI_REGISTER_PROTOCOL_NOTIFY        RegisterProtocolNotify;
    EFI_LOCATE_HANDLE                   LocateHandle;
    EFI_LOCATE_DEVICE_PATH              LocateDevicePath;
    EFI_INSTALL_CONFIGURATION_TABLE     InstallConfigurationTable;

    /* Image */
    EFI_IMAGE_LOAD                      LoadImage;
    EFI_IMAGE_START                     StartImage;
    EFI_EXIT                            Exit;
    EFI_IMAGE_UNLOAD                    UnloadImage;
    EFI_EXIT_BOOT_SERVICES              ExitBootServices;

    /* Misc */
    EFI_GET_NEXT_MONOTONIC_COUNT        GetNextMonotonicCount;
    EFI_STALL                           Stall;
    EFI_SET_WATCHDOG_TIMER              SetWatchdogTimer;

    /* Driver */
    EFI_CONNECT_CONTROLLER              ConnectController;
    EFI_DISCONNECT_CONTROLLER           DisconnectController;

    /* Protocol open/close */
    EFI_OPEN_PROTOCOL                   OpenProtocol;
    EFI_CLOSE_PROTOCOL                  CloseProtocol;
    EFI_OPEN_PROTOCOL_INFORMATION       OpenProtocolInformation;

    /* Library */
    EFI_PROTOCOLS_PER_HANDLE            ProtocolsPerHandle;
    EFI_LOCATE_HANDLE_BUFFER            LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL                 LocateProtocol;
    EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES InstallMultipleProtocolInterfaces;
    EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES UninstallMultipleProtocolInterfaces;

    /* CRC & misc */
    EFI_CALCULATE_CRC32                 CalculateCrc32;
    EFI_COPY_MEM                        CopyMem;
    EFI_SET_MEM                         SetMem;
    EFI_CREATE_EVENT_EX                 CreateEventEx;
} EFI_BOOT_SERVICES;

/* ============================================================================
 * 12. EFI_RUNTIME_SERVICES (minimal — we only need the header)
 * ============================================================================ */

typedef struct {
    EFI_TABLE_HEADER    Hdr;
    VOID                *GetTime;
    VOID                *SetTime;
    VOID                *GetWakeupTime;
    VOID                *SetWakeupTime;
    VOID                *SetVirtualAddressMap;
    VOID                *ConvertPointer;
    VOID                *GetVariable;
    VOID                *GetNextVariableName;
    VOID                *SetVariable;
    VOID                *GetNextHighMonotonicCount;
    VOID                *ResetSystem;
} EFI_RUNTIME_SERVICES;

/* ============================================================================
 * 13. EFI_CONFIGURATION_TABLE
 * ============================================================================ */

typedef struct {
    EFI_GUID    VendorGuid;
    VOID        *VendorTable;
} EFI_CONFIGURATION_TABLE;

/* ============================================================================
 * 14. EFI_SYSTEM_TABLE
 * ============================================================================ */

struct _EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER                    Hdr;
    CHAR16                              *FirmwareVendor;
    UINT32                              FirmwareRevision;
    EFI_HANDLE                          ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL      *ConIn;
    EFI_HANDLE                          ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL     *ConOut;
    EFI_HANDLE                          StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL     *StdErr;
    EFI_RUNTIME_SERVICES                *RuntimeServices;
    EFI_BOOT_SERVICES                   *BootServices;
    UINTN                               NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE             *ConfigurationTable;
};

/* ============================================================================
 * 15. Common GUIDs
 * ============================================================================ */

/* EFI_GRAPHICS_OUTPUT_PROTOCOL GUID: {9042a9de-23dc-4a38-96fb-7aded080516a} */
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    {0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}}

/* EFI_LOADED_IMAGE_PROTOCOL GUID: {5b1b31a1-9562-11d2-8e3f-00a0c969723b} */
#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5b1b31a1, 0x9562, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}

/* EFI_SIMPLE_FILE_SYSTEM_PROTOCOL GUID: {964e5b22-6459-11d2-8e39-00a0c969723b} */
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}

/* ============================================================================
 * 16. Memory comparison helpers (inline)
 * ============================================================================ */

static inline BOOLEAN guid_equals(EFI_GUID *a, EFI_GUID *b) {
    UINT32 *a32 = (UINT32*)a;
    UINT32 *b32 = (UINT32*)b;
    return (a32[0] == b32[0] && a32[1] == b32[1] &&
            a32[2] == b32[2] && a32[3] == b32[3]);
}

static inline VOID memcpy_efi(VOID *dst, const VOID *src, UINTN n) {
    UINT8 *d = (UINT8*)dst;
    const UINT8 *s = (const UINT8*)src;
    for (UINTN i = 0; i < n; i++) d[i] = s[i];
}

static inline VOID memset_efi(VOID *dst, UINT8 val, UINTN n) {
    UINT8 *d = (UINT8*)dst;
    for (UINTN i = 0; i < n; i++) d[i] = val;
}

static inline INTN strcmp_efi(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (INTN)(*a) - (INTN)(*b);
}

static inline UINTN strlen_efi(const char *s) {
    UINTN n = 0;
    while (*s++) n++;
    return n;
}

#endif /* TINYOS_UEFI_TYPES_H */
