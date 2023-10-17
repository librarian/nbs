// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.26.0
// 	protoc        v3.19.0
// source: cloud/blockstore/public/api/protos/mount.proto

package protos

import (
	protos "github.com/ydb-platform/nbs/cloud/storage/core/protos"
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	protoimpl "google.golang.org/protobuf/runtime/protoimpl"
	reflect "reflect"
	sync "sync"
)

const (
	// Verify that this generated code is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(20 - protoimpl.MinVersion)
	// Verify that runtime/protoimpl is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(protoimpl.MaxVersion - 20)
)

type EClientIpcType int32

const (
	EClientIpcType_IPC_GRPC  EClientIpcType = 0
	EClientIpcType_IPC_VHOST EClientIpcType = 1
	EClientIpcType_IPC_NBD   EClientIpcType = 2
	EClientIpcType_IPC_NVME  EClientIpcType = 3
	EClientIpcType_IPC_SCSI  EClientIpcType = 4
	EClientIpcType_IPC_RDMA  EClientIpcType = 5
)

// Enum value maps for EClientIpcType.
var (
	EClientIpcType_name = map[int32]string{
		0: "IPC_GRPC",
		1: "IPC_VHOST",
		2: "IPC_NBD",
		3: "IPC_NVME",
		4: "IPC_SCSI",
		5: "IPC_RDMA",
	}
	EClientIpcType_value = map[string]int32{
		"IPC_GRPC":  0,
		"IPC_VHOST": 1,
		"IPC_NBD":   2,
		"IPC_NVME":  3,
		"IPC_SCSI":  4,
		"IPC_RDMA":  5,
	}
)

func (x EClientIpcType) Enum() *EClientIpcType {
	p := new(EClientIpcType)
	*p = x
	return p
}

func (x EClientIpcType) String() string {
	return protoimpl.X.EnumStringOf(x.Descriptor(), protoreflect.EnumNumber(x))
}

func (EClientIpcType) Descriptor() protoreflect.EnumDescriptor {
	return file_cloud_blockstore_public_api_protos_mount_proto_enumTypes[0].Descriptor()
}

func (EClientIpcType) Type() protoreflect.EnumType {
	return &file_cloud_blockstore_public_api_protos_mount_proto_enumTypes[0]
}

func (x EClientIpcType) Number() protoreflect.EnumNumber {
	return protoreflect.EnumNumber(x)
}

// Deprecated: Use EClientIpcType.Descriptor instead.
func (EClientIpcType) EnumDescriptor() ([]byte, []int) {
	return file_cloud_blockstore_public_api_protos_mount_proto_rawDescGZIP(), []int{0}
}

type EMountFlag int32

const (
	EMountFlag_MF_NONE                EMountFlag = 0
	EMountFlag_MF_THROTTLING_DISABLED EMountFlag = 1
	EMountFlag_MF_FORCE_WRITE         EMountFlag = 2
	EMountFlag_MF_FILL                EMountFlag = 3 // TODO: NBS-4488. Remove this Mount flag.
)

// Enum value maps for EMountFlag.
var (
	EMountFlag_name = map[int32]string{
		0: "MF_NONE",
		1: "MF_THROTTLING_DISABLED",
		2: "MF_FORCE_WRITE",
		3: "MF_FILL",
	}
	EMountFlag_value = map[string]int32{
		"MF_NONE":                0,
		"MF_THROTTLING_DISABLED": 1,
		"MF_FORCE_WRITE":         2,
		"MF_FILL":                3,
	}
)

func (x EMountFlag) Enum() *EMountFlag {
	p := new(EMountFlag)
	*p = x
	return p
}

func (x EMountFlag) String() string {
	return protoimpl.X.EnumStringOf(x.Descriptor(), protoreflect.EnumNumber(x))
}

func (EMountFlag) Descriptor() protoreflect.EnumDescriptor {
	return file_cloud_blockstore_public_api_protos_mount_proto_enumTypes[1].Descriptor()
}

func (EMountFlag) Type() protoreflect.EnumType {
	return &file_cloud_blockstore_public_api_protos_mount_proto_enumTypes[1]
}

func (x EMountFlag) Number() protoreflect.EnumNumber {
	return protoreflect.EnumNumber(x)
}

// Deprecated: Use EMountFlag.Descriptor instead.
func (EMountFlag) EnumDescriptor() ([]byte, []int) {
	return file_cloud_blockstore_public_api_protos_mount_proto_rawDescGZIP(), []int{1}
}

type TMountVolumeRequest struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// Optional request headers.
	Headers *THeaders `protobuf:"bytes,1,opt,name=Headers,proto3" json:"Headers,omitempty"`
	// Label of volume to mount.
	DiskId string `protobuf:"bytes,2,opt,name=DiskId,proto3" json:"DiskId,omitempty"`
	// VM information.
	InstanceId string `protobuf:"bytes,3,opt,name=InstanceId,proto3" json:"InstanceId,omitempty"`
	// VM token.
	Token string `protobuf:"bytes,4,opt,name=Token,proto3" json:"Token,omitempty"`
	// Volume access mode.
	VolumeAccessMode EVolumeAccessMode `protobuf:"varint,6,opt,name=VolumeAccessMode,proto3,enum=NCloud.NBlockStore.NProto.EVolumeAccessMode" json:"VolumeAccessMode,omitempty"`
	// Volume mount mode.
	VolumeMountMode EVolumeMountMode `protobuf:"varint,7,opt,name=VolumeMountMode,proto3,enum=NCloud.NBlockStore.NProto.EVolumeMountMode" json:"VolumeMountMode,omitempty"`
	// IPC type used by client (only for monitoring).
	IpcType EClientIpcType `protobuf:"varint,8,opt,name=IpcType,proto3,enum=NCloud.NBlockStore.NProto.EClientIpcType" json:"IpcType,omitempty"`
	// Client version info.
	ClientVersionInfo string `protobuf:"bytes,10,opt,name=ClientVersionInfo,proto3" json:"ClientVersionInfo,omitempty"`
	// Obsolete, use MountFlags instead.
	ThrottlingDisabled bool `protobuf:"varint,11,opt,name=ThrottlingDisabled,proto3" json:"ThrottlingDisabled,omitempty"`
	// Volume generation.
	MountSeqNumber uint64 `protobuf:"varint,12,opt,name=MountSeqNumber,proto3" json:"MountSeqNumber,omitempty"`
	// Mount flags.
	MountFlags uint32 `protobuf:"varint,14,opt,name=MountFlags,proto3" json:"MountFlags,omitempty"`
	// Encryption spec.
	EncryptionSpec *TEncryptionSpec `protobuf:"bytes,15,opt,name=EncryptionSpec,proto3" json:"EncryptionSpec,omitempty"`
	// Mount sequential number for disk filling.
	// We don't allow clients with old sequential number to mount disk for read/write
	// in order to prevent data corruption during disk filling.
	FillSeqNumber uint64 `protobuf:"varint,16,opt,name=FillSeqNumber,proto3" json:"FillSeqNumber,omitempty"`
	// We don't allow clients with wrong fill generation to mount disk for read/write
	// in order to prevent data corruption during disk filling.
	FillGeneration uint64 `protobuf:"varint,17,opt,name=FillGeneration,proto3" json:"FillGeneration,omitempty"`
}

func (x *TMountVolumeRequest) Reset() {
	*x = TMountVolumeRequest{}
	if protoimpl.UnsafeEnabled {
		mi := &file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *TMountVolumeRequest) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*TMountVolumeRequest) ProtoMessage() {}

func (x *TMountVolumeRequest) ProtoReflect() protoreflect.Message {
	mi := &file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use TMountVolumeRequest.ProtoReflect.Descriptor instead.
func (*TMountVolumeRequest) Descriptor() ([]byte, []int) {
	return file_cloud_blockstore_public_api_protos_mount_proto_rawDescGZIP(), []int{0}
}

func (x *TMountVolumeRequest) GetHeaders() *THeaders {
	if x != nil {
		return x.Headers
	}
	return nil
}

func (x *TMountVolumeRequest) GetDiskId() string {
	if x != nil {
		return x.DiskId
	}
	return ""
}

func (x *TMountVolumeRequest) GetInstanceId() string {
	if x != nil {
		return x.InstanceId
	}
	return ""
}

func (x *TMountVolumeRequest) GetToken() string {
	if x != nil {
		return x.Token
	}
	return ""
}

func (x *TMountVolumeRequest) GetVolumeAccessMode() EVolumeAccessMode {
	if x != nil {
		return x.VolumeAccessMode
	}
	return EVolumeAccessMode_VOLUME_ACCESS_READ_WRITE
}

func (x *TMountVolumeRequest) GetVolumeMountMode() EVolumeMountMode {
	if x != nil {
		return x.VolumeMountMode
	}
	return EVolumeMountMode_VOLUME_MOUNT_LOCAL
}

func (x *TMountVolumeRequest) GetIpcType() EClientIpcType {
	if x != nil {
		return x.IpcType
	}
	return EClientIpcType_IPC_GRPC
}

func (x *TMountVolumeRequest) GetClientVersionInfo() string {
	if x != nil {
		return x.ClientVersionInfo
	}
	return ""
}

func (x *TMountVolumeRequest) GetThrottlingDisabled() bool {
	if x != nil {
		return x.ThrottlingDisabled
	}
	return false
}

func (x *TMountVolumeRequest) GetMountSeqNumber() uint64 {
	if x != nil {
		return x.MountSeqNumber
	}
	return 0
}

func (x *TMountVolumeRequest) GetMountFlags() uint32 {
	if x != nil {
		return x.MountFlags
	}
	return 0
}

func (x *TMountVolumeRequest) GetEncryptionSpec() *TEncryptionSpec {
	if x != nil {
		return x.EncryptionSpec
	}
	return nil
}

func (x *TMountVolumeRequest) GetFillSeqNumber() uint64 {
	if x != nil {
		return x.FillSeqNumber
	}
	return 0
}

func (x *TMountVolumeRequest) GetFillGeneration() uint64 {
	if x != nil {
		return x.FillGeneration
	}
	return 0
}

type TMountVolumeResponse struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// Optional error, set only if error happened.
	Error *protos.TError `protobuf:"bytes,1,opt,name=Error,proto3" json:"Error,omitempty"`
	// Volume information.
	Volume *TVolume `protobuf:"bytes,2,opt,name=Volume,proto3" json:"Volume,omitempty"`
	// Session identifier.
	SessionId string `protobuf:"bytes,3,opt,name=SessionId,proto3" json:"SessionId,omitempty"`
	// Inactive clients timeout (in milliseconds).
	InactiveClientsTimeout uint32 `protobuf:"varint,4,opt,name=InactiveClientsTimeout,proto3" json:"InactiveClientsTimeout,omitempty"`
	// Service version information.
	ServiceVersionInfo string `protobuf:"bytes,5,opt,name=ServiceVersionInfo,proto3" json:"ServiceVersionInfo,omitempty"`
}

func (x *TMountVolumeResponse) Reset() {
	*x = TMountVolumeResponse{}
	if protoimpl.UnsafeEnabled {
		mi := &file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *TMountVolumeResponse) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*TMountVolumeResponse) ProtoMessage() {}

func (x *TMountVolumeResponse) ProtoReflect() protoreflect.Message {
	mi := &file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use TMountVolumeResponse.ProtoReflect.Descriptor instead.
func (*TMountVolumeResponse) Descriptor() ([]byte, []int) {
	return file_cloud_blockstore_public_api_protos_mount_proto_rawDescGZIP(), []int{1}
}

func (x *TMountVolumeResponse) GetError() *protos.TError {
	if x != nil {
		return x.Error
	}
	return nil
}

func (x *TMountVolumeResponse) GetVolume() *TVolume {
	if x != nil {
		return x.Volume
	}
	return nil
}

func (x *TMountVolumeResponse) GetSessionId() string {
	if x != nil {
		return x.SessionId
	}
	return ""
}

func (x *TMountVolumeResponse) GetInactiveClientsTimeout() uint32 {
	if x != nil {
		return x.InactiveClientsTimeout
	}
	return 0
}

func (x *TMountVolumeResponse) GetServiceVersionInfo() string {
	if x != nil {
		return x.ServiceVersionInfo
	}
	return ""
}

type TUnmountVolumeRequest struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// Optional request headers.
	Headers *THeaders `protobuf:"bytes,1,opt,name=Headers,proto3" json:"Headers,omitempty"`
	// Label of volume to unmount.
	DiskId string `protobuf:"bytes,2,opt,name=DiskId,proto3" json:"DiskId,omitempty"`
	// VM information.
	InstanceId string `protobuf:"bytes,3,opt,name=InstanceId,proto3" json:"InstanceId,omitempty"`
	// VM token.
	Token string `protobuf:"bytes,4,opt,name=Token,proto3" json:"Token,omitempty"`
	// If unmount shall ignore pending operations. Do we need it?
	Force bool `protobuf:"varint,5,opt,name=Force,proto3" json:"Force,omitempty"`
	// Session identifier.
	SessionId string `protobuf:"bytes,6,opt,name=SessionId,proto3" json:"SessionId,omitempty"`
}

func (x *TUnmountVolumeRequest) Reset() {
	*x = TUnmountVolumeRequest{}
	if protoimpl.UnsafeEnabled {
		mi := &file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[2]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *TUnmountVolumeRequest) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*TUnmountVolumeRequest) ProtoMessage() {}

func (x *TUnmountVolumeRequest) ProtoReflect() protoreflect.Message {
	mi := &file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[2]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use TUnmountVolumeRequest.ProtoReflect.Descriptor instead.
func (*TUnmountVolumeRequest) Descriptor() ([]byte, []int) {
	return file_cloud_blockstore_public_api_protos_mount_proto_rawDescGZIP(), []int{2}
}

func (x *TUnmountVolumeRequest) GetHeaders() *THeaders {
	if x != nil {
		return x.Headers
	}
	return nil
}

func (x *TUnmountVolumeRequest) GetDiskId() string {
	if x != nil {
		return x.DiskId
	}
	return ""
}

func (x *TUnmountVolumeRequest) GetInstanceId() string {
	if x != nil {
		return x.InstanceId
	}
	return ""
}

func (x *TUnmountVolumeRequest) GetToken() string {
	if x != nil {
		return x.Token
	}
	return ""
}

func (x *TUnmountVolumeRequest) GetForce() bool {
	if x != nil {
		return x.Force
	}
	return false
}

func (x *TUnmountVolumeRequest) GetSessionId() string {
	if x != nil {
		return x.SessionId
	}
	return ""
}

type TUnmountVolumeResponse struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// Optional error, set only if error happened.
	Error *protos.TError `protobuf:"bytes,1,opt,name=Error,proto3" json:"Error,omitempty"`
}

func (x *TUnmountVolumeResponse) Reset() {
	*x = TUnmountVolumeResponse{}
	if protoimpl.UnsafeEnabled {
		mi := &file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[3]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *TUnmountVolumeResponse) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*TUnmountVolumeResponse) ProtoMessage() {}

func (x *TUnmountVolumeResponse) ProtoReflect() protoreflect.Message {
	mi := &file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[3]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use TUnmountVolumeResponse.ProtoReflect.Descriptor instead.
func (*TUnmountVolumeResponse) Descriptor() ([]byte, []int) {
	return file_cloud_blockstore_public_api_protos_mount_proto_rawDescGZIP(), []int{3}
}

func (x *TUnmountVolumeResponse) GetError() *protos.TError {
	if x != nil {
		return x.Error
	}
	return nil
}

var File_cloud_blockstore_public_api_protos_mount_proto protoreflect.FileDescriptor

var file_cloud_blockstore_public_api_protos_mount_proto_rawDesc = []byte{
	0x0a, 0x2e, 0x63, 0x6c, 0x6f, 0x75, 0x64, 0x2f, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x73, 0x74, 0x6f,
	0x72, 0x65, 0x2f, 0x70, 0x75, 0x62, 0x6c, 0x69, 0x63, 0x2f, 0x61, 0x70, 0x69, 0x2f, 0x70, 0x72,
	0x6f, 0x74, 0x6f, 0x73, 0x2f, 0x6d, 0x6f, 0x75, 0x6e, 0x74, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f,
	0x12, 0x19, 0x4e, 0x43, 0x6c, 0x6f, 0x75, 0x64, 0x2e, 0x4e, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x53,
	0x74, 0x6f, 0x72, 0x65, 0x2e, 0x4e, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x1a, 0x33, 0x63, 0x6c, 0x6f,
	0x75, 0x64, 0x2f, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x73, 0x74, 0x6f, 0x72, 0x65, 0x2f, 0x70, 0x75,
	0x62, 0x6c, 0x69, 0x63, 0x2f, 0x61, 0x70, 0x69, 0x2f, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2f,
	0x65, 0x6e, 0x63, 0x72, 0x79, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f,
	0x1a, 0x30, 0x63, 0x6c, 0x6f, 0x75, 0x64, 0x2f, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x73, 0x74, 0x6f,
	0x72, 0x65, 0x2f, 0x70, 0x75, 0x62, 0x6c, 0x69, 0x63, 0x2f, 0x61, 0x70, 0x69, 0x2f, 0x70, 0x72,
	0x6f, 0x74, 0x6f, 0x73, 0x2f, 0x68, 0x65, 0x61, 0x64, 0x65, 0x72, 0x73, 0x2e, 0x70, 0x72, 0x6f,
	0x74, 0x6f, 0x1a, 0x2f, 0x63, 0x6c, 0x6f, 0x75, 0x64, 0x2f, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x73,
	0x74, 0x6f, 0x72, 0x65, 0x2f, 0x70, 0x75, 0x62, 0x6c, 0x69, 0x63, 0x2f, 0x61, 0x70, 0x69, 0x2f,
	0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2f, 0x76, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x2e, 0x70, 0x72,
	0x6f, 0x74, 0x6f, 0x1a, 0x25, 0x63, 0x6c, 0x6f, 0x75, 0x64, 0x2f, 0x73, 0x74, 0x6f, 0x72, 0x61,
	0x67, 0x65, 0x2f, 0x63, 0x6f, 0x72, 0x65, 0x2f, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2f, 0x65,
	0x72, 0x72, 0x6f, 0x72, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x22, 0xe6, 0x05, 0x0a, 0x13, 0x54,
	0x4d, 0x6f, 0x75, 0x6e, 0x74, 0x56, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x52, 0x65, 0x71, 0x75, 0x65,
	0x73, 0x74, 0x12, 0x3d, 0x0a, 0x07, 0x48, 0x65, 0x61, 0x64, 0x65, 0x72, 0x73, 0x18, 0x01, 0x20,
	0x01, 0x28, 0x0b, 0x32, 0x23, 0x2e, 0x4e, 0x43, 0x6c, 0x6f, 0x75, 0x64, 0x2e, 0x4e, 0x42, 0x6c,
	0x6f, 0x63, 0x6b, 0x53, 0x74, 0x6f, 0x72, 0x65, 0x2e, 0x4e, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x2e,
	0x54, 0x48, 0x65, 0x61, 0x64, 0x65, 0x72, 0x73, 0x52, 0x07, 0x48, 0x65, 0x61, 0x64, 0x65, 0x72,
	0x73, 0x12, 0x16, 0x0a, 0x06, 0x44, 0x69, 0x73, 0x6b, 0x49, 0x64, 0x18, 0x02, 0x20, 0x01, 0x28,
	0x09, 0x52, 0x06, 0x44, 0x69, 0x73, 0x6b, 0x49, 0x64, 0x12, 0x1e, 0x0a, 0x0a, 0x49, 0x6e, 0x73,
	0x74, 0x61, 0x6e, 0x63, 0x65, 0x49, 0x64, 0x18, 0x03, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0a, 0x49,
	0x6e, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x49, 0x64, 0x12, 0x14, 0x0a, 0x05, 0x54, 0x6f, 0x6b,
	0x65, 0x6e, 0x18, 0x04, 0x20, 0x01, 0x28, 0x09, 0x52, 0x05, 0x54, 0x6f, 0x6b, 0x65, 0x6e, 0x12,
	0x58, 0x0a, 0x10, 0x56, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x41, 0x63, 0x63, 0x65, 0x73, 0x73, 0x4d,
	0x6f, 0x64, 0x65, 0x18, 0x06, 0x20, 0x01, 0x28, 0x0e, 0x32, 0x2c, 0x2e, 0x4e, 0x43, 0x6c, 0x6f,
	0x75, 0x64, 0x2e, 0x4e, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x53, 0x74, 0x6f, 0x72, 0x65, 0x2e, 0x4e,
	0x50, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x45, 0x56, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x41, 0x63, 0x63,
	0x65, 0x73, 0x73, 0x4d, 0x6f, 0x64, 0x65, 0x52, 0x10, 0x56, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x41,
	0x63, 0x63, 0x65, 0x73, 0x73, 0x4d, 0x6f, 0x64, 0x65, 0x12, 0x55, 0x0a, 0x0f, 0x56, 0x6f, 0x6c,
	0x75, 0x6d, 0x65, 0x4d, 0x6f, 0x75, 0x6e, 0x74, 0x4d, 0x6f, 0x64, 0x65, 0x18, 0x07, 0x20, 0x01,
	0x28, 0x0e, 0x32, 0x2b, 0x2e, 0x4e, 0x43, 0x6c, 0x6f, 0x75, 0x64, 0x2e, 0x4e, 0x42, 0x6c, 0x6f,
	0x63, 0x6b, 0x53, 0x74, 0x6f, 0x72, 0x65, 0x2e, 0x4e, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x45,
	0x56, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x4d, 0x6f, 0x75, 0x6e, 0x74, 0x4d, 0x6f, 0x64, 0x65, 0x52,
	0x0f, 0x56, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x4d, 0x6f, 0x75, 0x6e, 0x74, 0x4d, 0x6f, 0x64, 0x65,
	0x12, 0x43, 0x0a, 0x07, 0x49, 0x70, 0x63, 0x54, 0x79, 0x70, 0x65, 0x18, 0x08, 0x20, 0x01, 0x28,
	0x0e, 0x32, 0x29, 0x2e, 0x4e, 0x43, 0x6c, 0x6f, 0x75, 0x64, 0x2e, 0x4e, 0x42, 0x6c, 0x6f, 0x63,
	0x6b, 0x53, 0x74, 0x6f, 0x72, 0x65, 0x2e, 0x4e, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x45, 0x43,
	0x6c, 0x69, 0x65, 0x6e, 0x74, 0x49, 0x70, 0x63, 0x54, 0x79, 0x70, 0x65, 0x52, 0x07, 0x49, 0x70,
	0x63, 0x54, 0x79, 0x70, 0x65, 0x12, 0x2c, 0x0a, 0x11, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x56,
	0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x49, 0x6e, 0x66, 0x6f, 0x18, 0x0a, 0x20, 0x01, 0x28, 0x09,
	0x52, 0x11, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x49,
	0x6e, 0x66, 0x6f, 0x12, 0x2e, 0x0a, 0x12, 0x54, 0x68, 0x72, 0x6f, 0x74, 0x74, 0x6c, 0x69, 0x6e,
	0x67, 0x44, 0x69, 0x73, 0x61, 0x62, 0x6c, 0x65, 0x64, 0x18, 0x0b, 0x20, 0x01, 0x28, 0x08, 0x52,
	0x12, 0x54, 0x68, 0x72, 0x6f, 0x74, 0x74, 0x6c, 0x69, 0x6e, 0x67, 0x44, 0x69, 0x73, 0x61, 0x62,
	0x6c, 0x65, 0x64, 0x12, 0x26, 0x0a, 0x0e, 0x4d, 0x6f, 0x75, 0x6e, 0x74, 0x53, 0x65, 0x71, 0x4e,
	0x75, 0x6d, 0x62, 0x65, 0x72, 0x18, 0x0c, 0x20, 0x01, 0x28, 0x04, 0x52, 0x0e, 0x4d, 0x6f, 0x75,
	0x6e, 0x74, 0x53, 0x65, 0x71, 0x4e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x12, 0x1e, 0x0a, 0x0a, 0x4d,
	0x6f, 0x75, 0x6e, 0x74, 0x46, 0x6c, 0x61, 0x67, 0x73, 0x18, 0x0e, 0x20, 0x01, 0x28, 0x0d, 0x52,
	0x0a, 0x4d, 0x6f, 0x75, 0x6e, 0x74, 0x46, 0x6c, 0x61, 0x67, 0x73, 0x12, 0x52, 0x0a, 0x0e, 0x45,
	0x6e, 0x63, 0x72, 0x79, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x53, 0x70, 0x65, 0x63, 0x18, 0x0f, 0x20,
	0x01, 0x28, 0x0b, 0x32, 0x2a, 0x2e, 0x4e, 0x43, 0x6c, 0x6f, 0x75, 0x64, 0x2e, 0x4e, 0x42, 0x6c,
	0x6f, 0x63, 0x6b, 0x53, 0x74, 0x6f, 0x72, 0x65, 0x2e, 0x4e, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x2e,
	0x54, 0x45, 0x6e, 0x63, 0x72, 0x79, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x53, 0x70, 0x65, 0x63, 0x52,
	0x0e, 0x45, 0x6e, 0x63, 0x72, 0x79, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x53, 0x70, 0x65, 0x63, 0x12,
	0x24, 0x0a, 0x0d, 0x46, 0x69, 0x6c, 0x6c, 0x53, 0x65, 0x71, 0x4e, 0x75, 0x6d, 0x62, 0x65, 0x72,
	0x18, 0x10, 0x20, 0x01, 0x28, 0x04, 0x52, 0x0d, 0x46, 0x69, 0x6c, 0x6c, 0x53, 0x65, 0x71, 0x4e,
	0x75, 0x6d, 0x62, 0x65, 0x72, 0x12, 0x26, 0x0a, 0x0e, 0x46, 0x69, 0x6c, 0x6c, 0x47, 0x65, 0x6e,
	0x65, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x18, 0x11, 0x20, 0x01, 0x28, 0x04, 0x52, 0x0e, 0x46,
	0x69, 0x6c, 0x6c, 0x47, 0x65, 0x6e, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x4a, 0x04, 0x08,
	0x0d, 0x10, 0x0e, 0x22, 0x85, 0x02, 0x0a, 0x14, 0x54, 0x4d, 0x6f, 0x75, 0x6e, 0x74, 0x56, 0x6f,
	0x6c, 0x75, 0x6d, 0x65, 0x52, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65, 0x12, 0x2b, 0x0a, 0x05,
	0x45, 0x72, 0x72, 0x6f, 0x72, 0x18, 0x01, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x15, 0x2e, 0x4e, 0x43,
	0x6c, 0x6f, 0x75, 0x64, 0x2e, 0x4e, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x54, 0x45, 0x72, 0x72,
	0x6f, 0x72, 0x52, 0x05, 0x45, 0x72, 0x72, 0x6f, 0x72, 0x12, 0x3a, 0x0a, 0x06, 0x56, 0x6f, 0x6c,
	0x75, 0x6d, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x22, 0x2e, 0x4e, 0x43, 0x6c, 0x6f,
	0x75, 0x64, 0x2e, 0x4e, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x53, 0x74, 0x6f, 0x72, 0x65, 0x2e, 0x4e,
	0x50, 0x72, 0x6f, 0x74, 0x6f, 0x2e, 0x54, 0x56, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x52, 0x06, 0x56,
	0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x12, 0x1c, 0x0a, 0x09, 0x53, 0x65, 0x73, 0x73, 0x69, 0x6f, 0x6e,
	0x49, 0x64, 0x18, 0x03, 0x20, 0x01, 0x28, 0x09, 0x52, 0x09, 0x53, 0x65, 0x73, 0x73, 0x69, 0x6f,
	0x6e, 0x49, 0x64, 0x12, 0x36, 0x0a, 0x16, 0x49, 0x6e, 0x61, 0x63, 0x74, 0x69, 0x76, 0x65, 0x43,
	0x6c, 0x69, 0x65, 0x6e, 0x74, 0x73, 0x54, 0x69, 0x6d, 0x65, 0x6f, 0x75, 0x74, 0x18, 0x04, 0x20,
	0x01, 0x28, 0x0d, 0x52, 0x16, 0x49, 0x6e, 0x61, 0x63, 0x74, 0x69, 0x76, 0x65, 0x43, 0x6c, 0x69,
	0x65, 0x6e, 0x74, 0x73, 0x54, 0x69, 0x6d, 0x65, 0x6f, 0x75, 0x74, 0x12, 0x2e, 0x0a, 0x12, 0x53,
	0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x49, 0x6e, 0x66,
	0x6f, 0x18, 0x05, 0x20, 0x01, 0x28, 0x09, 0x52, 0x12, 0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65,
	0x56, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x49, 0x6e, 0x66, 0x6f, 0x22, 0xd8, 0x01, 0x0a, 0x15,
	0x54, 0x55, 0x6e, 0x6d, 0x6f, 0x75, 0x6e, 0x74, 0x56, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x52, 0x65,
	0x71, 0x75, 0x65, 0x73, 0x74, 0x12, 0x3d, 0x0a, 0x07, 0x48, 0x65, 0x61, 0x64, 0x65, 0x72, 0x73,
	0x18, 0x01, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x23, 0x2e, 0x4e, 0x43, 0x6c, 0x6f, 0x75, 0x64, 0x2e,
	0x4e, 0x42, 0x6c, 0x6f, 0x63, 0x6b, 0x53, 0x74, 0x6f, 0x72, 0x65, 0x2e, 0x4e, 0x50, 0x72, 0x6f,
	0x74, 0x6f, 0x2e, 0x54, 0x48, 0x65, 0x61, 0x64, 0x65, 0x72, 0x73, 0x52, 0x07, 0x48, 0x65, 0x61,
	0x64, 0x65, 0x72, 0x73, 0x12, 0x16, 0x0a, 0x06, 0x44, 0x69, 0x73, 0x6b, 0x49, 0x64, 0x18, 0x02,
	0x20, 0x01, 0x28, 0x09, 0x52, 0x06, 0x44, 0x69, 0x73, 0x6b, 0x49, 0x64, 0x12, 0x1e, 0x0a, 0x0a,
	0x49, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x49, 0x64, 0x18, 0x03, 0x20, 0x01, 0x28, 0x09,
	0x52, 0x0a, 0x49, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x49, 0x64, 0x12, 0x14, 0x0a, 0x05,
	0x54, 0x6f, 0x6b, 0x65, 0x6e, 0x18, 0x04, 0x20, 0x01, 0x28, 0x09, 0x52, 0x05, 0x54, 0x6f, 0x6b,
	0x65, 0x6e, 0x12, 0x14, 0x0a, 0x05, 0x46, 0x6f, 0x72, 0x63, 0x65, 0x18, 0x05, 0x20, 0x01, 0x28,
	0x08, 0x52, 0x05, 0x46, 0x6f, 0x72, 0x63, 0x65, 0x12, 0x1c, 0x0a, 0x09, 0x53, 0x65, 0x73, 0x73,
	0x69, 0x6f, 0x6e, 0x49, 0x64, 0x18, 0x06, 0x20, 0x01, 0x28, 0x09, 0x52, 0x09, 0x53, 0x65, 0x73,
	0x73, 0x69, 0x6f, 0x6e, 0x49, 0x64, 0x22, 0x45, 0x0a, 0x16, 0x54, 0x55, 0x6e, 0x6d, 0x6f, 0x75,
	0x6e, 0x74, 0x56, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x52, 0x65, 0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65,
	0x12, 0x2b, 0x0a, 0x05, 0x45, 0x72, 0x72, 0x6f, 0x72, 0x18, 0x01, 0x20, 0x01, 0x28, 0x0b, 0x32,
	0x15, 0x2e, 0x4e, 0x43, 0x6c, 0x6f, 0x75, 0x64, 0x2e, 0x4e, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x2e,
	0x54, 0x45, 0x72, 0x72, 0x6f, 0x72, 0x52, 0x05, 0x45, 0x72, 0x72, 0x6f, 0x72, 0x2a, 0x64, 0x0a,
	0x0e, 0x45, 0x43, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x49, 0x70, 0x63, 0x54, 0x79, 0x70, 0x65, 0x12,
	0x0c, 0x0a, 0x08, 0x49, 0x50, 0x43, 0x5f, 0x47, 0x52, 0x50, 0x43, 0x10, 0x00, 0x12, 0x0d, 0x0a,
	0x09, 0x49, 0x50, 0x43, 0x5f, 0x56, 0x48, 0x4f, 0x53, 0x54, 0x10, 0x01, 0x12, 0x0b, 0x0a, 0x07,
	0x49, 0x50, 0x43, 0x5f, 0x4e, 0x42, 0x44, 0x10, 0x02, 0x12, 0x0c, 0x0a, 0x08, 0x49, 0x50, 0x43,
	0x5f, 0x4e, 0x56, 0x4d, 0x45, 0x10, 0x03, 0x12, 0x0c, 0x0a, 0x08, 0x49, 0x50, 0x43, 0x5f, 0x53,
	0x43, 0x53, 0x49, 0x10, 0x04, 0x12, 0x0c, 0x0a, 0x08, 0x49, 0x50, 0x43, 0x5f, 0x52, 0x44, 0x4d,
	0x41, 0x10, 0x05, 0x2a, 0x56, 0x0a, 0x0a, 0x45, 0x4d, 0x6f, 0x75, 0x6e, 0x74, 0x46, 0x6c, 0x61,
	0x67, 0x12, 0x0b, 0x0a, 0x07, 0x4d, 0x46, 0x5f, 0x4e, 0x4f, 0x4e, 0x45, 0x10, 0x00, 0x12, 0x1a,
	0x0a, 0x16, 0x4d, 0x46, 0x5f, 0x54, 0x48, 0x52, 0x4f, 0x54, 0x54, 0x4c, 0x49, 0x4e, 0x47, 0x5f,
	0x44, 0x49, 0x53, 0x41, 0x42, 0x4c, 0x45, 0x44, 0x10, 0x01, 0x12, 0x12, 0x0a, 0x0e, 0x4d, 0x46,
	0x5f, 0x46, 0x4f, 0x52, 0x43, 0x45, 0x5f, 0x57, 0x52, 0x49, 0x54, 0x45, 0x10, 0x02, 0x12, 0x0b,
	0x0a, 0x07, 0x4d, 0x46, 0x5f, 0x46, 0x49, 0x4c, 0x4c, 0x10, 0x03, 0x42, 0x35, 0x5a, 0x33, 0x61,
	0x2e, 0x79, 0x61, 0x6e, 0x64, 0x65, 0x78, 0x2d, 0x74, 0x65, 0x61, 0x6d, 0x2e, 0x72, 0x75, 0x2f,
	0x63, 0x6c, 0x6f, 0x75, 0x64, 0x2f, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x73, 0x74, 0x6f, 0x72, 0x65,
	0x2f, 0x70, 0x75, 0x62, 0x6c, 0x69, 0x63, 0x2f, 0x61, 0x70, 0x69, 0x2f, 0x70, 0x72, 0x6f, 0x74,
	0x6f, 0x73, 0x62, 0x06, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x33,
}

var (
	file_cloud_blockstore_public_api_protos_mount_proto_rawDescOnce sync.Once
	file_cloud_blockstore_public_api_protos_mount_proto_rawDescData = file_cloud_blockstore_public_api_protos_mount_proto_rawDesc
)

func file_cloud_blockstore_public_api_protos_mount_proto_rawDescGZIP() []byte {
	file_cloud_blockstore_public_api_protos_mount_proto_rawDescOnce.Do(func() {
		file_cloud_blockstore_public_api_protos_mount_proto_rawDescData = protoimpl.X.CompressGZIP(file_cloud_blockstore_public_api_protos_mount_proto_rawDescData)
	})
	return file_cloud_blockstore_public_api_protos_mount_proto_rawDescData
}

var file_cloud_blockstore_public_api_protos_mount_proto_enumTypes = make([]protoimpl.EnumInfo, 2)
var file_cloud_blockstore_public_api_protos_mount_proto_msgTypes = make([]protoimpl.MessageInfo, 4)
var file_cloud_blockstore_public_api_protos_mount_proto_goTypes = []interface{}{
	(EClientIpcType)(0),            // 0: NCloud.NBlockStore.NProto.EClientIpcType
	(EMountFlag)(0),                // 1: NCloud.NBlockStore.NProto.EMountFlag
	(*TMountVolumeRequest)(nil),    // 2: NCloud.NBlockStore.NProto.TMountVolumeRequest
	(*TMountVolumeResponse)(nil),   // 3: NCloud.NBlockStore.NProto.TMountVolumeResponse
	(*TUnmountVolumeRequest)(nil),  // 4: NCloud.NBlockStore.NProto.TUnmountVolumeRequest
	(*TUnmountVolumeResponse)(nil), // 5: NCloud.NBlockStore.NProto.TUnmountVolumeResponse
	(*THeaders)(nil),               // 6: NCloud.NBlockStore.NProto.THeaders
	(EVolumeAccessMode)(0),         // 7: NCloud.NBlockStore.NProto.EVolumeAccessMode
	(EVolumeMountMode)(0),          // 8: NCloud.NBlockStore.NProto.EVolumeMountMode
	(*TEncryptionSpec)(nil),        // 9: NCloud.NBlockStore.NProto.TEncryptionSpec
	(*protos.TError)(nil),          // 10: NCloud.NProto.TError
	(*TVolume)(nil),                // 11: NCloud.NBlockStore.NProto.TVolume
}
var file_cloud_blockstore_public_api_protos_mount_proto_depIdxs = []int32{
	6,  // 0: NCloud.NBlockStore.NProto.TMountVolumeRequest.Headers:type_name -> NCloud.NBlockStore.NProto.THeaders
	7,  // 1: NCloud.NBlockStore.NProto.TMountVolumeRequest.VolumeAccessMode:type_name -> NCloud.NBlockStore.NProto.EVolumeAccessMode
	8,  // 2: NCloud.NBlockStore.NProto.TMountVolumeRequest.VolumeMountMode:type_name -> NCloud.NBlockStore.NProto.EVolumeMountMode
	0,  // 3: NCloud.NBlockStore.NProto.TMountVolumeRequest.IpcType:type_name -> NCloud.NBlockStore.NProto.EClientIpcType
	9,  // 4: NCloud.NBlockStore.NProto.TMountVolumeRequest.EncryptionSpec:type_name -> NCloud.NBlockStore.NProto.TEncryptionSpec
	10, // 5: NCloud.NBlockStore.NProto.TMountVolumeResponse.Error:type_name -> NCloud.NProto.TError
	11, // 6: NCloud.NBlockStore.NProto.TMountVolumeResponse.Volume:type_name -> NCloud.NBlockStore.NProto.TVolume
	6,  // 7: NCloud.NBlockStore.NProto.TUnmountVolumeRequest.Headers:type_name -> NCloud.NBlockStore.NProto.THeaders
	10, // 8: NCloud.NBlockStore.NProto.TUnmountVolumeResponse.Error:type_name -> NCloud.NProto.TError
	9,  // [9:9] is the sub-list for method output_type
	9,  // [9:9] is the sub-list for method input_type
	9,  // [9:9] is the sub-list for extension type_name
	9,  // [9:9] is the sub-list for extension extendee
	0,  // [0:9] is the sub-list for field type_name
}

func init() { file_cloud_blockstore_public_api_protos_mount_proto_init() }
func file_cloud_blockstore_public_api_protos_mount_proto_init() {
	if File_cloud_blockstore_public_api_protos_mount_proto != nil {
		return
	}
	file_cloud_blockstore_public_api_protos_encryption_proto_init()
	file_cloud_blockstore_public_api_protos_headers_proto_init()
	file_cloud_blockstore_public_api_protos_volume_proto_init()
	if !protoimpl.UnsafeEnabled {
		file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*TMountVolumeRequest); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*TMountVolumeResponse); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[2].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*TUnmountVolumeRequest); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_cloud_blockstore_public_api_protos_mount_proto_msgTypes[3].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*TUnmountVolumeResponse); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
	}
	type x struct{}
	out := protoimpl.TypeBuilder{
		File: protoimpl.DescBuilder{
			GoPackagePath: reflect.TypeOf(x{}).PkgPath(),
			RawDescriptor: file_cloud_blockstore_public_api_protos_mount_proto_rawDesc,
			NumEnums:      2,
			NumMessages:   4,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_cloud_blockstore_public_api_protos_mount_proto_goTypes,
		DependencyIndexes: file_cloud_blockstore_public_api_protos_mount_proto_depIdxs,
		EnumInfos:         file_cloud_blockstore_public_api_protos_mount_proto_enumTypes,
		MessageInfos:      file_cloud_blockstore_public_api_protos_mount_proto_msgTypes,
	}.Build()
	File_cloud_blockstore_public_api_protos_mount_proto = out.File
	file_cloud_blockstore_public_api_protos_mount_proto_rawDesc = nil
	file_cloud_blockstore_public_api_protos_mount_proto_goTypes = nil
	file_cloud_blockstore_public_api_protos_mount_proto_depIdxs = nil
}
