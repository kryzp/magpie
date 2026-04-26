
R_PASS_RECORD_DEF(R_GenerateMipsPassFn)
{
	GFX_CmdBuffer *cmd = ctx->cmd;
	
	const R_GenerateMipsPassData *user_data = ctx->user_data;

	GFX_CmdGenerateMipmaps(cmd, user_data->texture);
}
