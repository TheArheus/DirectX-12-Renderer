
#pragma once

class shader_input
{
	std::vector<D3D12_ROOT_PARAMETER1> Parameters;
	std::vector<D3D12_STATIC_SAMPLER_DESC> Samplers;

	// TODO: Shader Visibility
public:
	shader_input* PushShaderResource(u32 Register, u32 Space, D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC)
	{
		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameter.Descriptor = {Register, Space, Flags};
		Parameters.push_back(Parameter);

		return this;
	}

	shader_input* PushConstantBuffer(u32 Register, u32 Space, D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC)
	{
		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameter.Descriptor = {Register, Space, Flags};
		Parameters.push_back(Parameter);

		return this;
	}

	shader_input* PushUnorderedAccess(u32 Register, u32 Space, D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC)
	{
		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameter.Descriptor = {Register, Space, Flags};
		Parameters.push_back(Parameter);

		return this;
	}

	shader_input* PushShaderResourceTable(u32 Start, u32 Count, u32 Space, D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC)
	{
		// NOTE: don't like this solution. 
		// Maybe there could be something better?
		D3D12_DESCRIPTOR_RANGE1* ParameterRange = new D3D12_DESCRIPTOR_RANGE1;
		ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ParameterRange->BaseShaderRegister = Start;
		ParameterRange->NumDescriptors = Count;
		ParameterRange->RegisterSpace = Space;
		ParameterRange->Flags = Flags;
		ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameter.DescriptorTable = {1, ParameterRange};

		Parameters.push_back(Parameter);

		return this;
	}

	shader_input* PushConstantBufferTable(u32 Start, u32 Count, u32 Space, D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC)
	{
		D3D12_DESCRIPTOR_RANGE1* ParameterRange = new D3D12_DESCRIPTOR_RANGE1;
		ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ParameterRange->BaseShaderRegister = Start;
		ParameterRange->NumDescriptors = Count;
		ParameterRange->RegisterSpace = Space;
		ParameterRange->Flags = Flags;
		ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameter.DescriptorTable = {1, ParameterRange};

		Parameters.push_back(Parameter);

		return this;
	}

	shader_input* PushUnorderedAccessTable(u32 Start, u32 Count, u32 Space, D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC)
	{
		D3D12_DESCRIPTOR_RANGE1* ParameterRange = new D3D12_DESCRIPTOR_RANGE1;
		ParameterRange->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		ParameterRange->BaseShaderRegister = Start;
		ParameterRange->NumDescriptors = Count;
		ParameterRange->RegisterSpace = Space;
		ParameterRange->Flags = Flags;
		ParameterRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER1 Parameter = {};
		Parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Parameter.DescriptorTable = {1, ParameterRange};

		Parameters.push_back(Parameter);

		return this;
	}

	shader_input* PushSampler(u32 Register, u32 Space, D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC)
	{
		D3D12_STATIC_SAMPLER_DESC Parameter = {};
		Parameter.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		Parameter.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Parameter.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Parameter.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Parameter.MipLODBias = 0;
		Parameter.MaxAnisotropy = 1;
		//Parameter.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		Parameter.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		Parameter.MinLOD = 0;
		Parameter.MaxLOD = 0;
		Parameter.ShaderRegister = Register;
		Parameter.RegisterSpace = Space;
		Parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		Samplers.push_back(Parameter);

		return this;
	}


	void Build(ID3D12Device1* Device, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE)
	{
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSignatureDesc;
		RootSignatureDesc.Init_1_1(Parameters.size(), Parameters.data(), Samplers.size(), Samplers.data(), Flags);

		ComPtr<ID3DBlob> Signature;
		ComPtr<ID3DBlob> Error;
		HRESULT Res = D3DX12SerializeVersionedRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &Signature, &Error);
		Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&Handle));
	}

	ComPtr<ID3D12RootSignature> Handle;
};

