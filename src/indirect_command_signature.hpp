
#pragma once

class indirect_command_signature
{
	std::vector<D3D12_INDIRECT_ARGUMENT_DESC> Parameters;
	size_t StrideInBytes = 0;

public:

	indirect_command_signature* PushCommandDraw()
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
		Parameters.push_back(Parameter);
		StrideInBytes += sizeof(D3D12_DRAW_ARGUMENTS);
		return this;
	}

	indirect_command_signature* PushCommandDrawIndexed()
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
		Parameters.push_back(Parameter);
		StrideInBytes += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
		return this;
	}

	indirect_command_signature* PushCommandDispatch()
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
		Parameters.push_back(Parameter);
		StrideInBytes += sizeof(D3D12_DISPATCH_ARGUMENTS);
		return this;
	}	

	indirect_command_signature* PushCommandDispatchMesh()
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
		Parameters.push_back(Parameter);
		StrideInBytes += sizeof(D3D12_DISPATCH_ARGUMENTS);
		return this;
	}	

	indirect_command_signature* PushCommandDispatchRays()
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS;
		Parameters.push_back(Parameter);
		StrideInBytes += sizeof(D3D12_DISPATCH_ARGUMENTS);
		return this;
	}	

	indirect_command_signature* PushShaderResource(u32 RootParameterIndex)
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
		Parameter.ShaderResourceView.RootParameterIndex = RootParameterIndex;
		Parameters.push_back(Parameter);
		StrideInBytes += 8;
		return this;
	}

	indirect_command_signature* PushConstantBuffer(u32 RootParameterIndex)
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
		Parameter.ConstantBufferView.RootParameterIndex = RootParameterIndex;
		Parameters.push_back(Parameter);
		StrideInBytes += 8;
		return this;
	}

	indirect_command_signature* PushConstant(u32 RootParameterIndex)
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		Parameter.Constant.RootParameterIndex = RootParameterIndex;
		Parameters.push_back(Parameter);
		StrideInBytes += sizeof(D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT); // ???
		return this;
	}

	indirect_command_signature* PushUnorderedAccess(u32 RootParameterIndex)
	{
		D3D12_INDIRECT_ARGUMENT_DESC Parameter = {};
		Parameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
		Parameter.UnorderedAccessView.RootParameterIndex = RootParameterIndex;
		Parameters.push_back(Parameter);
		StrideInBytes += 8;
		return this;
	}

	void Build(ID3D12Device1* Device, ID3D12RootSignature* RootSignature)
	{
		D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc = {};
		CommandSignatureDesc.pArgumentDescs = Parameters.data();
		CommandSignatureDesc.NumArgumentDescs = Parameters.size();
		CommandSignatureDesc.ByteStride = AlignUp(StrideInBytes, 8);
		Device->CreateCommandSignature(&CommandSignatureDesc, RootSignature, IID_PPV_ARGS(&Handle));
	}

	ComPtr<ID3D12CommandSignature> Handle;
};

