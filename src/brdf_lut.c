
internal void RenderPassGenerateBRDFLookUp(RenderState *rs,
					   RenderInfo *render_info,
					   void *context)
{
	CommandBuffer *cmd = &rs->cmd;

	GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&shaders->brdf_lut_program, NULL);
	pipeline_def.depth_stencil_state.depth_test_enabled = false;
	pipeline_def.depth_stencil_state.depth_write_enabled = false;
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32_SFLOAT;
		
	PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
	CmdBindBindless(cmd, st.bind_point, st.layout);
	CmdBindPipeline(cmd, st.bind_point, st.pipeline);

	CmdDrawVerticesN(cmd, 3);
}

internal void BRDFLookUpGenerate(RenderGraph *graph, Image *out)
{
	RenderPass render_pass = {0};
	render_pass.type = RenderPassType_Graphics;
	render_pass.graphics.Record = RenderPassGenerateBRDFLookUp;
	render_pass.graphics.attachment_count = 1;
	render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
									    FetchStandardImageView(out),
									    0, v4(0.f, 0.f, 0.f, 1.f));

	RenderGraphPush(graph, &render_pass);
}
