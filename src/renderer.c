
// TODO(kp): (In order of what to do next to achieve feature parity with magpie C++)
//           [x]  1. Pipeline state caching, two seperate hash
//                   tables for layouts and pipelines. Also cache
//                   image views automaticlly.
//           [x]  2. Irradiance + Prefilter map generation.
//           [x]  3. Remove combined image-sampler from bindless
//                   and add seperate image + sampler tables.
//           [x]  4. Generic hash table implementation.
//           [x]  5. Well commented codebase (self commenting code counts).
//           [x]  6. More Assert(...), DebugLog(...) and DebugLogCrash(...) in the codebase.
//           [x]  7. Investigate how I'm taking up ~100kb of memory in the allocated 32MB?
//                   --> RenderPass is just a very big struct, and the renderer has 32
//                       of them at all times.
//           [x]  8. Model loading.
//           [ ]  9. Split up files accordingly to make the code easier to manage.
//           [ ] 10. Bindless material system from C++ pre-rework
//                   (with bindless, a material is *just* data you pass to a
//                   shader, since textures are just parameters
//                   like any other into the material).
//           [ ] 11. Scene system (e.g: a scene might have some
//                   objects to render, multiple lighting probes, etc...)
//           [ ] 12. Due to dynamic rendering, there is a lot of data you have to duplicate
//                   between graphics pipelines and render info's (view mask, formats, etc...),
//                   figure out a way to merge this together. Maybe pass render info
//                   into GraphicsPipelineCreate(...)?
//           [ ] 13. Deferred Rendering.
//                   --> Lighting.
//
//                   <<< MAGPIE C++ ENDS HERE >>>
//
//           [ ] 14. Debug renderer (lines, spheres, etc...) (seperate thing)
//           [ ] 15. Text rendering (fonts)
//           [ ] 16. (This applies to all bindless resources.)
//                   resource_id should *not* be assigned in the
//                   graphics device. In fact, the graphics device
//                   should not be managing bindless in the first
//                   place, that should be a policy of the renderer.
//                   --> Maybe in the future, have a BindlessResources
//                       struct in the high level, that different renderers
//                       can use to manage their bindless resources
//           [ ] 17. Mipmap generation should happen automatically when executing render passes.
//                   --> When that is achieved, automatically call CmdBeginRendering(...) and
//                       CmdEndRendering(...) around the Record(...) function, since mipmaps
//                       are pretty much the only reason I don't already do that.
//           [ ] 18. Switch to using timeline semaphores over fences for frame synchronisation.

internal Mesh
MeshInit(VertexFormat *format,
		 u32 vertex_count, void *vertices,
		 u32 index_count, u16 *indices)
{
	Mesh mesh = {0};
	mesh.vertex_format = format;
	mesh.vertex_count = vertex_count;
	mesh.index_count = index_count;
	
	u64 vertex_buffer_size = vertex_count * format->vertex_size;
	u64 index_buffer_size = index_count * sizeof(u16);
	
	mesh.vertex_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
										   vertex_buffer_size);
	
	mesh.index_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
										  index_buffer_size);
	
	GPUBuffer staging_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
												 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												 vertex_buffer_size + index_buffer_size);
	{
		GPUBufferWrite(&staging_buffer, vertices, vertex_buffer_size, 0);
		GPUBufferWrite(&staging_buffer, indices, index_buffer_size, vertex_buffer_size);
		
		CommandBuffer cmd = BeginGraphicsInstantSubmit();
		{
			VkBufferCopy stage_to_vertex_copy = {0};
			stage_to_vertex_copy.srcOffset = 0;
			stage_to_vertex_copy.dstOffset = 0;
			stage_to_vertex_copy.size = vertex_buffer_size;
			
			CmdCopyBufferToBuffer(&cmd,
								  &staging_buffer,
								  &mesh.vertex_buffer,
								  1, &stage_to_vertex_copy);
			
			VkBufferCopy stage_to_index_copy = {0};
			stage_to_index_copy.srcOffset = vertex_buffer_size;
			stage_to_index_copy.dstOffset = 0;
			stage_to_index_copy.size = index_buffer_size;
			
			CmdCopyBufferToBuffer(&cmd,
								  &staging_buffer,
								  &mesh.index_buffer,
								  1, &stage_to_index_copy);
		}
		EndGraphicsInstantSubmit(&cmd);
	}
	GraphicsWaitIdle();
	GPUBufferDestroy(&staging_buffer);
	
	return mesh;
}

internal void
MeshDestroy(Mesh *mesh)
{
	GPUBufferDestroy(&mesh->vertex_buffer);
	GPUBufferDestroy(&mesh->index_buffer);
}

internal void
CmdBindAndDrawMesh(CommandBuffer *cmd,
				   Mesh *mesh)
{
	CmdBindVertexBuffer(cmd, 0, &mesh->vertex_buffer, 0);
	CmdBindIndexBuffer(cmd, &mesh->index_buffer, 0);
	CmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
}

internal SubModel *
ModelCreateSubModel(Model *model)
{
	SubModel *sub_model = MemoryArenaPush(model->arena, sizeof(SubModel));
	sub_model->next = model->sub_models;
	model->sub_models = sub_model;
	model->sub_model_count++;
	
	return sub_model;
}

internal b32
AssimpMeshHasPositions(struct aiMesh *mesh)
{
	return mesh->mVertices && mesh->mNumVertices > 0;
}

internal b32
AssimpMeshHasFaces(struct aiMesh *mesh)
{
	return mesh->mFaces && mesh->mNumFaces > 0;
}

internal b32
AssimpMeshHasNormals(struct aiMesh *mesh)
{
	return mesh->mNormals && mesh->mNumVertices > 0;
}

internal b32
AssimpMeshHasTangentsAndBitangents(struct aiMesh *mesh)
{
	return mesh->mTangents && mesh->mBitangents && mesh->mNumVertices > 0;
}

internal b32
AssimpMeshHasTextureCoords(struct aiMesh *mesh, u32 index)
{
	return mesh->mTextureCoords[index] && mesh->mNumVertices > 0;
}

internal b32
AssimpMeshHasVertexColours(struct aiMesh *mesh, u32 index)
{
	return mesh->mColors[index] && mesh->mNumVertices > 0;
}

typedef struct ModelVertex
{
	v3 position;
	v2 texcoord;
	v3 colour;
	v3 normal;
	v3 tangent;
	v3 bitangent;
}
ModelVertex;

internal void
ModelLoadProcessSubModel(Renderer *renderer, MemoryArena *arena, SubModel *sub_model, struct aiMesh *assimp_mesh, const struct aiScene *scene, struct aiMatrix4x4 transform)
{
	ScratchArena scratch = GetScratch(arena);
	
	ModelVertex *vertices = MemoryArenaPush(scratch.arena, sizeof(ModelVertex) * assimp_mesh->mNumVertices);
	
	// TODO(kp): Transforms should be applied when rendering (so be a member of a SubModel)
	//           rather than being directly applied to vertices when loading them in.
	for(i32 i = 0; i < assimp_mesh->mNumVertices; i++)
	{
		ModelVertex *vertex = vertices + i;
		
		if(AssimpMeshHasPositions(assimp_mesh))
		{
			struct aiVector3D position = assimp_mesh->mVertices[i];
			aiTransformVecByMatrix4(&position, &transform);
			
			vertex->position = v3(position.x, position.y, position.z); 
		}
		else
		{
			vertex->position = v3(0.f, 0.f, 0.f);
		}
		
		if(AssimpMeshHasTextureCoords(assimp_mesh, 0))
		{
			struct aiVector3D uv = assimp_mesh->mTextureCoords[0][i];
			
			vertex->texcoord = v2(uv.x, uv.y);
		}
		else
		{
			vertex->texcoord = v2(0.f, 0.f);
		}
		
		if(AssimpMeshHasVertexColours(assimp_mesh, 0))
		{
			struct aiColor4D colour = assimp_mesh->mColors[0][i];
			
			vertex->colour = v3(colour.r, colour.g, colour.b);
		}
		else
		{
			vertex->colour = v3(1.f, 1.f, 1.f);
		}
		
		if(AssimpMeshHasNormals(assimp_mesh))
		{
			// TODO(kp): This won't work.
			//           Need to use a corrected transformation matrix for normals!
			//           Unless assimp transformations are orthonormal?
			//           --> Investigate this.
			
			struct aiVector3D normal = assimp_mesh->mNormals[i];
			aiTransformVecByMatrix4(&normal, &transform);
			
			vertex->normal = v3(normal.x, normal.y, normal.z);
		}
		else
		{
			vertex->normal = v3(0.f, 0.f, 1.f);
		}
		
		if(AssimpMeshHasTangentsAndBitangents(assimp_mesh))
		{
			struct aiVector3D tangent = assimp_mesh->mTangents[i];
			struct aiVector3D bitangent = assimp_mesh->mBitangents[i];
			
			aiTransformVecByMatrix4(&tangent, &transform);
			aiTransformVecByMatrix4(&bitangent, &transform);
			
			vertex->tangent = v3(tangent.x, tangent.y, tangent.z);
			vertex->bitangent = v3(bitangent.x, bitangent.y, bitangent.z);
		}
		else
		{
			vertex->tangent = v3(1.f, 0.f, 0.f);
			vertex->bitangent = v3(0.f, 1.f, 0.f);
		}
	}
	
	u32 index_count = 0;
	
	for(i32 i = 0; i < assimp_mesh->mNumFaces; i++)
	{
		struct aiFace *face = assimp_mesh->mFaces + i;
		
		for(i32 j = 0; j < face->mNumIndices; j++)
		{
			index_count++;
		}
	}
	
	u16 *indices = MemoryArenaPush(scratch.arena, sizeof(u16) * index_count);
	
	index_count = 0;
	
	for(i32 i = 0; i < assimp_mesh->mNumFaces; i++)
	{
		struct aiFace *face = assimp_mesh->mFaces + i;
		
		for(i32 j = 0; j < face->mNumIndices; j++)
		{
			indices[index_count] = face->mIndices[j];
			index_count++;
		}
	}
	
	sub_model->mesh = MeshInit(&renderer->model_vertex_format,
							   assimp_mesh->mNumVertices, vertices,
							   index_count, indices);
	
	// TODO(kp): Material is currently unassigned.
	
	ReleaseScratch(&scratch);
}

/*
const aiMaterial *assimpMaterial = scene->mMaterials[assimpMesh->mMaterialIndex];

MaterialData data;
data.technique = "texturedPBR_gbuffer_opaque"; // temporarily just the forced material type

fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_DIFFUSE,				m_app->getTextures().getFallbackDiffuse());
fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_LIGHTMAP,				m_app->getTextures().getFallbackAmbient());
fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_DIFFUSE_ROUGHNESS,	m_app->getTextures().getFallbackRoughnessMetallic());
fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_NORMALS,				m_app->getTextures().getFallbackNormals());
fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_EMISSIVE,				m_app->getTextures().getFallbackEmissive());

submesh->setMaterial(m_app->getRenderer().buildMaterial(data));
*/

internal void
ModelLoadProcessNodes(Renderer *renderer, MemoryArena *arena, Model *model, struct aiNode *node, const struct aiScene *scene, struct aiMatrix4x4 transform)
{
	struct aiMatrix4x4 node_transform = node->mTransformation;
	aiMultiplyMatrix4(&node_transform, &transform);
	
	for(i32 i = 0; i < node->mNumMeshes; i++)
	{
		struct aiMesh *assimp_mesh = scene->mMeshes[node->mMeshes[i]];
		
		SubModel *sub_model = ModelCreateSubModel(model);
		
		ModelLoadProcessSubModel(renderer, arena, sub_model, assimp_mesh, scene, node_transform);
	}
	
	for (i32 i = 0; i < node->mNumChildren; i++)
	{
		ModelLoadProcessNodes(renderer, arena, model, node->mChildren[i], scene, node_transform);
	}
}

internal Model
ModelLoadFromPath(Renderer *renderer, MemoryArena *arena, String8 path)
{
	const struct aiScene *scene = aiImportFile((char *)path.str,
											   aiProcess_Triangulate |
											   aiProcess_FlipWindingOrder |
											   aiProcess_CalcTangentSpace |
											   aiProcess_FlipUVs);
	
	if(!scene ||
	   (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 ||
	   !scene->mRootNode)
	{
		DebugLogCrash("Failed to load model.");
	}
	
	Model model = {0};
	model.arena = arena;
	//model.directory = str8("...");
	
	struct aiMatrix4x4 identity = {
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
	
	DebugLog("Loading model...");
	
	ModelLoadProcessNodes(renderer, arena, &model, scene->mRootNode, scene, identity);
	
	aiReleaseImport(scene);
	return model;
}

internal RenderingAttachment
RenderingAttachmentInitColour(VkAttachmentLoadOp load_op,
							  ImageView *view,
							  ImageView *resolve,
							  v4 clear_colour)
{
	RenderingAttachment attachment = {0};
	
	attachment.info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.info.imageView = view->view;
	attachment.info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.info.loadOp = load_op;
	attachment.info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.info.clearValue = (VkClearValue){ .color = { clear_colour.r, clear_colour.g, clear_colour.b, clear_colour.a } };
	
	if(resolve)
	{
		attachment.info.resolveImageView = resolve->view;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	}
	else
	{
		attachment.info.resolveImageView = VK_NULL_HANDLE;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.info.resolveMode = VK_RESOLVE_MODE_NONE;
	}
	
	attachment.image = view->image;
	
	attachment.width  = view->image->width  >> view->base_mip_level;
	attachment.height = view->image->height >> view->base_mip_level;
	
	return attachment;
}

internal RenderingAttachment
RenderingAttachmentInitDepth(VkAttachmentLoadOp load_op,
							 ImageView *view,
							 ImageView *resolve,
							 f32 clear_depth,
							 u32 clear_stencil)
{
	RenderingAttachment attachment = {0};
	
	attachment.info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.info.imageView = view->view;
	attachment.info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachment.info.loadOp = load_op;
	attachment.info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.info.clearValue = (VkClearValue){ .depthStencil = { clear_depth, clear_stencil } };
	
	if (resolve)
	{
		attachment.info.resolveImageView = resolve->view;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachment.info.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
	}
	else
	{
		attachment.info.resolveImageView = VK_NULL_HANDLE;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.info.resolveMode = VK_RESOLVE_MODE_NONE;
	}
	
	attachment.image = view->image;
	
	attachment.width  = view->image->width  >> view->base_mip_level;
	attachment.height = view->image->height >> view->base_mip_level;
	
	return attachment;
}

internal void
RendererPushRenderPass(Renderer *renderer, RenderPass *pass)
{
	renderer->passes[renderer->pass_count] = *pass;
	renderer->pass_count++;
}

internal void
RendererExecuteRenderPasses(Renderer *renderer, CommandBuffer *cmd)
{
	for(i32 i = 0; i < renderer->pass_count; i++)
	{
		RenderPass pass = renderer->passes[i];
		
		switch(pass.type)
		{
			case RenderPassType_Graphics:
			{
				RenderInfo render_info = {0};
				render_info.view_mask = pass.graphics.view_mask;
				
				for(i32 j = 0; j < pass.graphics.attachment_count; j++)
				{
					RenderingAttachment *attachment = pass.graphics.attachments + j;
					
					render_info.width = attachment->width;
					render_info.height = attachment->height;
					
					render_info.samples = attachment->image->samples;
					
					if(ImageIsDepth(attachment->image))
					{
						render_info.depth_attachment = attachment->info;
						
						CmdTransitionImageLayout(cmd, attachment->image,
												 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
					}
					else
					{
						render_info.colour_attachments[render_info.colour_attachment_count++] = attachment->info;
						
						CmdTransitionImageLayout(cmd, attachment->image,
												 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
					}
				}
				
				for(i32 j = 0; j < pass.graphics.view_count; j++)
				{
					ImageView *view = pass.graphics.views[j];
					
					if(ImageIsDepth(view->image))
					{
						CmdTransitionImageLayout(cmd, view->image,
												 VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
					}
					else
					{
						CmdTransitionImageLayout(cmd, view->image,
												 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
					}
				}
				
				pass.graphics.Record(renderer, cmd, &render_info, pass.context);
			}
			break;
			
			case RenderPassType_Compute:
			{
				// TODO(kp)
			}
			break;
		}
	}
	
	renderer->pass_count = 0;
}

internal void
RendererRenderPassExportHDRCubemap(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
	CmdBeginRendering(cmd, render_info);
	{
		struct
		{
			u64 transform_buffer;
			u32 hdr_image_id;
			u32 linear_sampler_id;
			u32 _padding[2];
		}
		args;
		
		args.transform_buffer = renderer->cubemap_capture_transforms.device_address;
		args.hdr_image_id = FetchStandardImageView(&renderer->environment_hdr_image)->resource_id;
		args.linear_sampler_id = renderer->linear_sampler.resource_id;
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->environment_hdr_to_cubemap_program, &renderer->environment_cube_vertex_format);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
		pipeline_def.view_mask = 0b111111;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
		
		CmdPushConstants(cmd,
						 st.layout,
						 VK_SHADER_STAGE_ALL_GRAPHICS,
						 sizeof(args), &args, 0);
		
		CmdBindAndDrawMesh(cmd, &renderer->environment_cube_mesh);
	}
	CmdEndRendering(cmd);
	
	CmdPrepareForMipmapping(cmd, &renderer->environment_cubemap);
	CmdGenerateMipmaps(cmd, &renderer->environment_cubemap);
	
	DebugLog("Created Environment Cubemap.");
}

internal void
RendererGenerateEnvironmentMap(Renderer *renderer)
{
	renderer->environment_cubemap = ImageAllocate(1024, 1024, 1,
												  VK_FORMAT_R32G32B32A32_SFLOAT,
												  VK_IMAGE_VIEW_TYPE_CUBE,
												  VK_IMAGE_TILING_OPTIMAL,
												  4,
												  VK_SAMPLE_COUNT_1_BIT,
												  false, false);
	
	RenderPass render_pass = {0};
	render_pass.type = RenderPassType_Graphics;
	render_pass.graphics.Record = RendererRenderPassExportHDRCubemap;
	render_pass.graphics.view_mask = 0b111111;
	render_pass.graphics.view_count = 1;
	render_pass.graphics.views[0] = FetchStandardImageView(&renderer->environment_hdr_image);
	render_pass.graphics.attachment_count = 1;
	render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_LOAD,
																		FetchStandardImageView(&renderer->environment_cubemap),
																		0, v4(0.f, 0.f, 0.f, 1.f));
	
	RendererPushRenderPass(renderer, &render_pass);
}

internal void
RendererRenderPassGenerateIrradianceMap(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
	CmdBeginRendering(cmd, render_info);
	{
		struct
		{
			u64 transform_buffer;
			u32 environment_map_id;
			u32 linear_sampler_id;
			u32 _padding[2];
		}
		args;
		
		args.transform_buffer = renderer->cubemap_capture_transforms.device_address;
		args.environment_map_id = FetchStandardImageView(&renderer->environment_cubemap)->resource_id;
		args.linear_sampler_id = renderer->linear_sampler.resource_id;
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->irradiance_map_program, &renderer->environment_cube_vertex_format);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
		pipeline_def.view_mask = 0b111111;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
		
		CmdPushConstants(cmd,
						 st.layout,
						 VK_SHADER_STAGE_ALL_GRAPHICS,
						 sizeof(args), &args, 0);
		
		CmdBindAndDrawMesh(cmd, &renderer->environment_cube_mesh);
	}
	CmdEndRendering(cmd);
	
	CmdPrepareForMipmapping(cmd, &renderer->environment_probe.irradiance);
	CmdGenerateMipmaps(cmd, &renderer->environment_probe.irradiance);
	
	DebugLog("Created Irradiance Cubemap.");
}

internal void
RendererRenderPassGeneratePrefilterMap(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
	CmdBeginRendering(cmd, render_info);
	{
		struct
		{
			u64 transform_buffer;
			f32 roughness;
			u32 environment_map_id;
			u32 linear_sampler_id;
			u32 _padding;
		}
		args;
		
		args.transform_buffer = renderer->cubemap_capture_transforms.device_address;
		args.roughness = *((f32 *)context);
		args.environment_map_id = FetchStandardImageView(&renderer->environment_cubemap)->resource_id;
		args.linear_sampler_id = renderer->linear_sampler.resource_id;
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->prefilter_map_program, &renderer->environment_cube_vertex_format);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
		pipeline_def.view_mask = 0b111111;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
		
		CmdPushConstants(cmd,
						 st.layout,
						 VK_SHADER_STAGE_ALL_GRAPHICS,
						 sizeof(args), &args, 0);
		
		CmdBindAndDrawMesh(cmd, &renderer->environment_cube_mesh);
	}
	CmdEndRendering(cmd);
	
	DebugLog("Created a Prefilter Cubemap mip level.");
}

internal void
RendererGenerateEnvironmentProbeFromEnvironmentCubemap(Renderer *renderer, Image *environment_cubemap)
{
	// NOTE(kp): Irradiance Map.
	{
		renderer->environment_probe.irradiance = ImageAllocate(32, 32, 1,
															   VK_FORMAT_R32G32B32A32_SFLOAT,
															   VK_IMAGE_VIEW_TYPE_CUBE,
															   VK_IMAGE_TILING_OPTIMAL,
															   4,
															   VK_SAMPLE_COUNT_1_BIT,
															   false, false);
		
		RenderPass render_pass = {0};
		render_pass.type = RenderPassType_Graphics;
		render_pass.graphics.Record = RendererRenderPassGenerateIrradianceMap;
		render_pass.graphics.view_mask = 0b111111;
		render_pass.graphics.view_count = 1;
		render_pass.graphics.views[0] = FetchStandardImageView(&renderer->environment_cubemap);
		render_pass.graphics.attachment_count = 1;
		render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_LOAD,
																			FetchStandardImageView(&renderer->environment_probe.irradiance),
																			0, v4(0.f, 0.f, 0.f, 1.f));
		
		RendererPushRenderPass(renderer, &render_pass);
	}
	
	// NOTE(kp): Prefilter Map.
	{
		renderer->environment_probe.prefilter = ImageAllocate(128, 128, 1,
															  VK_FORMAT_R32G32B32A32_SFLOAT,
															  VK_IMAGE_VIEW_TYPE_CUBE,
															  VK_IMAGE_TILING_OPTIMAL,
															  4,
															  VK_SAMPLE_COUNT_1_BIT,
															  false, false);
		
		i32 mipmap_count = renderer->environment_probe.prefilter.mipmap_count;
		
		for(i32 mip_level = 0; mip_level < mipmap_count; mip_level++)
		{
			ImageView *prefilter_view = FetchImageView(&renderer->environment_probe.prefilter,
													   ImageLayerCount(&renderer->environment_probe.prefilter),
													   0, mip_level);
			
			f32 roughness = (f32)(mip_level) / (f32)(mipmap_count - 1);
			
			RenderPass render_pass = {0};
			render_pass.type = RenderPassType_Graphics;
			MemoryCopy(render_pass.context, &roughness, sizeof(f32));
			render_pass.graphics.Record = RendererRenderPassGeneratePrefilterMap;
			render_pass.graphics.view_mask = 0b111111;
			render_pass.graphics.view_count = 1;
			render_pass.graphics.views[0] = FetchStandardImageView(&renderer->environment_cubemap);
			render_pass.graphics.attachment_count = 1;
			render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_LOAD,
																				prefilter_view,
																				0, v4(0.f, 0.f, 0.f, 1.f));
			
			RendererPushRenderPass(renderer, &render_pass);
		}
	}
}

internal void
RendererInit(Renderer *renderer, MemoryArena *arena)
{
	renderer->linear_sampler = SamplerInitFilter(VK_FILTER_NEAREST);
	
	// ---
	
	// NOTE(kp): Generate BRDF lookup table.
	
	// TODO(kp)
	
	// ---
	
	// NOTE(kp): Setup environment buffer and irradiance + prefilter maps.
	
	m4 capture_projection_matrix = M4Perspective(90.0f, 1.0f, 0.1f, 10.0f);
	
	// NOTE(kp): We have to flip the Z lookat coordinate because cubemaps use
	//           a left-handed sampling standard (thank you renderman) but
	//           we use a right-handed coordinate system.
	m4 capture_view_matrices[] = {
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 1.f, 0.f, 0.f), v3(0.f, 1.f, 0.f)), // X+
		M4LookAt(v3(0.f, 0.f, 0.f), v3(-1.f, 0.f, 0.f), v3(0.f, 1.f, 0.f)), // X-
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 1.f, 0.f), v3(0.f, 0.f, 1.f)), // Y+
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f,-1.f, 0.f), v3(0.f, 0.f,-1.f)), // Y-
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f,-1.f), v3(0.f, 1.f, 0.f)), // Z+
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f), v3(0.f, 1.f, 0.f)), // Z-
	};
	
	renderer->cubemap_capture_transforms = GPUBufferAllocate(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
															 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
															 sizeof(m4) * 6);
	
	for(i32 i = 0; i < 6; i++)
	{
		m4 m = M4MultiplyM4(capture_projection_matrix, capture_view_matrices[i]);
		GPUBufferWrite(&renderer->cubemap_capture_transforms, &m, sizeof(m4), sizeof(m4) * i);
	}
	
	typedef struct EnvironmentCubeVertex
	{
		v3 position;
	}
	EnvironmentCubeVertex;
	
	AddVertexBinding(&renderer->environment_cube_vertex_format, sizeof(EnvironmentCubeVertex), VK_VERTEX_INPUT_RATE_VERTEX);
	{
		AddVertexAttribute(&renderer->environment_cube_vertex_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(EnvironmentCubeVertex, position));
	}
	
	EnvironmentCubeVertex vertices[] = {
		{ { -1.f,  1.f, -1.f } },
		{ { -1.f, -1.f, -1.f } },
		{ {  1.f, -1.f, -1.f } },
		{ {  1.f,  1.f, -1.f } },
		{ { -1.f,  1.f,  1.f } },
		{ { -1.f, -1.f,  1.f } },
		{ {  1.f, -1.f,  1.f } },
		{ {  1.f,  1.f,  1.f } }
	};
	
	u16 indices[] = {
		0, 2, 1,
		2, 0, 3,
		
		7, 5, 6,
		5, 7, 4,
		
		4, 1, 5,
		1, 4, 0,
		
		3, 6, 2,
		6, 3, 7,
		
		1, 6, 5,
		6, 1, 2,
		
		4, 3, 0,
		3, 4, 7
	};
	
	renderer->environment_cube_mesh = MeshInit(&renderer->environment_cube_vertex_format,
											   ArraySize(vertices), vertices,
											   ArraySize(indices), indices);
	
	renderer->environment_hdr_image = ImageLoadFromPath(str8("res/environment_map.hdr"));
	
	// TODO(kp): Loading in of assets like shaders and images should be done via a seperate asset system.
	
	renderer->environment_hdr_to_cubemap_program = ShaderProgramInit(sizeof(u64) + sizeof(u32)*4, 2);
	{
		renderer->environment_hdr_to_cubemap_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/hdr_to_environment_cubemap_vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->environment_hdr_to_cubemap_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/hdr_to_environment_cubemap_fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	renderer->irradiance_map_program = ShaderProgramInit(sizeof(u64) + sizeof(u32)*4, 2);
	{
		renderer->irradiance_map_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/irradiance_convolution_vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->irradiance_map_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/irradiance_convolution_fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	renderer->prefilter_map_program = ShaderProgramInit(sizeof(u64) + sizeof(f32) + sizeof(u32)*3, 2);
	{
		renderer->prefilter_map_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/prefilter_convolution_vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->prefilter_map_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/prefilter_convolution_fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	// ---
	
	// NOTE(kp): Setup model related stuff.
	
	renderer->model_program = ShaderProgramInit(sizeof(m4), 2);
	{
		renderer->model_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/model_vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->model_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/model_fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	AddVertexBinding(&renderer->model_vertex_format, sizeof(ModelVertex), VK_VERTEX_INPUT_RATE_VERTEX);
	{
		AddVertexAttribute(&renderer->model_vertex_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, position));
		AddVertexAttribute(&renderer->model_vertex_format, VK_FORMAT_R32G32_SFLOAT,    offsetof(ModelVertex, texcoord));
		AddVertexAttribute(&renderer->model_vertex_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, colour));
		AddVertexAttribute(&renderer->model_vertex_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, normal));
		AddVertexAttribute(&renderer->model_vertex_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, tangent));
		AddVertexAttribute(&renderer->model_vertex_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, bitangent));
	}
	
	renderer->damaged_helmet_model = ModelLoadFromPath(renderer, arena, str8("res/DamagedHelmet/DamagedHelmet.gltf"));
	
	// ---
	
	// NOTE(kp): Depth buffer.
	
	renderer->depth_buffer = ImageAllocate(graphics_device->swapchain.width,
										   graphics_device->swapchain.height,
										   1,
										   graphics_device->depth_format,
										   VK_IMAGE_VIEW_TYPE_2D,
										   VK_IMAGE_TILING_OPTIMAL,
										   1,
										   VK_SAMPLE_COUNT_1_BIT,
										   false, false);
	
	// ---
	
	RendererGenerateEnvironmentMap(renderer);
	RendererGenerateEnvironmentProbeFromEnvironmentCubemap(renderer, &renderer->environment_cubemap);
}

internal void
RendererDestroy(Renderer *renderer)
{
	ImageDestroy(&renderer->depth_buffer);
	SamplerDestroy(&renderer->linear_sampler);
	
	MeshDestroy(&renderer->environment_cube_mesh);
	GPUBufferDestroy(&renderer->cubemap_capture_transforms);
	
	ImageDestroy(&renderer->environment_hdr_image);
	ImageDestroy(&renderer->environment_cubemap);
	
	ImageDestroy(&renderer->environment_probe.irradiance);
	ImageDestroy(&renderer->environment_probe.prefilter);
	
	ShaderProgramDestroy(&renderer->environment_hdr_to_cubemap_program);
	ShaderProgramDestroy(&renderer->irradiance_map_program);
	ShaderProgramDestroy(&renderer->prefilter_map_program);
	ShaderProgramDestroy(&renderer->model_program);
	
	// TODO(kp): Destroy model.
}

internal void
RendererRenderPassRenderModel(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
	CmdBeginRendering(cmd, render_info);
	{
		static f32 time = 0.f;
		time -= .01f;
		
		m4 transform = m4(1.f);
		transform = M4MultiplyM4(M4RotateAxis(time, v3(0.f, 1.f, 0.f)), transform);
		transform = M4MultiplyM4(M4ScaleV3(v3(1.f, 1.f, 1.f)), transform);
		transform = M4MultiplyM4(M4TranslateV3(v3(0.f, 0.f, -4.f)), transform);
		transform = M4MultiplyM4(M4Perspective(70.f, 1280.f/720.f, .1f, 10.f), transform);
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->model_program, &renderer->model_vertex_format);
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = graphics_device->swapchain.format;
		pipeline_def.has_depth_attachment = true;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
		CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(m4), &transform, 0);
		CmdBindAndDrawMesh(cmd, &renderer->damaged_helmet_model.sub_models[0].mesh);
	}
	CmdEndRendering(cmd);
}

internal void
RendererBeginFrame(Renderer *renderer)
{
	renderer->present_cmd = BeginGraphicsPresent();
	
	RenderPass render_pass = {0};
	render_pass.type = RenderPassType_Graphics;
	render_pass.graphics.Record = RendererRenderPassRenderModel;
	render_pass.graphics.attachment_count = 2;
	render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
																		GetCurrentSwapchainImageView(&graphics_device->swapchain),
																		0, v4(0.f, 0.f, 0.f, 1.f));
	render_pass.graphics.attachments[1] = RenderingAttachmentInitDepth(VK_ATTACHMENT_LOAD_OP_CLEAR,
																	   FetchStandardImageView(&renderer->depth_buffer),
																	   0, 1.f, 0);
	
	RendererPushRenderPass(renderer, &render_pass);
}

internal void
RendererEndFrame(Renderer *renderer)
{
	RendererExecuteRenderPasses(renderer, &renderer->present_cmd);
	EndGraphicsPresent(&renderer->present_cmd);
}
