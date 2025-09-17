
internal ImageAccessInfo SyncGetSrcImageAccessInfo(ImageAccessType access)
{
	ImageAccessInfo info = {0};
	
	switch (access) {
	case ImageAccessType_Undefined:
		info.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		info.stage  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case ImageAccessType_General:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
	case ImageAccessType_GraphicsRead:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case ImageAccessType_GraphicsReadWrite:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case ImageAccessType_ComputeRead:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case ImageAccessType_ComputeReadWrite:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case ImageAccessType_ColourWrite:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		info.access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case ImageAccessType_DepthWrite:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		info.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case ImageAccessType_TransferSrc:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case ImageAccessType_TransferDst:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		break;
	case ImageAccessType_Present:
		info.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		info.stage  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	}

	return info;
}

internal ImageAccessInfo SyncGetDstImageAccessInfo(ImageAccessType access)
{
	ImageAccessInfo info = {0};
	
	switch (access) {
	case ImageAccessType_Undefined:
		info.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		info.stage  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case ImageAccessType_General:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
	case ImageAccessType_GraphicsRead:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT;
		break;
	case ImageAccessType_GraphicsReadWrite:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case ImageAccessType_ComputeRead:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT;
		break;
	case ImageAccessType_ComputeReadWrite:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case ImageAccessType_ColourWrite:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		info.access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case ImageAccessType_DepthWrite:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		info.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case ImageAccessType_TransferSrc:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
		break;
	case ImageAccessType_TransferDst:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		break;
	case ImageAccessType_Present:
		info.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		info.stage  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	}

	return info;
}

internal GPUBufferAccessInfo SyncGetSrcBufferAccessInfo(GPUBufferAccessType access)
{
	GPUBufferAccessInfo info = {0};
	
	switch (access) {
	case GPUBufferAccessType_Undefined:
		info.stage  = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case GPUBufferAccessType_GraphicsReadWrite:
		info.stage  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GPUBufferAccessType_ComputeReadWrite:
		info.stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GPUBufferAccessType_TransferSrc:
		info.stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case GPUBufferAccessType_TransferDst:
		info.stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		break;
	}

	return info;
}

internal GPUBufferAccessInfo SyncGetDstBufferAccessInfo(GPUBufferAccessType access)
{
	GPUBufferAccessInfo info = {0};
	
	switch (access) {
	case GPUBufferAccessType_Undefined:
		info.stage  = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case GPUBufferAccessType_GraphicsReadWrite:
		info.stage  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT;
		break;
	case GPUBufferAccessType_ComputeReadWrite:
		info.stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT;
		break;
	case GPUBufferAccessType_TransferSrc:
		info.stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
		break;
	case GPUBufferAccessType_TransferDst:
		info.stage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		break;
	}

	return info;
}
