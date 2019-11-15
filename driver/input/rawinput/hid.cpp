
typedef BOOLEAN (__stdcall *HidD_GetProductString_)(HANDLE handle, PVOID buffer, ULONG buffer_len);
typedef VOID (__stdcall *HidD_GetHidGuid_)(GUID* hidGuid);
typedef BOOLEAN (__stdcall *HidD_GetPreparsedData_)(HANDLE handle, PHIDP_PREPARSED_DATA *preparsed_data);
typedef BOOLEAN (__stdcall *HidD_FreePreparsedData_)(PHIDP_PREPARSED_DATA preparsed_data);
typedef NTSTATUS (__stdcall *HidP_GetCaps_)(PHIDP_PREPARSED_DATA preparsed_data, HIDP_CAPS *caps);
typedef NTSTATUS (__stdcall *HidP_GetButtonCaps_)(HIDP_REPORT_TYPE reportType, PHIDP_BUTTON_CAPS buttonCaps, PUSHORT buttonCapsLength, PHIDP_PREPARSED_DATA preparsed_data);
typedef NTSTATUS (__stdcall *HidP_GetUsages_)(HIDP_REPORT_TYPE reportType, USAGE usagePage, USHORT LinkCollection, PUSAGE UsageList, PULONG UsageLength, PHIDP_PREPARSED_DATA PreparsedData, PCHAR Report, ULONG ReportLength);
typedef NTSTATUS (__stdcall *HidP_GetValueCaps_)(HIDP_REPORT_TYPE ReportType, PHIDP_VALUE_CAPS ValueCaps, PUSHORT ValueCapsLength, PHIDP_PREPARSED_DATA PreparsedData);
typedef NTSTATUS (__stdcall *HidP_GetUsageValue_)(HIDP_REPORT_TYPE ReportType, USAGE UsagePage, USHORT LinkCollection, USAGE Usage,PULONG UsageValue, PHIDP_PREPARSED_DATA PreparsedData,PCHAR Report, ULONG ReportLength);

static HidD_GetProductString_ HidD_GetProductString;
static HidD_GetHidGuid_ HidD_GetHidGuid;
static HidD_GetPreparsedData_ HidD_GetPreparsedData;
static HidD_FreePreparsedData_ HidD_FreePreparsedData;
static HidP_GetCaps_ HidP_GetCaps;
static HidP_GetButtonCaps_ HidP_GetButtonCaps;
static HidP_GetUsages_ HidP_GetUsages;
static HidP_GetValueCaps_ HidP_GetValueCaps;
static HidP_GetUsageValue_ HidP_GetUsageValue;
	
static auto lookupHidLibrary() -> bool {
	
	static HMODULE libHandle = LoadLibraryA("hid.dll");
	if (libHandle) {
#define RESOLVE(x) x = (x##_)GetProcAddress(libHandle, #x); if (!x) return false;
		RESOLVE(HidD_GetProductString);
        RESOLVE(HidD_GetHidGuid);
		RESOLVE(HidD_GetPreparsedData);
		RESOLVE(HidD_FreePreparsedData);
		RESOLVE(HidP_GetCaps);
		RESOLVE(HidP_GetButtonCaps);
		RESOLVE(HidP_GetUsages);
		RESOLVE(HidP_GetValueCaps);
		RESOLVE(HidP_GetUsageValue);
#undef RESOLVE
		
		return true;
	}
	
	return false;
}