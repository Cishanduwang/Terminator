#include"pch.h"
#include"Common.h"

UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\TERMINATOR");
UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\??\\TERMINATORLINK");
PDEVICE_OBJECT DeviceObject = NULL;

void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	IoDeleteSymbolicLink(&SymlinkName);
	IoDeleteDevice(DeviceObject);
}

NTSTATUS DispatchPassThru(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	NTSTATUS status = STATUS_SUCCESS;

	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	switch (irpsp->MajorFunction)
	{
	case IRP_MJ_CREATE:
		KdPrint(("[*]Terminator:IRP_Create."));
		break;
	case IRP_MJ_CLOSE:
		KdPrint(("[*]Terminator:IRP_Close"));
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
		break;
	}

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS DispatchIOCTL(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);

	switch (irpsp->Parameters.DeviceIoControl.IoControlCode)
	{
	case TERMINATOR_TERMINATEPROCESS:
	{
		auto len = irpsp->Parameters.DeviceIoControl.InputBufferLength;
		if (len < sizeof(ProcessData))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		auto data = (ProcessData*)irpsp->Parameters.DeviceIoControl.Type3InputBuffer;
		if (data == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		PEPROCESS process;
		status = PsLookupProcessByProcessId(ULongToHandle(data->ProcessId), &process);
		if (!NT_SUCCESS(status))
		{
			break;
		}
		HANDLE process_handle = NULL;
		status = ObOpenObjectByPointer(process, 0, NULL, 0, NULL, KernelMode, &process_handle);
		if (!NT_SUCCESS(status))
		{
			break;
		}
		status = ZwTerminateProcess(process_handle, STATUS_PROCESS_IS_TERMINATING);
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

extern "C"
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS status;
	int i;

	DriverObject->DriverUnload = DriverUnload;
	status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("[*]Terminator:CreateDevice Failed."));
		return status;
	}
	status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(DeviceObject);
		KdPrint(("[*]Terminator:CreateSymbolink Failed."));
		return status;
	}
	for ( i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = DispatchPassThru;
	}

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIOCTL;

	KdPrint(("[*]Terminator:DriverLoad Success."));
	return status;
}